local frequencies = {100, 200, 400}
local sum = 0
for i, hz in ipairs(frequencies) do
    sum = sum + hz
    local label = hz >= 200 and "high" or "low"
    print(i, hz, label)
end
local function period(hz)
    assert(hz > 0, "Frequency must be positive")
    return 1 / hz
end
print("sum", sum)
print("period_s", period(200))
print("pi", math.pi, "sin(pi/2)", math.sin(math.pi / 2))
local n = 0
repeat n = n + 1 until n == 3
while n > 0 do n = n - 1 end
assert(n == 0)
