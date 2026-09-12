local function add(x)
    local y = x + 1
    return y
end
local value = 10
value = add(value)
value = value + 2
assert(value == 13)
print("Lua debugger demo:", value)
