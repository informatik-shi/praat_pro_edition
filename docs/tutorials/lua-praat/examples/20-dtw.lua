local grid = praat.call("Create KlattGrid from vowel",
    "lesson_vowel", 1, 125, 700, 60, 1200, 90,
    2500, 150, 3500, 0.05, 1000)[1]
local sound = praat.call("To Sound")[1]
local a = praat.call("To MFCC", 12, 0.015, 0.005, 100, 100, 0)[1]
local b = praat.call("Copy", "mfcc_copy")[1]
praat.select(a, b)
local dtw = praat.call("To DTW", 1, 0, 0, 0, 0.056,
    true, true, "no restriction")[1]
local distance = praat.call("Get distance (weighted)")
print("self_weighted_distance", distance)
assert(distance == distance and distance >= 0 and distance < math.huge)
local diagonal = praat.call("Get distance value", 0.5, 0.5)
local mapped = praat.call("Get y time from x time", 0.5)
print("diagonal_distance", diagonal, "mapped_time_s", mapped)
assert(math.abs(diagonal) < 1e-8 and math.abs(mapped-0.5) < 0.01)
