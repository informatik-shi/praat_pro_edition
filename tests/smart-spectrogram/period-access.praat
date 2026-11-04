Read SmartSpectrogram from CSV/TSV: "intervals.tsv"
smart = selected()
v = Get value at time and frequency: 0.25, 10
assert v = undefined
v = Get value at time and frequency: 0.01, 10
assert v = undefined
v = Get value at time and frequency: 0.99, 10
assert v = undefined
v = Get value at time and frequency: 0.125, 10
assert abs(v + 20) < 0.000001
# A gap still permits explicit extraction of the nearest calculated frame.
To Spectrum (nearest period): 0.25
f = Get frequency from bin number: 1
assert f = 10
ltas = To Ltas (1-to-1)
v = object[ltas,1]
assert abs(v + 20) < 0.000001
selectObject: smart
To Spectrum (nearest period): 0.7
ltas = To Ltas (1-to-1)
assert abs(object[ltas,1] + 60) < 0.000001
selectObject: smart
To Spectrum (nearest period): -2
ltas = To Ltas (1-to-1)
assert abs(object[ltas,2] + 30) < 0.000001
selectObject: smart
To Spectrum (nearest period): 2
ltas = To Ltas (1-to-1)
assert abs(object[ltas,3] + 80) < 0.000001
selectObject: smart
Save as TSV file: "../../.local-build/smart-intervals-roundtrip.tsv"
Save as binary file: "../../.local-build/smart-intervals.bin"
Save as text file: "../../.local-build/smart-intervals.txt"
Read SmartSpectrogram from CSV/TSV: "../../.local-build/smart-intervals-roundtrip.tsv"
v = Get value at time and frequency: 0.25, 10
assert v = undefined
Read from file: "../../.local-build/smart-intervals.bin"
v = Get value at time and frequency: 0.25, 10
assert v = undefined
Read from file: "../../.local-build/smart-intervals.txt"
v = Get value at time and frequency: 0.25, 10
assert v = undefined
writeInfoLine: "SMART PERIOD ACCESS TESTS: PASS"
