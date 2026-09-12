-- Файл: 03-comparison.lua
local plt = require('praat.plot')
local fig, axes = plt.subplots(1, 2, {title='Два представления', width=10, height=5})
local x = {1, 2, 3, 4, 5}
local y = {4, 7, 3, 8, 6}

axes[1]:scatter(x, y, {color='#2ca02c', size=2})
axes[1]:set_title('Наблюдения'):set_xlabel('Номер'):set_ylabel('Значение')
axes[1]:set_ylim(0, 10)

axes[2]:bar(x, y, {color='#9467bd', width=0.6})
axes[2]:set_title('Группы'):set_xlabel('Группа'):set_ylabel('Значение')
axes[2]:set_ylim(0, 10)
fig:savefig('03-comparison.png', 150)
fig:show()
