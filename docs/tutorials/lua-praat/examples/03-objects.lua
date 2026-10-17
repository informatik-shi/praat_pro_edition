local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
local spectrum = praat.call("To Spectrum", true)[1]
praat.select(sound)
print("duration_s", praat.call("Get total duration"))
for _, object in ipairs(praat.objects()) do
    print(object.id, object.class, object.name)
end
assert(praat.selected()[1] == sound)
praat.select({sound, spectrum})
assert(#praat.selected() == 2)
praat.select(spectrum)
praat.call("Remove")
praat.select(sound)
