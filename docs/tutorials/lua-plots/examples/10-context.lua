-- Файл: 10-context.lua
local plt = require('praat.plot')
local selected = praat.selected()
assert(#selected==1, 'Выберите один открытый объект в Objects')
local found = false
for _, e in ipairs(plt.editors()) do
    if e.object_id==selected[1] then
        found = true
        print('Редактор:', e.id, e.type, e.name)
        print('Видимый диапазон:', e.start, e.finish)
        print('Выделение:', e.selection_start, e.selection_end)
        print('Есть панель/слой:', e.has_panel, e.has_overlay)
        -- Раскомментируйте, чтобы удалить только слой спектрограммы:
        -- plt.detach(e.id, 'spectrogram')
    end
end
assert(found, 'У выбранного объекта нет поддерживаемого открытого редактора')
