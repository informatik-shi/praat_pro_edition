# Run by test-tablet-gui.sh in an isolated GTK instance.
Create Sound from formula: "tablet_sound", 1, 0, 1, 22050, "0.2*sin(2*pi*200*x)+0.05*sin(2*pi*1000*x)"
sound = selected()
View & Edit
editor: sound
    Show analyses: "yes", "yes", "yes", "yes", "no", 10
    Analysis length in samples: "yes", 256, 64, 0
endeditor
To TextGrid: "words phones", ""
grid = selected()
Insert boundary: 1, 0.5
Set interval text: 1, 1, "Проверка разметки"
plusObject: sound
View & Edit
selectObject: sound
To Spectrum: "yes"
Rename: "tablet_spectrum"
View & Edit
selectObject: sound
To Pitch: 0.01, 75, 600
Rename: "tablet_pitch"
View & Edit
selectObject: sound
To Intensity: 75, 0.01, "yes"
Down to IntensityTier
Rename: "tablet_intensity"
View & Edit
selectObject: sound
To Formant (burg): 0.01, 5, 5500, 0.025, 50
Down to FormantGrid
Rename: "tablet_formants"
View & Edit
selectObject: sound
To SpectrogramS: 256, 64, 5000, 0, "Gaussian"
Rename: "tablet_samples"
samples = selected()
View & Edit
Read SmartSpectrogram from CSV/TSV: "../smart-spectrogram/irregular.csv"
Rename: "tablet_smart"
smart = selected()
View & Edit
Read frequency trajectories from CSV/TSV: "../trajectories/tracks.csv"
Rename: "tablet_tracks"
tracks = selected()
View & Edit
selectObject: sound
To Spectrogram: 0.005, 5000, 0.002, 20, "Gaussian"
ordinary = selected()
selectObject: sound
plusObject: grid
plusObject: tracks
plusObject: smart
plusObject: ordinary
View & Edit selected
Run Lua file: "plots.lua"
writeFileLine: "../../.local-build/tablet-windows.ok", "PASS: analyses, annotation, custom objects, combined layers and Lua plot opened"
