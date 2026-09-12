-- A Lua script can create objects and run the regular Praat analysis commands.
local sound = praat.call("Create Sound from formula",
    "Lua tone", 1, 0, 1, 44100, "0.2*sin(2*pi*440*x)")
print("Sound id:", sound[1])
print("Duration:", praat.call("Get total duration"))
local spectrum = praat.call("To Spectrum", true)
print("Spectrum id:", spectrum[1])
praat.select(sound)
-- Select the Sound in Objects to listen to it or open its editor.
print("Done. Selected:", praat.selected()[1])
