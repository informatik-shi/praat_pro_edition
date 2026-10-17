local plt = require("praat.plot")
local sound = praat.call("Create Sound from formula", "heatmap_chirp",
    1, 0, 0.5, 16000, "0.2*sin(2*pi*(200*x+1800*x*x))")[1]
praat.call("To Spectrogram", 0.01, 4000, 0.005, 50, "Gaussian")
praat.call("To Matrix")
local values = praat.call("Get all values")
for _, row in ipairs(values) do
    for j, power in ipairs(row) do
        row[j] = 10 * math.log(math.max(power, 1e-30), 10)
    end
end
local fig, ax = plt.subplots(1, 1, {title="Spectrogram", width=8, height=4})
ax:imshow(values, {extent={0, 0.5, 0, 4000}})
ax:set_xlabel("Time (s)"):set_ylabel("Frequency (Hz)")
fig:savefig("lesson-spectrogram.png", 150)
