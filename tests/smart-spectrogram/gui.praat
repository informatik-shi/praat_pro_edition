Read SmartSpectrogram from CSV/TSV: "irregular.csv"
smart = selected()
Create Sound from formula: "reference", 1, 0, 1, 16000, "0.2*sin(2*pi*200*x)"
sound = selected()
To Pitch (raw autocorrelation): 0.01, 75, 600, 15, "no", 0.03, 0.45, 0.01, 0.35, 0.14
pitch = selected()
selectObject: smart
plusObject: pitch
Attach Pitch
selectObject: sound
To Intensity: 100, 0, "yes"
intensity = selected()
selectObject: smart
plusObject: intensity
Attach Intensity
Create FrequencyTrajectories: "overlay", 0, 1, 1
Add point: 1, 0.2, 100
tracks = selected()
selectObject: smart
plusObject: tracks
Attach FrequencyTrajectories
selectObject: smart
plusObject: sound
View & Edit
