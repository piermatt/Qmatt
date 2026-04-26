/*
 * Noise-Optimized Differential ODMR Firmware
 * Target: Seeeduino XIAO ESP32-C3
 * Repository: https://github.com/piermatt/Qmatt
 * MIT license
 * Author: M. Pierpaoli [Gdansk University of Technology]
 *
 * Pin mapping (ESP32-C3 GPIO):
 *   Laser Gate  : D0 = GPIO 2  (via 2N2222A transistor, active-HIGH)
 *   ADC Signal  : D1 = GPIO 3  (ADC_SIGNAL net → TL082 U1.2 output)
 *   ADF4351 CLK : D8 = GPIO 8  (SPI SCK)
 *   ADF4351 DATA: D10 = GPIO 10 (SPI MOSI)
 *   ADF4351 LE  : D7 = GPIO 7  (Chip select / latch enable)
 *   ADF4351 CE  : tied directly to 3.3 V on board (always enabled)
 *
 * TL082 virtual-ground note:
 *   Op-amp runs on +5 V single supply. R1/R2 (1 kΩ each) create
 *   VIRTUAL_GND at +2.5 V → output idles near mid-rail (~2048 counts).
 *   Contrast is computed differentially (ON vs OFF) → offset cancels.
 *
 * NV⁻ charge-state protection:
 *   1. Survey scan: MW always ON, laser gated per step (short pulses).
 *   2. Main sweep: laser gated OFF between frequency steps.
 *
 * Sweep timing (DIFFERENTIAL_AVG = 8):
 *   Per step: 8 × 2 × (5 ms settle + 20 ms ADC) ≈ 0.4 s → ~33 s total
 */

#include <SPI.h>

// ── Pin definitions ────────────────────────────────────────────────
#define PIN_LASER_GATE   1    // GPIO 2 (D0)
#define PIN_ADC_SIGNAL   3    // GPIO 3 (D1) — ADC channel
#define PIN_ADF_LE       5    // GPIO 7 (D7)

// ── Sweep parameters ──────────────────────────────────────────────
#define FREQ_START_MHZ   2850.0f
#define FREQ_STOP_MHZ    2890.0f
#define FREQ_STEP_MHZ    0.5f
#define DIFFERENTIAL_AVG 8         // ON/OFF pairs per step
#define SETTLE_US        5000      // 5 ms RF-switch settling

// ── ADF4351 parameters ────────────────────────────────────────────
#define ADF_REFIN_MHZ    25.0f
#define ADF_POWER        3         // +5 dBm
#define ADF_MOD          50        // fPFD/MOD = 500 kHz resolution

static uint32_t g_R4 = 0;

// ── SPI helper ────────────────────────────────────────────────────
static void adf_write(uint32_t reg) {
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_ADF_LE, LOW);
  SPI.transfer((reg >> 24) & 0xFF);
  SPI.transfer((reg >> 16) & 0xFF);
  SPI.transfer((reg >>  8) & 0xFF);
  SPI.transfer( reg        & 0xFF);
  digitalWrite(PIN_ADF_LE, HIGH);
  delayMicroseconds(2);
  digitalWrite(PIN_ADF_LE, LOW);
  SPI.endTransaction();
}

// ── RF output enable / disable ────────────────────────────────────
static void adf_rf(bool on) {
  if (on) g_R4 |=  (1UL << 5);
  else    g_R4 &= ~(1UL << 5);
  adf_write(g_R4);
}

// ── Program ADF4351 to target frequency ──────────────────────────
static void adf_set_freq(float freq_mhz) {
  const float fPFD = ADF_REFIN_MHZ;
  uint32_t INT  = (uint32_t)(freq_mhz / fPFD);
  uint32_t FRAC = (uint32_t)roundf((freq_mhz / fPFD - INT) * ADF_MOD);
  if (FRAC >= (uint32_t)ADF_MOD) { INT++; FRAC = 0; }

  uint8_t  prescaler = (INT >= 75) ? 1 : 0;
  uint32_t band_sel  = (uint32_t)ceilf(fPFD / 0.125f);
  if (band_sel > 255) band_sel = 255;

  uint32_t R5 = (5UL) | (3UL << 19);
  g_R4 = (4UL) | ((uint32_t)(ADF_POWER & 0x3) << 3) | (1UL << 5) | (1UL << 23);
  uint32_t R3 = (3UL) | (band_sel << 12);
  uint32_t R2 = (2UL) | (1UL << 7) | (7UL << 9) | (1UL << 14)
              | (6UL << 26) | ((uint32_t)prescaler << 27);
  uint32_t R1 = (1UL) | ((uint32_t)ADF_MOD << 3) | (1UL << 15);
  uint32_t R0 = (0UL) | ((FRAC & 0xFFF) << 3) | ((INT & 0xFFFF) << 15);

  adf_write(R5); adf_write(g_R4); adf_write(R3);
  adf_write(R2); adf_write(R1);   adf_write(R0);
}

// ── ADC: 20 ms integration (50 Hz noise rejection) ────────────────
static uint32_t adc_20ms() {
  uint32_t sum = 0;
  for (int i = 0; i < 64; i++) {
    sum += analogRead(PIN_ADC_SIGNAL);
    delayMicroseconds(312);   // 64 × 312 µs ≈ 20 ms
  }
  return sum;
}

static int sweep_steps() {
  return (int)roundf((FREQ_STOP_MHZ - FREQ_START_MHZ) / FREQ_STEP_MHZ) + 1;
}

// ── PASS 1: Survey scan ───────────────────────────────────────────
void run_survey() {
  int steps = sweep_steps();
  Serial.println("# SURVEY_START (MW-ON, laser gated per step)");
  Serial.println("# Freq_MHz, ADC_Sum");
  adf_rf(true);
  for (int i = 0; i < steps; i++) {
    if (Serial.available() && Serial.peek() == 'X') break;
    float freq = FREQ_START_MHZ + i * FREQ_STEP_MHZ;
    adf_set_freq(freq);
    delayMicroseconds(SETTLE_US);
    digitalWrite(PIN_LASER_GATE, HIGH);
    uint32_t val = adc_20ms();
    digitalWrite(PIN_LASER_GATE, LOW);
    Serial.print(freq, 2); Serial.print(", "); Serial.println(val);
  }
  adf_rf(false);
  Serial.println("# SURVEY_END");
}

// ── PASS 2: Differential ODMR sweep ──────────────────────────────
void run_sweep() {
  int steps = sweep_steps();
  Serial.println("# ODMR_SWEEP_START");
  Serial.println("# Freq_MHz, OFF_sum, ON_sum, Contrast_%");
  Serial.println("# Laser warm-up 500 ms...");
  digitalWrite(PIN_LASER_GATE, HIGH); delay(500);
  digitalWrite(PIN_LASER_GATE, LOW);

  for (int i = 0; i < steps; i++) {
    if (Serial.available() && Serial.peek() == 'X') break;
    float freq = FREQ_START_MHZ + i * FREQ_STEP_MHZ;
    adf_set_freq(freq);
    delay(5);

    uint32_t total_off = 0, total_on = 0;
    for (int j = 0; j < DIFFERENTIAL_AVG; j++) {
      adf_rf(false);
      delayMicroseconds(SETTLE_US);
      digitalWrite(PIN_LASER_GATE, HIGH);
      total_off += adc_20ms();
      digitalWrite(PIN_LASER_GATE, LOW);

      adf_rf(true);
      delayMicroseconds(SETTLE_US);
      digitalWrite(PIN_LASER_GATE, HIGH);
      total_on  += adc_20ms();
      digitalWrite(PIN_LASER_GATE, LOW);
    }
    float contrast = (total_off > 0)
      ? ((float)total_off - (float)total_on) / (float)total_off * 100.0f
      : 0.0f;
    Serial.print(freq, 2);   Serial.print(", ");
    Serial.print(total_off); Serial.print(", ");
    Serial.print(total_on);  Serial.print(", ");
    Serial.println(contrast, 4);
  }
  adf_rf(false);
  Serial.println("# ODMR_SWEEP_END");
}

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  pinMode(PIN_LASER_GATE, OUTPUT);
  pinMode(PIN_ADF_LE,     OUTPUT);
  digitalWrite(PIN_LASER_GATE, LOW);
  digitalWrite(PIN_ADF_LE, LOW);
  SPI.begin();
  analogReadResolution(12);
  adf_set_freq(FREQ_START_MHZ);
  adf_rf(false);
  Serial.println("# ODMR ready. Commands: S=full  V=survey  D=diff  X=abort");
}

// ── Loop ─────────────────────────────────────────────────────────
void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if      (c == 'S' || c == 's') { run_survey(); run_sweep(); }
  else if (c == 'V' || c == 'v') { run_survey(); }
  else if (c == 'D' || c == 'd') { run_sweep();  }
}
