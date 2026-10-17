local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
local grid = praat.call("To TextGrid", "segments", "")[1]
praat.call("Insert boundary", 1, 0.5)
praat.call("Set interval text", 1, 1, "early")
praat.call("Set interval text", 1, 2, "late")
praat.select(sound)
local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
praat.select(grid)
local n = praat.call("Get number of intervals", 1)
for i = 1, n do
    praat.select(grid)
    local label = praat.call("Get label of interval", 1, i)
    local a = praat.call("Get start time of interval", 1, i)
    local b = praat.call("Get end time of interval", 1, i)
    if label ~= "" and b > a then
        praat.select(pitch)
        local hz = praat.call("Get quantile", a, b, 0.5, "Hertz")
        local value = hz == hz and string.format("%.3f", hz) or "NA"
        print(label, b-a, value)
    end
end
