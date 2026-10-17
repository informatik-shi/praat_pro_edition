local sound = praat.call("Create Sound from formula", "mixture",
    1, 0, 1, 16000, "0.2*sin(2*pi*200*x)+0.1*sin(2*pi*600*x)")[1]
local spectrum = praat.call("To Spectrum", true)[1]
local n = praat.call("Get number of bins")
local bestHz, bestMagnitude = 0, -1
for bin = 1, n do
    local re = praat.call("Get real value in bin", bin)
    local im = praat.call("Get imaginary value in bin", bin)
    local magnitude = math.sqrt(re*re + im*im)
    if magnitude > bestMagnitude then
        bestMagnitude = magnitude
        bestHz = praat.call("Get frequency from bin number", bin)
    end
end
print("largest_bin_Hz", bestHz)
assert(math.abs(bestHz - 200) < 2)
