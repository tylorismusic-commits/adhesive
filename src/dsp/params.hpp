// Adhesive — params.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace adhesive {

struct Params {
  float inputGainDb = 0.0f;

  bool glueOn = true;
  float glueThresholdDb = -18.0f;
  float glueRatio = 2.0f;      // 2, 4 or 10
  float glueAttackMs = 30.0f;  // 0.1 .. 30
  float glueReleaseMs = 0.0f;  // 0 = auto, else 100 .. 1200
  float glueMakeupDb = 0.0f;
  float glueMix = 1.0f;        // 0..1

  bool tapeOn = true;
  int tapeType = 0;            // 0 = reel, 1 = cassette
  int tapePosition = 1;        // 0 = before Glue, 1 = after Glue
  float tapeDrive = 20.0f;     // 0..100 %
  float tapeWow = 10.0f;       // 0..100 %
  float tapeMix = 0.25f;       // 0..1

  float outputDb = 0.0f;
  bool enabled = true;
};

} // namespace adhesive
