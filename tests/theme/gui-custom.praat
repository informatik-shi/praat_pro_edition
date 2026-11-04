Create Sound from formula: "Theme_custom_preview", 1, 0, 1.6, 22050, "0.2*sin(2*pi*200*x)+0.1*sin(2*pi*800*x)+0.05*sin(2*pi*1600*x)"
sound = selected("Sound")
Read SmartSpectrogram from CSV/TSV: "../../dist/examples/cta0001-g1-formants-smart.tsv"
smart = selected("SmartSpectrogram")
Read from file: "../../dist/examples/cta0001-g1-burg-formants.tsv.FrequencyTrajectories"
tracks = selected("FrequencyTrajectories")
selectObject: sound
regular = To Spectrogram: 0.005, 5000, 0.002, 20, "Gaussian"
selectObject: sound
plusObject: smart
plusObject: tracks
plusObject: regular
View & Edit selected
selectObject: sound
To Spectrum: "yes"
View & Edit
