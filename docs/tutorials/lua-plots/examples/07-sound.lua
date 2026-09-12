-- Файл: 07-sound.lua
local plt = require('praat.plot')
local ids = praat.call('Create Sound from formula',
    'Lua tutorial tone', 1, 0, 0.02, 22050, '0.2*sin(2*pi*440*x)')
praat.select(ids)
local count = praat.call('Get number of samples')
local time, amplitude = {}, {}
for sample=1,count do
    time[sample] = praat.call('Get time from sample number', sample)
    amplitude[sample] = praat.call('Get value at sample number', 1, sample)
end
local fig, ax = plt.subplots(1, 1, {title='Отсчёты объекта Sound'})
ax:plot(time, amplitude, {color='blue', linewidth=2})
ax:set_xlabel('Время, с'):set_ylabel('Амплитуда, Па')
fig:savefig('07-sound.png', 150)
fig:show()
print('Прочитано отсчётов:', count)
