-- Файл: 02-functions.lua
local plt = require('praat.plot')
local fig, ax = plt.subplots(1, 1, {title='Синус и косинус'})
local t, sine, cosine = {}, {}, {}

for i=1,201 do
    t[i] = (i-1)/100
    sine[i] = math.sin(2*math.pi*t[i])
    cosine[i] = math.cos(2*math.pi*t[i])
end

ax:plot(t, sine, {color='blue', linewidth=2, label='sin'})
ax:plot(t, cosine, {color='red', linewidth=2, label='cos'})
ax:set_xlabel('Время, с'):set_ylabel('Амплитуда')
ax:set_xlim(0, 2):set_ylim(-1.2, 1.2):legend()
fig:savefig('02-functions.png', 150)
fig:show()
