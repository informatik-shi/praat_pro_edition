local grid = praat.call("Create KlattGrid from vowel",
    "lesson_vowel", 1, 125, 700, 60, 1200, 90,
    2500, 150, 3500, 0.05, 1000)[1]
local sound = praat.call("To Sound")[1]
local lowRate = praat.call("Resample", 11000, 50)[1]
local lpc = praat.call("To LPC (burg)", 10, 0.025, 0.005, 50)[1]
print("lpc_frames", praat.call("Get number of frames"))
praat.call("Save as text file", "lesson.LPC")
