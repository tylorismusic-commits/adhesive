// Adhesive — DSP tests
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <cstdio>
#include <vector>

#include "adhesive_strip.hpp"
#include "glue.hpp"
#include "params.hpp"
#include "tape.hpp"
#include "transformer.hpp"

using namespace adhesive;

static int g_failures = 0;

static void check(bool ok, const char* what) {
  if (!ok) {
    ++g_failures;
    printf("FAIL: %s\n", what);
  }
}

static float rms(const std::vector<float>& buf, uint32_t start) {
  double sum = 0.0;
  for (uint32_t i = start; i < buf.size(); ++i) {
    sum += static_cast<double>(buf[i]) * buf[i];
  }
  return static_cast<float>(std::sqrt(sum / (buf.size() - start)));
}

static void fillSine(std::vector<float>& l, std::vector<float>& r, float freq,
                     float amp, float sr) {
  const double w = 2.0 * 3.14159265358979323846 * freq / sr;
  for (size_t i = 0; i < l.size(); ++i) {
    const float x = static_cast<float>(amp * std::sin(w * i));
    l[i] = x;
    r[i] = x;
  }
}

int main() {
  printf("Adhesive DSP tests\n");

  check(std::fabs(dbToGain(0.0f) - 1.0f) < 1.0e-6f, "dbToGain(0) == 1");
  check(std::fabs(gainToDb(1.0f)) < 1.0e-3f, "gainToDb(1) == 0");
  check(std::fabs(gainToDb(0.5f) + 6.0206f) < 0.05f, "gainToDb(0.5) == -6.02");

  // --- Glue: 4:1, threshold -20, 1 kHz sine at -6 dBFS ---
  {
    AdhesiveStrip strip;
    strip.prepare(48000.0f);
    Params p;
    p.enabled = true;
    p.inputGainDb = 0.0f;
    p.glueOn = true;
    p.glueThresholdDb = -20.0f;
    p.glueRatio = 4.0f;
    p.glueAttackMs = 10.0f;
    p.glueReleaseMs = 250.0f;
    p.glueMakeupDb = 0.0f;
    p.glueMix = 1.0f;
    p.tapeOn = false;
    p.outputDb = 0.0f;
    strip.setTargets(p);
    strip.reset();

    const uint32_t n = 48000;
    std::vector<float> l(n), r(n);
    fillSine(l, r, 1000.0f, 0.5f, 48000.0f);
    float gr = 0.0f;
    float out = -60.0f;
    strip.process(l.data(), r.data(), l.data(), r.data(), n, gr, out);
    check(gr > -14.0f && gr < -8.0f, "Glue reports ~ -11 dB reduction");
    check(out < -3.0f, "Glue output reduced");
    check(std::fabs(strip.latencySamples() - 192.0f) < 1.0f,
          "latency is the 4 ms tape transport delay at 48 kHz");
  }

  // --- Tape off still passes the aligned dry signal ---
  {
    AdhesiveStrip strip;
    strip.prepare(48000.0f);
    Params p;
    p.enabled = true;
    p.glueOn = false;
    p.tapeOn = false;
    p.inputGainDb = 0.0f;
    p.outputDb = 0.0f;
    strip.setTargets(p);
    strip.reset();

    const uint32_t n = 9600;
    std::vector<float> l(n), r(n);
    fillSine(l, r, 1000.0f, 0.25f, 48000.0f);
    float gr = 0.0f, out = -60.0f;
    strip.process(l.data(), r.data(), l.data(), r.data(), n, gr, out);
    float peak = 0.0f;
    for (uint32_t i = n - 4800; i < n; ++i) {
      peak = std::max(peak, std::fabs(l[i]));
    }
    check(peak > 0.22f && peak < 0.28f, "tape-off path stays near unity");
    check(std::isfinite(out) && out > -13.0f, "tape-off output meter is sane");
  }

  // --- Cassette is narrower and darker than reel at 12 kHz ---
  {
    std::vector<float> reelL(96000), reelR(96000);
    std::vector<float> casL(96000), casR(96000);
    fillSine(reelL, reelR, 12000.0f, 0.25f, 48000.0f);
    casL = reelL;
    casR = reelR;

    Params reel;
    reel.glueOn = false;
    reel.tapeOn = true;
    reel.tapeType = 0;
    reel.tapeDrive = 0.0f;
    reel.tapeWow = 0.0f;
    reel.tapeMix = 1.0f;

    Params cassette = reel;
    cassette.tapeType = 1;

    AdhesiveStrip a;
    a.prepare(48000.0f);
    a.setTargets(reel);
    a.reset();
    float gr = 0.0f, out = -60.0f;
    a.process(reelL.data(), reelR.data(), reelL.data(), reelR.data(), 96000,
              gr, out);

    AdhesiveStrip b;
    b.prepare(48000.0f);
    b.setTargets(cassette);
    b.reset();
    b.process(casL.data(), casR.data(), casL.data(), casR.data(), 96000, gr,
              out);

    const float reelRms = rms(reelL, 48000);
    const float cassetteRms = rms(casL, 48000);
    check(reelRms > 0.10f, "reel passes 12 kHz");
    check(cassetteRms < reelRms * 0.80f, "cassette rolls off 12 kHz");
  }

  // --- Always-on transformer colours a flat signal even with Glue/Tape off ---
  {
    AdhesiveStrip strip;
    strip.prepare(48000.0f);
    Params p;
    p.enabled = true;
    p.glueOn = false;
    p.tapeOn = false;
    p.inputGainDb = 0.0f;
    p.outputDb = 0.0f;
    strip.setTargets(p);
    strip.reset();

    const uint32_t n = 48000;
    std::vector<float> l(n), r(n);
    fillSine(l, r, 1000.0f, 0.5f, 48000.0f);
    float gr = 0.0f, out = -60.0f;
    // Reference: bypassed signal for the same input.
    std::vector<float> dryL = l;
    std::vector<float> dryR = r;
    AdhesiveStrip bypass;
    bypass.prepare(48000.0f);
    p.enabled = false;
    bypass.setTargets(p);
    bypass.reset();
    bypass.process(dryL.data(), dryR.data(), dryL.data(), dryR.data(), n, gr,
                   out);

    p.enabled = true;
    strip.setTargets(p);
    strip.reset();
    strip.process(l.data(), r.data(), l.data(), r.data(), n, gr, out);

    double diff = 0.0;
    double ref = 0.0;
    for (uint32_t i = 24000; i < n; ++i) {
      diff += std::fabs(static_cast<double>(l[i]) - dryL[i]);
      ref += std::fabs(static_cast<double>(dryL[i]));
    }
    const double relative = diff / std::max(ref, 1.0e-12);
    check(relative > 1.0e-5, "transformer is not a null operation");
    check(relative < 0.08, "transformer colour stays subtle");
  }

  // --- Master bypass skips the tape delay line and the transformer ---
  {
    AdhesiveStrip strip;
    strip.prepare(48000.0f);
    Params p;
    p.enabled = false;
    p.glueOn = true;
    p.tapeOn = true;
    p.tapeType = 1;
    p.glueThresholdDb = -30.0f;
    p.inputGainDb = 12.0f;
    p.outputDb = 6.0f;
    strip.setTargets(p);
    strip.reset();

    const uint32_t n = 9600;
    std::vector<float> l(n, 0.05f), r(n, 0.05f);
    float gr = 0.0f, out = -60.0f;
    strip.process(l.data(), r.data(), l.data(), r.data(), n, gr, out);
    check(std::fabs(l[n - 1] - 0.05f) < 0.002f,
          "master bypass passes the dry signal");
    check(std::fabs(gr) < 1.0e-6f, "master bypass reports no gain reduction");
  }

  // --- Tape before and after Glue produce different results ---
  {
    Params base;
    base.glueOn = true;
    base.glueThresholdDb = -24.0f;
    base.glueRatio = 10.0f;
    base.glueAttackMs = 0.1f;
    base.glueReleaseMs = 100.0f;
    base.glueMakeupDb = 6.0f;
    base.tapeOn = true;
    base.tapeDrive = 80.0f;
    base.tapeMix = 1.0f;

    std::vector<float> preL(24000), preR(24000);
    std::vector<float> postL(24000), postR(24000);
    fillSine(preL, preR, 220.0f, 0.7f, 48000.0f);
    postL = preL;
    postR = preR;

    AdhesiveStrip a;
    a.prepare(48000.0f);
    base.tapePosition = 0;
    a.setTargets(base);
    a.reset();
    float gr = 0.0f, out = -60.0f;
    a.process(preL.data(), preR.data(), preL.data(), preR.data(), 24000, gr,
              out);

    AdhesiveStrip b;
    b.prepare(48000.0f);
    base.tapePosition = 1;
    b.setTargets(base);
    b.reset();
    b.process(postL.data(), postR.data(), postL.data(), postR.data(), 24000,
              gr, out);

    double diff = 0.0;
    for (size_t i = 0; i < preL.size(); ++i) {
      diff += std::fabs(static_cast<double>(preL[i]) - postL[i]);
    }
    check(diff > 1.0, "tape pre/post routing is audibly different");
  }

  // --- Extreme cassette chain must stay finite ---
  {
    AdhesiveStrip strip;
    strip.prepare(48000.0f);
    Params p;
    p.enabled = true;
    p.inputGainDb = 20.0f;
    p.glueOn = true;
    p.glueThresholdDb = -30.0f;
    p.glueRatio = 10.0f;
    p.glueAttackMs = 0.1f;
    p.glueReleaseMs = 100.0f;
    p.glueMakeupDb = 20.0f;
    p.glueMix = 1.0f;
    p.tapeOn = true;
    p.tapeType = 1;
    p.tapePosition = 0;
    p.tapeDrive = 100.0f;
    p.tapeWow = 100.0f;
    p.tapeMix = 1.0f;
    p.outputDb = 12.0f;
    strip.setTargets(p);
    strip.reset();

    const uint32_t n = 4800;
    std::vector<float> l(n), r(n);
    uint32_t seed = 12345;
    for (uint32_t i = 0; i < n; ++i) {
      seed = seed * 1103515245u + 12345u;
      const float noise =
          static_cast<float>(static_cast<int>((seed >> 16) & 0x7fffu) - 4096) /
          4096.0f;
      l[i] = noise;
      r[i] = noise;
    }
    float gr = 0.0f, out = -60.0f;
    strip.process(l.data(), r.data(), l.data(), r.data(), n, gr, out);
    check(std::isfinite(gr) && std::isfinite(out),
          "extreme chain stays finite");
  }

  // --- Several sample rates ---
  for (const float sr : {44100.0f, 48000.0f, 96000.0f, 192000.0f}) {
    AdhesiveStrip strip;
    strip.prepare(sr);
    Params p;
    p.enabled = true;
    p.glueOn = false;
    p.tapeOn = true;
    p.tapePosition = 1;
    p.outputDb = 0.0f;
    strip.setTargets(p);
    strip.reset();
    const uint32_t n = 2048;
    std::vector<float> l(n, 0.25f), r(n, 0.25f);
    float gr = 0.0f, out = -60.0f;
    strip.process(l.data(), r.data(), l.data(), r.data(), n, gr, out);
    check(std::isfinite(l[n - 1]) && std::isfinite(out),
          "finite at sample rate");
    check(strip.latencySamples() ==
              static_cast<int>(std::lround(0.004f * sr)),
          "latency scales with sample rate");
  }

  if (g_failures == 0) {
    printf("ALL DSP TESTS PASSED\n");
  }
  return g_failures == 0 ? 0 : 1;
}
