-- Файл: 09-overlay.lua
local plt = require('praat.plot')
local ids = praat.call('Create Sound from formula',
    'Lua overlay lesson', 1, 0, 2, 22050,
    '0.15*sin(2*pi*440*x)+0.1*sin(2*pi*(1000*x+250*x^2))')
praat.call('View & Edit')
local editor
for _, e in ipairs(plt.editors()) do
    if e.object_id==ids[1] and e.type=='SoundEditor' then editor=e;break end
end
assert(editor)

local fig, ax = plt.subplots()
local time, frequency = {}, {}
for i=1,101 do
    time[i] = (i-1)/50
    frequency[i] = 1000+500*time[i]
end
ax:plot(time, frequency, {color='#ff3300', linewidth=3})
ax:plot({0,2}, {440,440}, {color='#00bbff', linewidth=2})
fig:attach(editor.id, 'spectrogram')
