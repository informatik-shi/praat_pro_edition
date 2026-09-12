-- Файл: 06-update.lua
local plt = require('praat.plot')
local fig, ax = plt.subplots(1, 1, {title='Сравнение версий', width=8, height=5})
ax:plot({0,1,2}, {1,2,3})
ax:set_title('До изменения')
fig:savefig('06-before.png', 150)

ax:clear()
ax:plot({0,1,2}, {3,1,4}, {color='orange', linewidth=2})
ax:set_title('После изменения'):set_xlim(0,2):set_ylim(0,5)
fig:savefig('06-after.png', 150)
fig:show()
