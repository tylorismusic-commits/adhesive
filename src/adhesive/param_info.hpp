// Adhesive — param_info.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace adhesive {

enum class Unit {
  Unitless = 0,
  Db = 1,
  Hz = 2,
  Ms = 3,
  Percent = 4,
  Ratio = 5,
};

// Control ports, in ascending port index order, interleaved with the stereo
// audio ports as defined in the plugin RDF.
enum PortIndex : uint32_t {
  PORT_AUDIO_IN_L = 0,
  PORT_AUDIO_IN_R = 1,
  PORT_AUDIO_OUT_L = 2,
  PORT_AUDIO_OUT_R = 3,
  PORT_INPUT_GAIN = 4,
  PORT_GLUE_ON = 5,
  PORT_GLUE_THRESHOLD = 6,
  PORT_GLUE_RATIO = 7,
  PORT_GLUE_ATTACK = 8,
  PORT_GLUE_RELEASE = 9,
  PORT_GLUE_MAKEUP = 10,
  PORT_GLUE_MIX = 11,
  PORT_TAPE_ON = 12,
  PORT_TAPE_TYPE = 13,
  PORT_TAPE_POSITION = 14,
  PORT_TAPE_DRIVE = 15,
  PORT_TAPE_WOW = 16,
  PORT_TAPE_MIX = 17,
  PORT_OUTPUT_GAIN = 18,
  PORT_ENABLE = 19,
  PORT_METER_GR = 20,
  PORT_METER_OUT = 21,
  PORT_LATENCY = 22,
  PORT_COUNT = 23
};

enum ParamGroup : uint32_t {
  GROUP_IO = 0,
  GROUP_GLUE = 1,
  GROUP_TAPE = 2,
  GROUP_GLOBAL = 3,
};

struct ScalePoint {
  float value = 0.0f;
  const char* label = "";
};

struct ParamInfo {
  uint32_t index = 0;
  const char* symbol = "";
  const char* name = "";
  float min = 0.0f;
  float max = 1.0f;
  float def = 0.0f;
  Unit unit = Unit::Unitless;
  bool logarithmic = false;
  bool toggle = false;
  bool enumeration = false;
  int group = GROUP_IO;
  const ScalePoint* points = nullptr;
  int pointCount = 0;
};

// Stepped SSL 4000G controls.  The release list starts at 0, which means the
// program-dependent Auto release.
inline constexpr ScalePoint kScaleRatio[] = {
    {2.0f, "2:1"}, {4.0f, "4:1"}, {10.0f, "10:1"}};

inline constexpr ScalePoint kScaleAttack[] = {
    {0.1f, "0.1"}, {0.3f, "0.3"}, {1.0f, "1"}, {3.0f, "3"},
    {10.0f, "10"}, {30.0f, "30"}};

inline constexpr ScalePoint kScaleRelease[] = {
    {0.0f, "AUTO"}, {100.0f, "0.1"}, {300.0f, "0.3"},
    {600.0f, "0.6"}, {1200.0f, "1.2"}};

inline constexpr ScalePoint kScaleTapeType[] = {
    {0.0f, "REEL"}, {1.0f, "CASSETTE"}};

inline constexpr ScalePoint kScaleTapePosition[] = {
    {0.0f, "PRE"}, {1.0f, "POST"}};

inline constexpr ParamInfo kParams[] = {
    // Audio ports 0-3 are handled in the LV2 wrapper.  The table below lists
    // the control ports in ascending index order.
    {4, "input_gain", "Input", -20.0f, 20.0f, 0.0f, Unit::Db, false, false,
     false, GROUP_IO, nullptr, 0},
    {5, "glue_on", "Glue", 0.0f, 1.0f, 1.0f, Unit::Unitless, false, true, false,
     GROUP_GLUE, nullptr, 0},
    {6, "glue_threshold", "Threshold", -30.0f, 0.0f, -15.0f, Unit::Db, false,
     false, false, GROUP_GLUE, nullptr, 0},
    {7, "glue_ratio", "Ratio", 2.0f, 10.0f, 2.0f, Unit::Ratio, false, false,
     false, GROUP_GLUE, kScaleRatio, 3},
    {8, "glue_attack", "Attack", 0.1f, 30.0f, 30.0f, Unit::Ms, false, false,
     false, GROUP_GLUE, kScaleAttack, 6},
    {9, "glue_release", "Release", 0.0f, 1200.0f, 0.0f, Unit::Ms, false, false,
     false, GROUP_GLUE, kScaleRelease, 5},
    {10, "glue_makeup", "Makeup", 0.0f, 20.0f, 0.0f, Unit::Db, false, false,
     false, GROUP_GLUE, nullptr, 0},
    {11, "glue_mix", "Mix", 0.0f, 100.0f, 100.0f, Unit::Percent, false, false,
     false, GROUP_GLUE, nullptr, 0},
    {12, "tape_on", "Tape", 0.0f, 1.0f, 1.0f, Unit::Unitless, false, true,
     false, GROUP_TAPE, nullptr, 0},
    {13, "tape_type", "Tape Type", 0.0f, 1.0f, 0.0f, Unit::Unitless, false,
     false, true, GROUP_TAPE, kScaleTapeType, 2},
    {14, "tape_position", "Tape Position", 0.0f, 1.0f, 1.0f, Unit::Unitless,
     false, false, true, GROUP_TAPE, kScaleTapePosition, 2},
    {15, "tape_drive", "Drive", 0.0f, 100.0f, 20.0f, Unit::Percent, false,
     false, false, GROUP_TAPE, nullptr, 0},
    {16, "tape_wow", "Wow", 0.0f, 100.0f, 10.0f, Unit::Percent, false, false,
     false, GROUP_TAPE, nullptr, 0},
    {17, "tape_mix", "Tape Mix", 0.0f, 100.0f, 25.0f, Unit::Percent, false,
     false, false, GROUP_TAPE, nullptr, 0},
    {18, "output_gain", "Output", -60.0f, 12.0f, 0.0f, Unit::Db, false, false,
     false, GROUP_IO, nullptr, 0},
    {19, "enable", "Enabled", 0.0f, 1.0f, 1.0f, Unit::Unitless, false, true,
     false, GROUP_GLOBAL, nullptr, 0},
};

inline constexpr int kParamInfoCount =
    static_cast<int>(sizeof(kParams) / sizeof(kParams[0]));

// Meter and latency ports.
inline constexpr uint32_t kMeterGrPort = PORT_METER_GR;
inline constexpr uint32_t kMeterOutPort = PORT_METER_OUT;
inline constexpr uint32_t kLatencyPort = PORT_LATENCY;

} // namespace adhesive
