Create Sound from formula: "window_recovery", 1, 0, 1.608, 16000, "0.2*sin(2*pi*300*x)"
sound = selected ("Sound")
View & Edit
editor: sound
Show analyses: "yes", "yes", "no", "no", "no", 10
Spectrogram settings: 0, 5000, 0.85, 70
asserterror The spectrogram is not defined at the edge of the sound.
Extract visible spectrogram
Spectrogram settings: 0, 5000, 0.005, 70
Extract visible spectrogram
endeditor
assert numberOfSelected ("Spectrogram") = 1
writeFile: "../../.local-build/gui-sound-window-recovery.ok", "PASS"
