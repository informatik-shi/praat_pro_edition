local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
local pulses = praat.call("To PointProcess (periodic, cc)", 75, 500)[1]
print("pulse_count", praat.call("Get number of points"))
local jitter = praat.call("Get jitter (local)", 0, 0, 0.0001, 0.02, 1.3)
print("jitter_fraction", jitter, "jitter_percent", 100*jitter)
assert(jitter == jitter and jitter < 0.01)
