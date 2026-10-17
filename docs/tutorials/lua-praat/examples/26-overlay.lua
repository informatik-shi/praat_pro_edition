local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
local plt = require("praat.plot")
local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
local time, hz = {}, {}
for i = 1, praat.call("Get number of frames") do
    local value = praat.call("Get value in frame", i, "Hertz")
    if value == value and value > 0 and value < math.huge then
        time[#time+1] = praat.call("Get time from frame number", i)
        hz[#hz+1] = value
    end
end
assert(#time > 0, "No valid pitch values")
praat.select(sound)
praat.call("View & Edit")
local editor
for _, e in ipairs(plt.editors()) do
    if e.object_id == sound then editor = e; break end
end
assert(editor, "SoundEditor not found")
local overlay, ax = plt.subplots()
ax:scatter(time, hz, {color="#dd5511", size=1.5})
overlay:attach(editor.id, "spectrogram")
local panel, bx = plt.subplots()
bx:plot(time, hz, {color="#166a73"})
bx:set_ylim(150, 250):set_ylabel("F0 (Hz)")
panel:attach(editor.id, "panel")
