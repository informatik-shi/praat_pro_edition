local lfs = require("lfs")
local folder = (arg and arg[1]) or "corpus"
assert(lfs.attributes(folder, "mode") == "directory", "Create the corpus folder first")
local files = {}
for name in lfs.dir(folder) do
    if name:lower():match("%.wav$") and
       lfs.attributes(folder .. "/" .. name, "mode") == "file" then
        files[#files+1] = name
    end
end
table.sort(files)
assert(#files > 0, "No WAV files found")
local function cell(value)
    return tostring(value):gsub("[\t\r\n]", " ")
end
local function number(value)
    if value == value and math.abs(value) < math.huge then
        return string.format("%.6f", value)
    end
    return "NA"
end
local output = assert(io.open("lesson-batch.tsv", "w"))
output:write("file\tduration_s\tmedian_F0_Hz\tstatus\n")
for _, name in ipairs(files) do
    local before, previous = {}, praat.selected()
    for _, o in ipairs(praat.objects()) do before[o.id] = true end
    local ok, duration, median = pcall(function()
        praat.call("Read from file", folder .. "/" .. name)
        local seconds = praat.call("Get total duration")
        praat.call("To Pitch", 0.01, 75, 500)
        return seconds, praat.call("Get quantile", 0, 0, 0.5, "Hertz")
    end)
    local temporary = {}
    for _, o in ipairs(praat.objects()) do
        if not before[o.id] then temporary[#temporary+1] = o.id end
    end
    if #temporary > 0 then praat.select(temporary); praat.call("Remove") end
    praat.select(previous)
    if not ok and tostring(duration):find("Lua execution stopped", 1, true) then
        output:close()
        error(duration, 0)
    end
    if ok then
        output:write(cell(name), "\t", number(duration), "\t", number(median), "\tOK\n")
    else
        output:write(cell(name), "\tNA\tNA\tERROR\n")
        print("ERROR", name, duration)
    end
    output:flush()
end
output:close()
print("files_processed", #files)
