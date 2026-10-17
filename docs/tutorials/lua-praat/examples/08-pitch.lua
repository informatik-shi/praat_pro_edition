local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
local n = praat.call("Get number of frames")
local valid, total = 0, 0
for frame = 1, n do
    local hz = praat.call("Get value in frame", frame, "Hertz")
    if hz == hz and hz > 0 and hz < math.huge then
        valid, total = valid + 1, total + hz
    end
end
assert(valid > 0, "No voiced frames")
local mean = total / valid
print("voiced_frames", valid, "all_frames", n)
print("frame_mean_F0_Hz", mean)
print("median_F0_Hz", praat.call("Get quantile", 0, 0, 0.5, "Hertz"))
assert(math.abs(mean - 200) < 2)
