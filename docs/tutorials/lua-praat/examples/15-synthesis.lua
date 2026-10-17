local grid = praat.call("Create KlattGrid from vowel",
    "lesson_vowel", 1, 125, 700, 60, 1200, 90,
    2500, 150, 3500, 0.05, 1000)[1]
local sound = praat.call("To Sound")[1]
praat.call("Save as WAV file", "lesson-vowel.wav")
praat.select(grid)
praat.call("Add pitch point", 0.8, 180)
local rising = praat.call("To Sound")[1]
praat.call("Save as WAV file", "lesson-rising-vowel.wav")
print("sound_ids", sound, rising)
