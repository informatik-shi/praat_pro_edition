local original = praat.call("Create Sound from formula", "two_tones",
    1, 0, 1, 16000, "0.2*sin(2*pi*200*x)+0.2*sin(2*pi*2000*x)")[1]
local before = praat.call("Get root-mean-square", 0, 0)
local filtered = praat.call("Filter (pass Hann band)", 100, 500, 50)[1]
local after = praat.call("Get root-mean-square", 0, 0)
print("RMS_before_after", before, after)
assert(after < before)
praat.call("Save as WAV file", "lesson-filtered.wav")
local impulse = praat.call("Create Sound from formula", "impulse",
    1, 0, 0.1, 16000, "if col=1 then 1 else 0 fi")[1]
local response = praat.call("Filter (one formant)", 800, 80)[1]
praat.call("Save as WAV file", "lesson-response.wav")
