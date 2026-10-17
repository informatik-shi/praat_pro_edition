local before, previous = {}, praat.selected()
for _, object in ipairs(praat.objects()) do before[object.id] = true end
local ok, message = pcall(function()
    praat.call("Create Sound from formula", "temporary", 1, 0, 1, 16000, "0")
    praat.call("This command deliberately does not exist")
end)
local created = {}
for _, object in ipairs(praat.objects()) do
    if not before[object.id] then created[#created+1] = object.id end
end
if #created > 0 then praat.select(created); praat.call("Remove") end
praat.select(previous)
assert(not ok)
print("handled_error", message)
