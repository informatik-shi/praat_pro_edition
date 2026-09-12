-- Файл: 05-file.lua
local plt = require('praat.plot')
local file = assert(io.open('measurements.tsv', 'r'))
assert(file:read('l'), 'Пустой файл') -- пропускаем заголовок
local time, frequency = {}, {}
local lineNumber = 1
for line in file:lines() do
    lineNumber = lineNumber + 1
    if line:match('%S') then
        local first, second = line:match('^%s*(%S+)%s+(%S+)%s*$')
        local t, f = tonumber(first), tonumber(second)
        assert(t and f, 'Ошибка в строке '..lineNumber)
        assert(#time==0 or t>time[#time], 'Время должно возрастать')
        time[#time+1], frequency[#frequency+1] = t, f
    end
end
file:close()
assert(#time>0, 'Нет измерений')

local fig, ax = plt.subplots(1, 1, {title='Измерения из файла'})
ax:plot(time, frequency, {color='blue', label='Траектория'})
ax:scatter(time, frequency, {color='red', size=2})
ax:set_xlabel('Время, с'):set_ylabel('Частота, Гц'):legend()
fig:savefig('05-file.png', 150)
fig:show()
