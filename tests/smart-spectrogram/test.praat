# Run from repository root with --FULL-TRUST.
Read SmartSpectrogram from CSV/TSV: "irregular.csv"
assert numberOfSelected("SmartSpectrogram") = 1
v = Get value at time and frequency: 0.1, 100
assert abs(v - (-25)) < 0.000001
v = Get value at time and frequency: 0.45, 150
assert abs(v - (-40)) < 0.000001
Set value: 2, 2, -18
v = Get value at time and frequency: 0.1, 100
assert abs(v - (-24)) < 0.000001
Save as CSV file: "../../.local-build/smart-roundtrip.csv"
Save as TSV file: "../../.local-build/smart-roundtrip.tsv"
Save as text file: "../../.local-build/smart-roundtrip.SmartSpectrogram"
Save as binary file: "../../.local-build/smart-roundtrip.bin"
Read SmartSpectrogram from CSV/TSV: "../../.local-build/smart-roundtrip.csv"
v = Get value at time and frequency: 0.1, 100
assert abs(v - (-24)) < 0.000001
Read SmartSpectrogram from CSV/TSV: "../../.local-build/smart-roundtrip.tsv"
v = Get value at time and frequency: 0.1, 100
assert abs(v - (-24)) < 0.000001
Read from file: "../../.local-build/smart-roundtrip.SmartSpectrogram"
v = Get value at time and frequency: 0.1, 100
assert abs(v - (-24)) < 0.000001
Read from file: "../../.local-build/smart-roundtrip.bin"
v = Get value at time and frequency: 0.1, 100
assert abs(v - (-24)) < 0.000001
smart = selected()
Create Sound from formula: "reference", 1, 0, 1, 16000, "0.2*sin(2*pi*200*x)"
sound = selected()
To Pitch (raw autocorrelation): 0.01, 75, 600, 15, "no", 0.03, 0.45, 0.01, 0.35, 0.14
pitch = selected()
selectObject: smart
plusObject: pitch
Attach Pitch
selectObject: smart
Save as binary file: "../../.local-build/smart-pitch.bin"
Read from file: "../../.local-build/smart-pitch.bin"
selectObject: sound
To Intensity: 100, 0, "yes"
intensity = selected()
selectObject: smart
plusObject: intensity
Attach Intensity
selectObject: smart
Save as binary file: "../../.local-build/smart-intensity.bin"
Read from file: "../../.local-build/smart-intensity.bin"
Create FrequencyTrajectories: "overlay", 0, 1, 1
Add point: 1, 0.2, 100
tracks = selected()
selectObject: smart
plusObject: tracks
Attach FrequencyTrajectories
selectObject: smart
Save as binary file: "../../.local-build/smart-overlays.bin"
Read from file: "../../.local-build/smart-overlays.bin"
v = Get value at time and frequency: 0.1, 100
assert abs(v - (-24)) < 0.000001
asserterror Times must be strictly increasing
Read SmartSpectrogram from CSV/TSV: "bad-times.csv"
asserterror expected 3 dB values
Read SmartSpectrogram from CSV/TSV: "bad-width.tsv"
writeInfoLine: "SMART SPECTROGRAM TESTS: PASS"
