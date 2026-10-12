Create Sound from formula: "sound_spectrogram", 1, 0, 1, 16000, "0.2*sin(2*pi*300*x) + 0.1*sin(2*pi*1400*x)"
sound = selected ("Sound")
View & Edit
editor: sound
Spectrogram settings: 0, 4000, 0.005, 30
Spectrogram settings: 0, 5000, 0.01, 80
endeditor
writeFile: "../../.local-build/gui-sound-spectrogram.ok", "ok"
