# Praat Pro Edition 7.0.02 — LuaFileSystem preview

Встроенный Lua теперь поддерживает `require("lfs")` без установки LuaRocks
или отдельной DLL. В EXE статически включён официальный LuaFileSystem 1.9.0.

```lua
local lfs = require("lfs")
print(lfs.currentdir())
for name in lfs.dir(".") do print(name) end
```

Эта сборка также содержит ранее добавленные импорт SEG, редактор Lua,
отладчик и графики. Для 64-битной Windows используйте `praat-lfs-x64.exe`;
для 32-битной Windows 7 — `praat-lfs-win7-x86.exe`. Старые EXE из прошлых
выпусков не содержат LuaFileSystem.

На Windows LuaFileSystem 1.9.0 использует системную кодовую страницу для
путей. Имена файлов с символами вне этой кодовой страницы могут не работать.

Лицензия LuaFileSystem включена в ZIP-пакет и исходники. Проверки и сведения
о сборке описаны в `docs/lua.md` и `project-docs/JOURNAL.md`.
