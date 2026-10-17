local grid = praat.call("Create KlattGrid from vowel",
    "lesson_vowel", 1, 125, 700, 60, 1200, 90,
    2500, 150, 3500, 0.05, 1000)[1]
local sound = praat.call("To Sound")[1]
local manipulation = praat.call("To Manipulation", 0.01, 75, 500)[1]
local tier = praat.call("Extract pitch tier")[1]
praat.call("Multiply frequencies", 0, 0, 1.2)
praat.select(manipulation, tier)
praat.call("Replace pitch tier")
praat.select(manipulation)
local result = praat.call("Get resynthesis (overlap-add)")[1]
praat.call("Save as WAV file", "lesson-resynthesized.wav")
print("duration_s", praat.call("Get total duration"))
