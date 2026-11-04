writeFileLine: "../../.local-build/spectrogram-s-gui.trace", "begin"
# Native Sound Editor, sample publication, stable hop grid on zoom, and Sound + SpectrogramS.
Create Sound from formula: "sample_settings_demo", 1, 0, 0.5, 22050, "0.2*sin(2*pi*500*x)+0.05*sin(2*pi*1500*x)"
sound = selected()
To SpectrogramS: 256, 64, 5000, 0, "Gaussian"
reference = selected()
selectObject: sound
View & Edit
editor: sound
    Show analyses: "yes", "no", "no", "no", "no", 10
    Analysis length in samples: "yes", 256, 64, 0
    Extract visible spectrogram
endeditor
assert numberOfSelected("SpectrogramS") = 1
initial = selected()
samples = Get analysis length in samples
assert samples = 256
editor: sound
    Zoom: 0.1, 0.2
    Extract visible spectrogram
endeditor
assert numberOfSelected("SpectrogramS") = 1
zoomed = selected()
first = Get time from frame number: 1
value = Get power at: first, 500
selectObject: reference
expected = Get power at: first, 500
assert abs(value-expected)<1e-9
appendFileLine: "../../.local-build/spectrogram-s-gui.trace", "base checks complete"
# Non-power-of-two settings publish the unpadded direct DFT.
editor: sound
    Analysis length in samples: "yes", 300, 64, 0
    Extract visible spectrogram
endeditor
assert numberOfSelected("SpectrogramS") = 1
size = Get transform size in samples
assert size = 300
method$ = Get transform method
assert method$ = "direct DFT"
editor: sound
    Analysis length in samples: "yes", 256, 64, 0
endeditor
appendFileLine: "../../.local-build/spectrogram-s-gui.trace", "DFT complete"
# A selected interval still publishes the nearest exact-window sample spectrum.
editor: sound
    Select: 0.14, 0.16
    View spectral slice
endeditor
assert numberOfSelected("Spectrum") = 1
frequency = Get frequency from bin number: 7
real = Get real value in bin: 7
imaginary = Get imaginary value in bin: 7
selectObject: reference
nearest_frame = Get frame number from time: 0.15
nearest_time = Get time from frame number: round(nearest_frame)
expected = Get power at: nearest_time, frequency
assert abs(real^2 + imaginary^2 - expected)<1e-9
appendFileLine: "../../.local-build/spectrogram-s-gui.trace", "slice complete"
# Old settings continue to provide ordinary Spectrogram analysis when seconds change.
editor: sound
    Spectrogram settings: 0, 5000, 0.003, 70
    Extract visible spectrogram
endeditor
assert numberOfSelected("Spectrogram") = 1
editor: sound
    Analysis length in samples: "yes", 256, 64, 0
endeditor
appendFileLine: "../../.local-build/spectrogram-s-gui.trace", "seconds complete"
selectObject: reference
View & Edit
appendFileLine: "../../.local-build/spectrogram-s-gui.trace", "standalone complete"
selectObject: sound
plusObject: reference
View & Edit
writeFileLine: "../../.local-build/spectrogram-s-gui-final.ok", "PASS: sample settings, direct DFT, publication, zoom grid, spectral slice, seconds fallback, standalone and combined editors"
