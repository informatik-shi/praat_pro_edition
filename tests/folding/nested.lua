-- A visible result must include the statements in folded blocks.
local function total()
    local sum = 0
    for i = 1, 3 do
        sum = sum + i
    end
    return sum
end
assert(total() == 6)
print("FOLDED LUA: PASS", total())
