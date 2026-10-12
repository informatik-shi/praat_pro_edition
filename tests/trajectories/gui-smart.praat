Read SmartSpectrogram from CSV/TSV: "../smart-spectrogram/irregular.csv"
smart = selected ("SmartSpectrogram")
Create FrequencyTrajectories: "smart_overlay", 0, 1, 3
tracks = selected ("FrequencyTrajectories")
Add point: 1, 0.2, 100
Add point: 1, 0.7, 150
Add point: 2, 0.2, 250
Create Sound from formula: "reference", 1, 0, 1, 16000, "0.2*sin(2*pi*200*x)"
sound = selected ("Sound")
selectObject: tracks
plusObject: smart
plusObject: sound
View & Edit
