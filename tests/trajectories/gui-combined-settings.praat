Read SmartSpectrogram from CSV/TSV: "../smart-spectrogram/irregular.csv"
smart = selected ("SmartSpectrogram")
Create Sound from formula: "combined_settings", 1, 0, 1, 16000, "0.2*sin(2*pi*200*x)"
sound = selected ("Sound")
To Spectrogram: 0.005, 5000, 0.002, 20, "Gaussian"
spectrogram = selected ("Spectrogram")
Create FrequencyTrajectories: "combined_settings", 0, 1, 1
tracks = selected ("FrequencyTrajectories")
Add point: 1, 0.2, 300
selectObject: sound
plusObject: tracks
plusObject: smart
plusObject: spectrogram
View & Edit selected
editor: sound
Spectrogram settings: 0, 4000, 0.005, 30
SmartSpectrogram display settings: 0, 3000, 20
Show selected Spectrogram
Show selected SmartSpectrogram
endeditor
writeFile: "../../.local-build/gui-combined-settings.ok", "ok"
