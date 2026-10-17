local grid = praat.call("Create KlattGrid from vowel",
    "lesson_vowel", 1, 125, 700, 60, 1200, 90,
    2500, 150, 3500, 0.05, 1000)[1]
local sound = praat.call("To Sound")[1]
local plt = require("praat.plot")
praat.call("Save as WAV file", "project-vowel.wav")
local annotation = praat.call("To TextGrid", "segments", "")[1]
praat.call("Insert boundary", 1, 0.33)
praat.call("Insert boundary", 1, 0.66)
for i, label in ipairs({"early", "middle", "late"}) do
    praat.call("Set interval text", 1, i, label)
end
praat.call("Save as text file", "project-vowel.TextGrid")
praat.select(sound)
local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
praat.select(sound)
local formant = praat.call("To Formant (burg)", 0.005, 5, 5500, 0.025, 50)[1]
local function finite(v) return v == v and math.abs(v) < math.huge end
local function value(v) return finite(v) and string.format("%.4f",v) or "NA" end
local output = assert(io.open("project-measurements.tsv", "w"))
output:write("label\tstart_s\tend_s\tduration_s\tmedian_F0_Hz\tF1_mid_Hz\tF2_mid_Hz\n")
local times, f1s, f2s = {}, {}, {}
praat.select(annotation)
local n = praat.call("Get number of intervals", 1)
for i = 1, n do
    praat.select(annotation)
    local label = praat.call("Get label of interval", 1, i)
    local a = praat.call("Get start time of interval", 1, i)
    local b = praat.call("Get end time of interval", 1, i)
    if label ~= "" and b > a then
        local midpoint = (a+b)/2
        praat.select(pitch)
        local f0 = praat.call("Get quantile", a, b, 0.5, "Hertz")
        praat.select(formant)
        local f1 = praat.call("Get value at time", 1, midpoint, "Hertz", "linear")
        local f2 = praat.call("Get value at time", 2, midpoint, "Hertz", "linear")
        output:write(table.concat({label, value(a), value(b), value(b-a),
            value(f0), value(f1), value(f2)}, "\t"), "\n")
        if finite(f1) and finite(f2) then
            times[#times+1], f1s[#f1s+1], f2s[#f2s+1] = midpoint, f1, f2
        end
    end
end
output:close()
assert(#times > 0, "No finite formants for the figure")
local fig, ax = plt.subplots(1, 1, {title="Interval midpoint formants", width=8, height=4})
ax:scatter(times, f1s, {label="F1", color="#166a73", size=2})
ax:scatter(times, f2s, {label="F2", color="#bd5b2e", size=2})
ax:set_xlabel("Time (s)"):set_ylabel("Frequency (Hz)"):legend()
fig:savefig("project-formants.png", 150)
local settings = assert(io.open("project-settings.txt", "w"))
settings:write("Praat Custom; embedded Lua 5.5.1\n",
    "Pitch: dt=0.01 s; floor=75 Hz; ceiling=500 Hz\n",
    "Formant burg: dt=0.005 s; max=5; ceiling=5500 Hz; window=0.025 s; preemphasis=50 Hz\n",
    "Tier=1; exclude empty labels; missing=NA; F0=interval median; F1/F2=midpoint\n")
settings:close()
praat.select(sound)
print("project_complete", n)
