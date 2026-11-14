// Adhesive — adhesive_strip.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cstdint>

#include "delay.hpp"
#include "filters.hpp"
#include "glue.hpp"
#include "params.hpp"
#include "preamp.hpp"
#include "smoother.hpp"
#include "tape.hpp"
#include "transformer.hpp"
#include "util.hpp"

namespace adhesive {

// Signal flow: input trim -> [tape, pre] -> Glue -> [tape, post] -> output
// transformer -> fader -> hidden clipper -> master bypass crossfade.
//
// The tape stage always runs (even when bypassed) because its transport delay
// is part of the plugin latency.  The dry bypass path is delayed by the same
// amount so bypass, tape-off, and tape-on all stay time-aligned in the host.
class AdhesiveStrip {
public:
  void prepare(float sampleRate) noexcept {
    sampleRate_ = sampleRate;

    glue_.prepare(sampleRate);
    tape_.prepare(sampleRate);
    transformer_.prepare(sampleRate);
    dryDelay_.prepare(sampleRate, Tape::kBaseDelayMs);

    inputGain_.setTime(20.0f, sampleRate);
    outputGain_.setTime(20.0f, sampleRate);
    enableAmt_.setTime(10.0f, sampleRate);
    inputGain_.reset(1.0f);
    outputGain_.reset(1.0f);
    enableAmt_.reset(1.0f);

    counter_ = 0;
  }

  void setTargets(const Params& p) noexcept {
    params_ = p;
    inputGain_.setTarget(dbToGain(p.inputGainDb));
    outputGain_.setTarget(dbToGain(p.outputDb));
    enableAmt_.setTarget(p.enabled ? 1.0f : 0.0f);

    glue_.setTargets(p.glueThresholdDb, p.glueRatio, p.glueAttackMs,
                     p.glueReleaseMs, p.glueMakeupDb, p.glueMix, p.glueOn);
    tape_.setTargets(p.tapeDrive, p.tapeWow, p.tapeMix, p.tapeOn,
                     p.tapeType);
  }

  void reset() noexcept {
    glue_.reset();
    tape_.reset();
    transformer_.reset();
    dryDelay_.reset();
    inputGain_.snap();
    outputGain_.snap();
    enableAmt_.snap();
    counter_ = 0;
  }

  int latencySamples() const noexcept { return dryDelay_.latencySamples(); }

  // Processes `frames` samples from the input buffers into the output buffers
  // (which may alias the inputs).  Returns the Glue gain reduction (<= 0 dB)
  // and the peak output level in dBFS (clamped for display).
  void process(const float* inLeft, const float* inRight, float* outLeft,
               float* outRight, uint32_t frames, float& grDb,
               float& outDb) noexcept {
    float lastGr = 0.0f;
    float peak = 0.0f;

    for (uint32_t i = 0; i < frames; ++i) {
      const float inGain = inputGain_.next();
      const float outGain = outputGain_.next();
      const float enable = enableAmt_.next();

      glue_.tick();
      tape_.tick();

      if ((counter_ & 15u) == 0u) {
        glue_.updateCoeffs();
        tape_.update();
      }

      float dryL = inLeft[i];
      float dryR = inRight[i];
      dryDelay_.process(dryL, dryR);

      float wetL = inLeft[i] * inGain;
      float wetR = inRight[i] * inGain;

      if (params_.tapePosition == 0) {
        tape_.process(wetL, wetR);
      }

      lastGr = glue_.process(wetL, wetR);

      if (params_.tapePosition != 0) {
        tape_.process(wetL, wetR);
      }

      // Always-on output iron: subtle even when Glue and Tape are bypassed.
      wetL = transformer_.process(0, wetL);
      wetR = transformer_.process(1, wetR);

      const float faderL = wetL * outGain;
      const float faderR = wetR * outGain;

      // Hidden output protection: linear below the knee, a smooth tanh
      // shoulder above it, and a hard ceiling at +0.2 dBFS.
      const float clippedL = softClip(faderL);
      const float clippedR = softClip(faderR);

      const float finalL = dryL + enable * (clippedL - dryL);
      const float finalR = dryR + enable * (clippedR - dryR);

      outLeft[i] = finalL;
      outRight[i] = finalR;

      peak = std::max(peak, std::max(std::fabs(finalL), std::fabs(finalR)));
      ++counter_;
    }

    grDb = lastGr;
    outDb = clamp(gainToDb(peak), -60.0f, 6.0f);
  }

private:
  float sampleRate_ = 48000.0f;
  Params params_;
  Glue glue_;
  Tape tape_;
  Transformer transformer_;
  FixedDelay dryDelay_;
  Smoothed inputGain_;
  Smoothed outputGain_;
  Smoothed enableAmt_;
  uint32_t counter_ = 0;

  // +0.2 dBFS: 10^(0.2/20)
  static constexpr float kOutputClipLevel = 1.02329299f;
  static constexpr float kOutputClipKnee = 0.90f * kOutputClipLevel;

  static inline float softClip(float x) noexcept {
    const float ax = std::fabs(x);
    if (ax <= kOutputClipKnee) {
      return x;
    }
    const float over = (ax - kOutputClipKnee) / (kOutputClipLevel - kOutputClipKnee);
    const float shaped =
        kOutputClipKnee + (kOutputClipLevel - kOutputClipKnee) * std::tanh(over);
    return x < 0.0f ? -shaped : shaped;
  }
};

} // namespace adhesive
