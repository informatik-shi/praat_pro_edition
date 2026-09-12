-- Файл: 08-panel.lua
local plt = require('praat.plot')
local ids = praat.call('Create Sound from formula',
    'Lua panel lesson', 1, 0, 2, 22050, '0.2*sin(2*pi*440*x)')
praat.call('View & Edit')
local editor
for _, e in ipairs(plt.editors()) do
    if e.object_id==ids[1] and e.type=='SoundEditor' then editor=e;break end
end
assert(editor, 'Редактор звука не найден')

local fig, ax = plt.subplots()
ax:plot({0,0.5,1,1.5,2}, {0.2,0.8,0.4,0.9,0.3}, {color='#9467bd',linewidth=2})
ax:set_title('Учебный показатель'):set_ylabel('отн. ед.'):set_ylim(0,1)
fig:attach(editor.id, 'panel')
print('Панель прикреплена к редактору', editor.id)
