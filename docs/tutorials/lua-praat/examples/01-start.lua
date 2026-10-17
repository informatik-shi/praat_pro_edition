local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
print(_VERSION)
print("duration_s", praat.call("Get total duration"))
print("samples", praat.call("Get number of samples"))
print("object_id", sound)
