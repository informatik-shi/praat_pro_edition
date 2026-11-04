# Open a separate GUI instance for this scenario; no source recording is saved.
Read sbl file: "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl"
Rename: "cta0001_Burg_preview"
sound = selected("Sound")
Read SmartSpectrogram from CSV/TSV: "../../dist/examples/cta0001-g1-formants-smart.tsv"
smart = selected("SmartSpectrogram")
Read from file: "../../dist/examples/cta0001-g1-burg-formants.tsv.FrequencyTrajectories"
tracks = selected("FrequencyTrajectories")
selectObject: sound
regular = To Spectrogram: 0.005, 5000, 0.002, 20, "Gaussian"
Rename: "cta0001_regular_Burg_preview"
selectObject: sound
plusObject: smart
plusObject: tracks
plusObject: regular
View & Edit selected
