local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
local grid = praat.call("To TextGrid", "segments events", "events")[1]
praat.call("Insert boundary", 1, 0.4)
praat.call("Set interval text", 1, 1, "part_A")
praat.call("Set interval text", 1, 2, "part_B")
praat.call("Insert point", 2, 0.4, "transition")
local count = praat.call("Get number of intervals", 1)
for interval = 1, count do
    local a = praat.call("Get start time of interval", 1, interval)
    local b = praat.call("Get end time of interval", 1, interval)
    local label = praat.call("Get label of interval", 1, interval)
    print(label, a, b, b-a)
end
praat.call("Save as text file", "lesson-annotation.TextGrid")
