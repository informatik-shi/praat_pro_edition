-- Файл: 04-heatmap.lua
local plt = require('praat.plot')
local fig, ax = plt.subplots(1, 1, {title='Учебная тепловая карта'})
local z = {}
for row=1,40 do
    z[row] = {}
    for col=1,60 do
        z[row][col] = math.sin(col/7)*math.cos(row/5)
    end
end
ax:imshow(z, {extent={0, 2, 0, 5000}})
ax:set_title('Синтетические данные')
ax:set_xlabel('Время, с'):set_ylabel('Частота, Гц')
ax:set_xlim(0, 2):set_ylim(0, 5000)
fig:savefig('04-heatmap.png', 150)
fig:show()
