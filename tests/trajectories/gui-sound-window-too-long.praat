# Regression: a Gaussian window of 0.85 s needs 1.7 s of audio.
Create Sound from formula: "window_length_check", 1, 0, 1.608, 16000, "0.2*sin(2*pi*300*x) + 0.1*sin(2*pi*1400*x)"
sound = selected ("Sound")
View & Edit
editor: sound
Show analyses: "yes", "yes", "yes", "yes", "no", 10
Spectrogram settings: 0, 5000, 0.85, 70
endeditor
