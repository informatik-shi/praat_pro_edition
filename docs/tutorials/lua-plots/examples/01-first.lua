-- Файл: 01-first.lua
local plt = require('praat.plot')
local fig, ax = plt.subplots(1, 1, {title='Первый график'})

local time = {0, 0.5, 1.0, 1.5, 2.0}
local value = {1, 3, 2, 4, 3}
ax:plot(time, value, {color='#1f77b4', linewidth=2, label='Измерения'})
ax:set_title('Изменение показателя')
ax:set_xlabel('Время, с')
ax:set_ylabel('Значение')
ax:set_ylim(0, 5)
ax:legend()

fig:savefig('01-first.png', 150)
fig:show()
