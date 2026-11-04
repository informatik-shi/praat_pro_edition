# Change these values and run in Praat Pro Edition.
# 300 -> direct DFT; 256 -> FFT. No implicit padding when transform_samples=0.
analysis_samples = 300
hop_samples = 75
transform_samples = 0
Create Sound from formula: "Sample_count_demo", 1, 0, 0.6, 22050, "0.2*sin(2*pi*500*x)+0.06*sin(2*pi*1500*x)"
sound = selected()
To SpectrogramS: analysis_samples, hop_samples, 5000, transform_samples, "Gaussian"
spectrogram = selected()
method$ = Get transform method
length_ms = Get analysis length in milliseconds
writeInfoLine: "Method: ", method$, "; analysis: ", analysis_samples, " samples = ", fixed$(length_ms,6), " ms"
selectObject: sound
plusObject: spectrogram
View & Edit
