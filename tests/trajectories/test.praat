# Run with --FULL-TRUST from the repository root. Temporary files stay in .local-build.
Read frequency trajectories from CSV/TSV: "tracks.csv"
original = selected()
v = Get value at time: 1, 0.5
assert v = 650
v = Get value at time: 2, 0.5
assert v = 1400
Save as CSV file: "../../.local-build/trajectory-roundtrip.csv"
Save as TSV file: "../../.local-build/trajectory-roundtrip.tsv"
Save as text file: "../../.local-build/trajectory-roundtrip.FrequencyTrajectories"
Save as binary file: "../../.local-build/trajectory-roundtrip.bin"
Read frequency trajectories from CSV/TSV: "../../.local-build/trajectory-roundtrip.csv"
v = Get value at time: 1, 0.55
assert abs(v - 665) < 0.000001
Read frequency trajectories from CSV/TSV: "../../.local-build/trajectory-roundtrip.tsv"
v = Get value at time: 2, 0.55
assert abs(v - 1390) < 0.000001
Read from file: "../../.local-build/trajectory-roundtrip.FrequencyTrajectories"
v = Get value at time: 3, 0.5
assert v = 2500
Read from file: "../../.local-build/trajectory-roundtrip.bin"
v = Get value at time: 1, 0.5
assert v = 650
asserterror Duplicate time
Read frequency trajectories from CSV/TSV: "duplicate.csv"
asserterror frequency cannot be negative
Read frequency trajectories from CSV/TSV: "invalid.csv"
Read frequency trajectories from CSV/TSV: "quoted.csv"
v = Get value at time: 1, 0.5
assert v = 500
Save as CSV file: "../../.local-build/trajectory-quoted.csv"
Read frequency trajectories from CSV/TSV: "../../.local-build/trajectory-quoted.csv"
v = Get value at time: 2, 0.5
assert v = 1600
Create FrequencyTrajectories: "empty", 0, 1, 2
Add point: 1, 0.1, 400
Add point: 1, 0.9, 800
v = Get value at time: 1, 0.5
assert v = 600
asserterror outside
Add point: 1, 2, 700
asserterror out of range
Add point: 3, 0.5, 700
Create Sound from formula: "source", 1, 0, 1, 16000, "0.15*sin(2*pi*500*x)+0.1*sin(2*pi*1500*x)"
To Formant (burg): 0.01, 5, 5500, 0.025, 50
To FrequencyTrajectories
v = Get value at time: 1, 0.5
assert v > 0
writeInfoLine: "TRAJECTORIES TESTS: PASS"
