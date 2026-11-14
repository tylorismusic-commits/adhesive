// Adhesive — lv2.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <cstdint>
#include <new>

#include <lv2/core/lv2.h>

#include "adhesive_strip.hpp"
#include "param_info.hpp"
#include "params.hpp"

namespace adhesive {

class AdhesiveInstance {
public:
  float* ports[PORT_COUNT] = {};
  float sampleRate = 48000.0f;
  AdhesiveStrip strip;
  Params params;
};

static const ParamInfo& infoFor(uint32_t index) noexcept {
  for (int i = 0; i < kParamInfoCount; ++i) {
    if (kParams[i].index == index) {
      return kParams[i];
    }
  }
  static ParamInfo fallback;
  fallback.index = index;
  return fallback;
}

static float ctrlValue(float* const* ports, uint32_t index,
                       float fallback) noexcept {
  const ParamInfo& info = infoFor(index);
  float v = fallback;
  if (index < PORT_COUNT && ports[index]) {
    v = ports[index][0];
  }
  if (!std::isfinite(v)) {
    v = fallback;
  }
  const float lo = std::min(info.min, info.max);
  const float hi = std::max(info.min, info.max);
  if (hi > lo) {
    v = std::min(std::max(v, lo), hi);
  }
  return v;
}

static void readParams(AdhesiveInstance* self) noexcept {
  self->params.inputGainDb = ctrlValue(self->ports, PORT_INPUT_GAIN, 0.0f);

  self->params.glueOn = ctrlValue(self->ports, PORT_GLUE_ON, 1.0f) > 0.5f;
  self->params.glueThresholdDb =
      ctrlValue(self->ports, PORT_GLUE_THRESHOLD, -15.0f);
  self->params.glueRatio = ctrlValue(self->ports, PORT_GLUE_RATIO, 2.0f);
  self->params.glueAttackMs = ctrlValue(self->ports, PORT_GLUE_ATTACK, 30.0f);
  self->params.glueReleaseMs =
      ctrlValue(self->ports, PORT_GLUE_RELEASE, 0.0f);
  self->params.glueMakeupDb = ctrlValue(self->ports, PORT_GLUE_MAKEUP, 0.0f);
  self->params.glueMix = ctrlValue(self->ports, PORT_GLUE_MIX, 100.0f) / 100.0f;

  self->params.tapeOn = ctrlValue(self->ports, PORT_TAPE_ON, 1.0f) > 0.5f;
  self->params.tapeType =
      ctrlValue(self->ports, PORT_TAPE_TYPE, 0.0f) >= 0.5f ? 1 : 0;
  self->params.tapePosition =
      ctrlValue(self->ports, PORT_TAPE_POSITION, 1.0f) >= 0.5f ? 1 : 0;
  self->params.tapeDrive = ctrlValue(self->ports, PORT_TAPE_DRIVE, 20.0f);
  self->params.tapeWow = ctrlValue(self->ports, PORT_TAPE_WOW, 10.0f);
  self->params.tapeMix = ctrlValue(self->ports, PORT_TAPE_MIX, 25.0f) / 100.0f;

  self->params.outputDb = ctrlValue(self->ports, PORT_OUTPUT_GAIN, 0.0f);
  self->params.enabled = ctrlValue(self->ports, PORT_ENABLE, 1.0f) > 0.5f;
}

static void reportLatency(AdhesiveInstance* self) noexcept {
  if (PORT_LATENCY < PORT_COUNT && self->ports[PORT_LATENCY]) {
    self->ports[PORT_LATENCY][0] =
        static_cast<float>(self->strip.latencySamples());
  }
}

} // namespace adhesive

using namespace adhesive;

extern "C" LV2_Handle lv2_adhesive_instantiate(
    const LV2_Descriptor* descriptor, double rate, const char* bundle_path,
    const LV2_Feature* const* features) {
  (void)descriptor;
  (void)bundle_path;
  (void)features;

  adhesive::AdhesiveInstance* self =
      new (std::nothrow) adhesive::AdhesiveInstance();
  if (!self) {
    return nullptr;
  }
  self->sampleRate = std::max(8000.0f, static_cast<float>(rate));
  self->strip.prepare(self->sampleRate);

  adhesive::readParams(self);
  self->strip.setTargets(self->params);
  self->strip.reset();
  return self;
}

extern "C" void lv2_adhesive_connect_port(LV2_Handle handle, uint32_t port,
                                         void* data_location) {
  adhesive::AdhesiveInstance* self =
      static_cast<adhesive::AdhesiveInstance*>(handle);
  if (!self) {
    return;
  }
  if (port < adhesive::PORT_COUNT) {
    self->ports[port] = static_cast<float*>(data_location);
  }
  if (port == adhesive::PORT_LATENCY && self->ports[port]) {
    self->ports[port][0] = static_cast<float>(self->strip.latencySamples());
  }
}

extern "C" void lv2_adhesive_activate(LV2_Handle handle) {
  adhesive::AdhesiveInstance* self =
      static_cast<adhesive::AdhesiveInstance*>(handle);
  if (!self) {
    return;
  }
  adhesive::readParams(self);
  self->strip.setTargets(self->params);
  self->strip.reset();
  adhesive::reportLatency(self);
}

extern "C" void lv2_adhesive_run(LV2_Handle handle, uint32_t sampleCount) {
  adhesive::AdhesiveInstance* self =
      static_cast<adhesive::AdhesiveInstance*>(handle);
  if (!self) {
    return;
  }

  const bool audioValid =
      self->ports[PORT_AUDIO_IN_L] && self->ports[PORT_AUDIO_IN_R] &&
      self->ports[PORT_AUDIO_OUT_L] && self->ports[PORT_AUDIO_OUT_R];

  float grDb = 0.0f;
  float outDb = -60.0f;
  if (audioValid) {
    adhesive::readParams(self);
    self->strip.setTargets(self->params);
    self->strip.process(
        self->ports[PORT_AUDIO_IN_L], self->ports[PORT_AUDIO_IN_R],
        self->ports[PORT_AUDIO_OUT_L], self->ports[PORT_AUDIO_OUT_R],
        sampleCount, grDb, outDb);
  }

  if (self->ports[PORT_METER_GR]) {
    self->ports[PORT_METER_GR][0] = grDb;
  }
  if (self->ports[PORT_METER_OUT]) {
    self->ports[PORT_METER_OUT][0] = outDb;
  }
  adhesive::reportLatency(self);
}

extern "C" void lv2_adhesive_deactivate(LV2_Handle handle) {
  adhesive::AdhesiveInstance* self =
      static_cast<adhesive::AdhesiveInstance*>(handle);
  if (!self) {
    return;
  }
  self->strip.reset();
}

extern "C" void lv2_adhesive_cleanup(LV2_Handle handle) {
  delete static_cast<adhesive::AdhesiveInstance*>(handle);
}

extern "C" const void* lv2_adhesive_extension_data(const char* uri) {
  (void)uri;
  return nullptr;
}

static const LV2_Descriptor adhesive_descriptor = {
    "https://github.com/tylorismusic-commits/adhesive",
    &lv2_adhesive_instantiate,
    &lv2_adhesive_connect_port,
    &lv2_adhesive_activate,
    &lv2_adhesive_run,
    &lv2_adhesive_deactivate,
    &lv2_adhesive_cleanup,
    &lv2_adhesive_extension_data,
};

extern "C" LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(
    uint32_t index) {
  if (index == 0) {
    return &adhesive_descriptor;
  }
  return nullptr;
}
