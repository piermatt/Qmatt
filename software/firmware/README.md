# Firmware — odmr_firmware.ino

## Target Platform
- **Board**: Seeeduino XIAO ESP32-C3
- **Arduino IDE**: ≥ 2.x with esp32 board support ≥ 3.x
- **Board package**: `esp32` by Espressif Systems

## Installation
1. In Arduino IDE → Boards Manager → search "esp32" → install Espressif esp32
2. Select: **Tools → Board → XIAO_ESP32C3**
3. Tools → Flash Size: **4MB (No OTA)**
4. Upload `odmr_firmware.ino`

## Serial Commands (115200 baud)
| Command | Action                              |
|---------|-------------------------------------|
| `S`     | Survey scan then differential sweep |
| `V`     | Survey scan only                    |
| `D`     | Differential sweep only             |
| `X`     | Abort current scan mid-sweep        |

## Output Format — Survey
```
# SURVEY_START
# Freq_MHz, ADC_Sum
2850.00, 87234
2850.50, 87190
...
# SURVEY_END
```

## Output Format — Differential Sweep
```
# ODMR_SWEEP_START
# Freq_MHz, OFF_sum, ON_sum, Contrast_%
2850.00, 698876, 698712, 0.0235
...
# ODMR_SWEEP_END
```

## Tuning Parameters
Edit these `#define` lines in the firmware:

| Define             | Default   | Description                  |
|--------------------|-----------|------------------------------|
| `FREQ_START_MHZ`   | 2850.0    | Sweep start frequency (MHz)  |
| `FREQ_STOP_MHZ`    | 2890.0    | Sweep stop frequency (MHz)   |
| `FREQ_STEP_MHZ`    | 0.5       | Step size (MHz)              |
| `DIFFERENTIAL_AVG` | 8         | ON/OFF averages per step     |
| `SETTLE_US`        | 5000      | RF settle time (µs)          |
| `ADF_POWER`        | 3 (+5dBm) | ADF4351 output power 0–3     |

## Dependencies
- Built-in `SPI.h` only — no external libraries required.
