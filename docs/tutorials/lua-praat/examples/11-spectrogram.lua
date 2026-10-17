local sound = praat.call("Create Sound from formula", "chirp",
    1, 0, 1, 16000, "0.2*sin(2*pi*(200*x+900*x*x))")[1]
local wide = praat.call("To Spectrogram", 0.005, 4000, 0.002, 20, "Gaussian")[1]
praat.select(sound)
local narrow = praat.call("To Spectrogram", 0.04, 4000, 0.002, 20, "Gaussian")[1]
print("spectrogram_ids", wide, narrow)
praat.call("Save as text file", "lesson-spectrogram.txt")
