// Adhesive — delay.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace adhesive {

// Fixed integer delay used to keep the dry path aligned with the tape stage's
// transport delay.  The reported latency is the tape base delay.
class FixedDelay {
public:
  void prepare(float sampleRate, float delayMs) noexcept {
    sampleRate_ = sampleRate;
    size_ = std::max(64, static_cast<int>(sampleRate * 0.05f) + 4);
    delay_ = std::max(1, std::min(size_ - 2,
                                  static_cast<int>(std::lround(
                                      delayMs * 0.001f * sampleRate))));
    for (auto& b : buffer_) {
      b.assign(static_cast<size_t>(size_), 0.0f);
    }
    write_ = 0;
  }

  void reset() noexcept {
    for (auto& b : buffer_) {
      std::fill(b.begin(), b.end(), 0.0f);
    }
    write_ = 0;
  }

  inline void process(float& left, float& right) noexcept {
    if (size_ <= 0) {
      return;
    }
    buffer_[0][write_] = left;
    buffer_[1][write_] = right;
    int read = write_ - delay_;
    if (read < 0) {
      read += size_;
    }
    left = buffer_[0][read];
    right = buffer_[1][read];
    write_ = (write_ + 1) % size_;
  }

  int latencySamples() const noexcept { return delay_; }

private:
  float sampleRate_ = 48000.0f;
  std::vector<float> buffer_[2];
  int write_ = 0;
  int delay_ = 1;
  int size_ = 0;
};

} // namespace adhesive
