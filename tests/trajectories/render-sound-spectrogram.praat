Create Sound from formula: "spectrogram_pixels", 1, 0, 1, 16000, "0.2*sin(2*pi*300*x) + 0.1*sin(2*pi*1400*x)"
To Spectrogram: 0.005, 5000, 0.002, 20, "Gaussian"
Paint: 0, 1, 0, 5000, 100, "yes", 30, 6, 0, "no"
Save as 300-dpi PNG file: "../../.local-build/spectrogram-narrow.png"
Erase all
Paint: 0, 1, 0, 5000, 100, "yes", 80, 6, 0, "no"
Save as 300-dpi PNG file: "../../.local-build/spectrogram-pixels.png"
