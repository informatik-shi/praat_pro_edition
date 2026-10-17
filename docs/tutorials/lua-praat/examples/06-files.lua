local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
praat.call("Save as WAV file", "lesson-tone.wav")
local loaded = praat.call("Read from file", "lesson-tone.wav")[1]
print("loaded_duration_s", praat.call("Get total duration"))
local part = praat.call("Extract part", 0.2, 0.5, "rectangular", 1, false)[1]
assert(math.abs(praat.call("Get total duration") - 0.3) < 1e-9)
praat.call("Save as WAV file", "lesson-part.wav")
local lfs = require("lfs")
print("folder", lfs.currentdir())
print("wav_bytes", lfs.attributes("lesson-tone.wav", "size"))
