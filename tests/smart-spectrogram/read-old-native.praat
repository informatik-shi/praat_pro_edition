Read from file: "legacy-v0.SmartSpectrogram"
v = Get value at time and frequency: 0.1, 100
assert abs(v + 25) < 0.000001
To Spectrum (nearest period): 0
ltas = To Ltas (1-to-1)
assert abs(object[ltas,2] + 30) < 0.000001
writeInfoLine: "SMART VERSION 0 COMPATIBILITY: PASS"
