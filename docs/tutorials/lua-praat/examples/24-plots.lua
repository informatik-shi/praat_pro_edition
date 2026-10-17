local plt = require("praat.plot")
local sound = praat.call("Create Sound from formula", "plot_tone",
    1, 0, 0.02, 16000, "0.2*sin(2*pi*200*x)")[1]
local time, amplitude = {}, {}
for i = 1, praat.call("Get number of samples") do
    time[i] = praat.call("Get time from sample number", i)
    amplitude[i] = praat.call("Get value at sample number", 1, i)
end
local fig, ax = plt.subplots(1, 1, {title="Waveform", width=8, height=4})
ax:plot(time, amplitude, {color="#166a73", linewidth=2, label="200 Hz"})
ax:set_xlabel("Time (s)"):set_ylabel("Amplitude (Pa)"):legend()
fig:savefig("lesson-waveform.png", 150)
-- fig:show() -- Uncomment when running in the graphical application.
