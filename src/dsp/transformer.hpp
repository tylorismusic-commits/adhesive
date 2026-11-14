// Adhesive — transformer.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "preamp.hpp"

namespace adhesive {

// A deliberately subtle output transformer.  It is always in the wet path, so
// even with Glue and Tape bypassed the bus still passes through a little iron:
// a low-frequency core lift, a touch of asymmetric second harmonic, and a
// gentle high-frequency roll-off.
class Transformer {
public:
  void prepare(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (auto& hp : highPass_) {
      hp.set(15.0f, sampleRate);
    }
    for (auto& lp : lowPass_) {
      lp.set(std::min(24000.0f, sampleRate * 0.49f), sampleRate);
    }
    for (auto& lp : core_) {
      lp.set(80.0f, sampleRate);
    }
    reset();
  }

  inline float process(int channel, float x) noexcept {
    // transformer core lift in the low end
    x = x + 0.04f * core_[channel].lp(x);

    // small asymmetric Class-A/iron transfer
    constexpr float drive = 0.35f;
    constexpr float bias = 0.012f;
    float y = sat_[channel].process(x * drive, bias) * (1.0f / drive);

    // core high-pass and a gentle top-end roll-off
    y = highPass_[channel].hp(y);
    y = lowPass_[channel].lp(y);
    return y;
  }

  void reset() noexcept {
    for (auto& s : sat_) {
      s.reset();
    }
    for (auto& f : highPass_) {
      f.reset();
    }
    for (auto& f : lowPass_) {
      f.reset();
    }
    for (auto& f : core_) {
      f.reset();
    }
  }

private:
  float sampleRate_ = 48000.0f;
  AdaaTanh sat_[2];
  OnePole highPass_[2];
  OnePole lowPass_[2];
  OnePole core_[2];
};

} // namespace adhesive
