local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
local fs = praat.call("Get sampling frequency")
local n = praat.call("Get number of samples")
local peak, squareSum = 0, 0
for i = 1, n do
    local value = praat.call("Get value at sample number", 1, i)
    peak = math.max(peak, math.abs(value))
    squareSum = squareSum + value * value
end
print("Fs_Hz", fs, "Nyquist_Hz", fs / 2)
print("first_time_s", praat.call("Get time from sample number", 1))
print("peak_Pa", peak, "rms_Pa", math.sqrt(squareSum / n))
assert(math.abs(math.sqrt(squareSum / n) - 0.2/math.sqrt(2)) < 1e-6)
local resampled = praat.call("Resample", 16000, 50)[1]
assert(praat.call("Get number of samples") == 16000)
