Read sbl file: "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl"
sound = selected("Sound")
Read SmartSpectrogram from CSV/TSV: "../../dist/examples/cta0001-g1-periods.tsv"
smart = selected("SmartSpectrogram")
plusObject: sound
View & Edit
Create FrequencyTrajectories: "periods_check", 0, 1.7, 1
tracks = selected("FrequencyTrajectories")
selectObject: sound
plusObject: smart
plusObject: tracks
View & Edit selected
