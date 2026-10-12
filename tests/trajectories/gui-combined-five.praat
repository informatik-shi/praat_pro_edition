Read SmartSpectrogram from CSV/TSV: "../smart-spectrogram/irregular.csv"
smart = selected ("SmartSpectrogram")
Create Sound from formula: "combined_five", 1, 0, 1, 16000, "0.2*sin(2*pi*200*x)"
sound = selected ("Sound")
To Spectrogram: 0.005, 5000, 0.002, 20, "Gaussian"
spectrogram = selected ("Spectrogram")
Create TextGrid: 0, 1, "words", ""
grid = selected ("TextGrid")
Create FrequencyTrajectories: "combined_five", 0, 1, 2
tracks = selected ("FrequencyTrajectories")
Add point: 1, 0.2, 300
Add point: 1, 0.7, 500
selectObject: sound
plusObject: grid
plusObject: tracks
plusObject: smart
plusObject: spectrogram
View & Edit selected
