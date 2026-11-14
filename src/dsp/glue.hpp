// Adhesive — glue.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "filters.hpp"
#include "smoother.hpp"
#include "util.hpp"

namespace adhesive {

// SSL 4000G-style stereo VCA bus compressor.  Stepped ratio/attack/release,
// soft knee, fixed 100 Hz side-chain high-pass, and the classic program-
// dependent Auto release (a multi-stage recovery approximated by a
// gain-reduction-dependent release time).  The VCA path itself is clean; the
// console colour comes from the tape and output transformer stages.
class Glue {
public:
  void prepare(float sampleRate) noexcept {
    sampleRate_ = sampleRate;

    threshold_.setTime(20.0f, sampleRate);
    ratio_.setTime(20.0f, sampleRate);
    attackMs_.setTime(20.0f, sampleRate);
    releaseMs_.setTime(20.0f, sampleRate);
    makeup_.setTime(20.0f, sampleRate);
    mix_.setTime(20.0f, sampleRate);
    active_.setTime(10.0f, sampleRate);

    threshold_.reset(-18.0f);
    ratio_.reset(2.0f);
    attackMs_.reset(30.0f);
    releaseMs_.reset(400.0f);
    makeup_.reset(1.0f);
    mix_.reset(1.0f);
    active_.reset(1.0f);

    for (auto& hp : sidechainHp_) {
      hp.set(100.0f, sampleRate);
      hp.reset();
    }

    env_ = 0.0f;
    gain_ = 1.0f;
    autoRelease_ = true;
    updateCoeffs();
  }

  void setTargets(float thresholdDb, float ratio, float attackMs,
                  float releaseMs, float makeupDb, float mix01,
                  bool enabled) noexcept {
    threshold_.setTarget(thresholdDb);
    ratio_.setTarget(ratio);
    attackMs_.setTarget(attackMs);
    autoRelease_ = releaseMs <= 0.5f;
    releaseMs_.setTarget(autoRelease_ ? 400.0f : releaseMs);
    makeup_.setTarget(dbToGain(makeupDb));
    mix_.setTarget(clamp(mix01, 0.0f, 1.0f));
    active_.setTarget(enabled ? 1.0f : 0.0f);
  }

  void tick() noexcept {
    threshold_.next();
    ratio_.next();
    attackMs_.next();
    releaseMs_.next();
    makeup_.next();
    mix_.next();
    active_.next();
  }

  void updateCoeffs() noexcept {
    const float attackSamples =
        std::max(0.01f, attackMs_.current() * 0.001f * sampleRate_);
    float releaseMs = releaseMs_.current();
    if (autoRelease_) {
      const float gr = clamp(-gainToDb(gain_), 0.0f, 20.0f);
      releaseMs = 160.0f + 35.0f * gr;
    }
    const float releaseSamples =
        std::max(0.1f, releaseMs * 0.001f * sampleRate_);
    attackCoeff_ = std::exp(-1.0f / attackSamples);
    releaseCoeff_ = std::exp(-1.0f / releaseSamples);
  }

  // Applies gain reduction and returns the meter value in dB (<= 0).
  float process(float& left, float& right) noexcept {
    const float detLeft = sidechainHp_[0].hp(left);
    const float detRight = sidechainHp_[1].hp(right);
    const float detector = std::max(std::fabs(detLeft), std::fabs(detRight));

    if (detector > env_) {
      env_ = attackCoeff_ * env_ + (1.0f - attackCoeff_) * detector;
    } else {
      env_ = releaseCoeff_ * env_ + (1.0f - releaseCoeff_) * detector;
    }
    if (env_ < 1.0e-20f) {
      env_ = 0.0f;
    }

    const float levelDb = clamp(gainToDb(env_), -120.0f, 24.0f);
    const float threshold = threshold_.current();
    const float ratio = std::max(ratio_.current(), 1.0f);
    const float slope = 1.0f - 1.0f / ratio;
    const float knee = 6.0f;

    const float over = levelDb - threshold;
    float reductionDb;
    if (over <= -0.5f * knee) {
      reductionDb = 0.0f;
    } else if (over >= 0.5f * knee) {
      reductionDb = slope * over;
    } else {
      const float t = over + 0.5f * knee;
      reductionDb = slope * t * t / (2.0f * knee);
    }
    reductionDb = std::max(reductionDb, 0.0f);

    const float nextGain = dbToGain(-reductionDb);
    const float makeup = makeup_.current();
    const float amount = mix_.current() * active_.current();

    const float wetLeft = left * nextGain * makeup;
    const float wetRight = right * nextGain * makeup;
    left += amount * (wetLeft - left);
    right += amount * (wetRight - right);

    gain_ = nextGain;
    return active_.current() > 1.0e-4f ? -reductionDb : 0.0f;
  }

  void reset() noexcept {
    for (auto& hp : sidechainHp_) {
      hp.reset();
    }
    env_ = 0.0f;
    gain_ = 1.0f;
    threshold_.snap();
    ratio_.snap();
    attackMs_.snap();
    releaseMs_.snap();
    makeup_.snap();
    mix_.snap();
    active_.snap();
    updateCoeffs();
  }

private:
  float sampleRate_ = 48000.0f;
  Smoothed threshold_;
  Smoothed ratio_;
  Smoothed attackMs_;
  Smoothed releaseMs_;
  Smoothed makeup_;
  Smoothed mix_;
  Smoothed active_;
  OnePole sidechainHp_[2];
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float env_ = 0.0f;
  float gain_ = 1.0f;
  bool autoRelease_ = true;
};

} // namespace adhesive
