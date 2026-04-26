# Schematic

## Design Notes

### Signal Chain
```
532 nm laser  ──(gate: Q1/2N2222A)──►  Diamond sample
                                              │ PL (>550 nm)
                                              ▼
                                        BPW34 photodiode
                                              │ photocurrent
                                              ▼
                                   TL082 U1.1 — transimpedance amp
                                   R_f = 1 MΩ, C_f = 10 pF
                                              │ voltage
                                              ▼
                                   TL082 U1.2 — unity buffer
                                   Virtual GND: R1=R2=1kΩ → VIRTUAL_GND=2.5V
                                              │
                                              ▼
                                   ESP32-C3 ADC (GPIO3, 12-bit, Vref=3.3V)
```

### Microwave Path
```
ADF4351 module
  CLK  → ESP32-C3 GPIO8  (SPI SCK)
  DATA → ESP32-C3 GPIO10 (SPI MOSI)
  LE   → ESP32-C3 GPIO7
  CE   → 3.3 V (always enabled)
  RF_OUT → SMA J2 → coaxial → Cu microwave loop near diamond
```

### Laser Gate
```
ESP32-C3 GPIO2 → 10 kΩ → Q1 base
Q1 collector → laser module VCC (3–5V)
Q1 emitter  → GND
```

### Power
- USB-C 5 V from PC or power bank → XIAO onboard 3.3 V LDO
- ADF4351 module powered from 5 V USB line
- Laser module powered from 5 V through Q1
