# Results

This folder contains example sweep data and expected output plots.

## `sample_data/sweep_example.csv`
Synthetic ODMR sweep simulating:
- Diamond with NV⁻ centers, zero-field splitting ≈ 2870 MHz
- Simulated resonance at 2870.2 MHz (Zeeman-shifted by ~6 mT external field)
- Lorentzian FWHM ≈ 3.5 MHz
- Peak contrast ≈ 1.8 %
- Gaussian noise σ = 0.05 % (realistic for 8-average single-supply ADC)

Run analysis:
```bash
python software/analysis/odmr_analysis.py results/sample_data/sweep_example.csv --save
```

## Expected Fit Output
```
Resonance centre : 2870.200 ± 0.08 MHz
Linewidth (FWHM) : 3.5 MHz
Contrast         : 1.80 %
Estimated B-field: ~7.1 mT  (projection along NV axis)
```

## Tips for Real Data
- If contrast is < 0.1%, check laser alignment and filter placement.
- Broad linewidth (> 10 MHz) suggests MW power too high or poor impedance matching of the loop antenna.
- Multiple dips indicate B-field splitting ms = +1 and ms = -1 (use double-Lorentzian fit).
- Baseline drift: increase `DIFFERENTIAL_AVG` in firmware or reduce laser power duty cycle.
