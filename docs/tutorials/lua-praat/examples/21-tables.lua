local result = praat.call("Create Table with column names", "measurements",
    3, "token frequency_Hz rms_Pa")[1]
for i, hz in ipairs({100, 200, 400}) do
    praat.select(result)
    praat.call("Set string value", i, "token", "tone_" .. hz)
    praat.call("Set numeric value", i, "frequency_Hz", hz)
    praat.call("Set numeric value", i, "rms_Pa", 0.2/math.sqrt(2))
end
praat.call("Save as tab-separated file", "lesson-table.tsv")
praat.call("Create Strings as tokens", "alpha beta gamma", " ")
local words = praat.call("List all strings")
assert(words[2] == "beta")
print(table.concat(words, ", "))
