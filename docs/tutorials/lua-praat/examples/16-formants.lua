local grid = praat.call("Create KlattGrid from vowel",
    "lesson_vowel", 1, 125, 700, 60, 1200, 90,
    2500, 150, 3500, 0.05, 1000)[1]
local sound = praat.call("To Sound")[1]
local formant = praat.call("To Formant (burg)", 0.005, 5, 5500, 0.025, 50)[1]
for _, t in ipairs({0.25, 0.5, 0.75}) do
    local f1 = praat.call("Get value at time", 1, t, "Hertz", "linear")
    local f2 = praat.call("Get value at time", 2, t, "Hertz", "linear")
    print("time_F1_F2", t, f1, f2)
    assert(f1 == f1 and f2 == f2, "Missing formant estimate")
    assert(f1 > 400 and f1 < 1000 and f2 > 900 and f2 < 1600)
end
