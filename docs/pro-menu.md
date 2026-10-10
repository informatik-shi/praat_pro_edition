# Меню Pro

В окне Praat Objects меню **Pro** находится сразу правее **Save**.

- **Read seg files...** создаёт TextGrid из группы файлов `*.seg_*`.
  Выбор уровней, кодировки и примеры описаны в [руководстве SEG](read-seg-files.md).
- **Read sbl file...** читает файл как необработанный монофонический звук:
  последовательность знаковых 16-битных отсчётов с порядком байтов little-endian.
  Заголовок файла не ожидается. Частота дискретизации полученного Sound —
  **22050 Гц**, без передискретизации самих отсчётов. Неполная пара байтов в
  конце файла игнорируется штатным загрузчиком Praat.

Пример Praat Script:

```praat
Read sbl file: "C:/data/example.sbl"
frequency = Get sampling frequency
assert frequency = 22050
```

В пакет сборки включён маленький `examples/sample.sbl` для проверки импорта.

Пример Lua:

```lua
local sound = praat.call("Read sbl file", "C:/data/example.sbl")
praat.select(sound)
assert(praat.call("Get sampling frequency") == 22050)
```
