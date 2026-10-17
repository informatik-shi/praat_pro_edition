local sound = praat.call("Create Sound from formula", "pause_demo",
    1, 0, 1, 44100,
    "if x>0.25 and x<0.75 then 0.2*sin(2*pi*200*x) else 0 fi")[1]
local intensity = praat.call("To Intensity", 75, 0.01, true)[1]
print("energy_mean_dB", praat.call("Get mean", 0, 0, "energy"))
print("median_dB", praat.call("Get quantile", 0, 0, 0.5))
local grid = praat.call("To TextGrid (silences)",
    -25, 0.10, 0.10, "silent", "sounding")[1]
print("intervals", praat.call("Get number of intervals", 1))
praat.call("Save as text file", "lesson-silences.TextGrid")
