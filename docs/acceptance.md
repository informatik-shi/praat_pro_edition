# Приёмка Praat Custom Script IDE

Дата: 2026-09-11. Windows x64, MSYS2 CLANG64, Clang 22.1.8.
Upstream Praat 7.0.02. Это работающая первая версия встроенного IDE.

## Сборка

Команда: `./build-windows.ps1 -Jobs 4 -Test`.
Сборка, integration runner и штатные suites завершились с кодом 0.
После финальных поправок только GUI (gutter/Go To Line) повторно выполнена
`./build-windows.ps1 -Jobs 4`, затем native smoke из `dist/praat-custom.exe`.
Полные численные suites после этих исключительно GUI-поправок не повторялись.

SHA-256 окончательного EXE:
`36a6ab7cb1771cda878f992a36acb8c2177059382093012790915a091b8daae5`.
Путь: `C:\Users\PC\Documents\job\praatrus\dist\praat-custom.exe`.
Проверка PE imports: Windows/UCRT DLL, без внешней DLL C++ runtime;
RichEdit загружается из системной Msftedit.dll. Отдельная чистая VM Windows 10
не использовалась: запуск проверен на текущем Windows-компьютере.

## Функции

| Возможность | Результат | Доказательство |
|---|---|---|
| Номера строк и прокрутка | YES | GUI: строки 11993–12002 большого файла совпадают с кодом |
| Syntax highlighting | YES | GUI + тест lexer; строки, числа, типы, comments, procedures |
| Ln/Col | YES | GUI: курсор и переходы обновляют status |
| Breakpoint клик / удаление / F9 | YES | GUI: повторный клик удаляет; F9 на 12001 |
| Сохранение breakpoints | YES | повторный запуск восстановил точку на строке 2 |
| Привязка при редактировании | YES | вставка перед 12001 переносит точку на 12002; Undo возвращает |
| F5 Start / Continue | YES | GUI и integration runner |
| Shift+F5 Stop | YES | GUI остаётся открыт; runner останавливает бесконечный цикл |
| F10 Step Over | YES | строка 2 → 3, вызов процедуры выполнен целиком |
| F11 Step Into | YES | строка 2 → calculate:6; вложенный normalize:10 в runner |
| Shift+F11 Step Out | YES | calculate → main:3; обе вложенные глубины в runner |
| Variables | YES | реальные numeric/string/vector/matrix/string-vector значения |
| Watch | YES | GUI `.value * 2 = 8`; runner x*2, length, exponent; stateful запреты |
| Call Stack | YES | GUI main:2 / calculate:6; runner проверяет normalize frame |
| Execution line | YES | жёлтый фон, стрелка, переходы на реальную строку |
| Runtime error | YES | fixture: файл, main, строка 3, исходное сообщение и навигация |
| Ctrl+G | YES | штатный диалог и выбор строки 6000 большого файла |
| Большие файлы | YES | 12000 команд, F9, вставка, Undo, Save без заметного зависания |
| Сохранение чистого текста | YES | после Save: 12001 содержательная строка, последняя value = 12000 |
| Windows executable | YES | собран и запущен непосредственно из dist |

Скорость GUI оценивалась по фактической отзывчивости, без измерения latency
каждого нажатия. RichEdit хранит только текст; номера и точки — отдельное окно.

## Автоматические результаты

- `.local-build/debugger-tests.log`: DEBUGGER INTEGRATION TESTS: PASS.
- `.local-build/test-results.txt`: test PASS, dwtest PASS, оба exit 0.
- В последнем `test.log` — 138 записей `### executing`, в `dwtest.log` — 78.
  Это число записей выполнения, а не утверждение о числе уникальных тестов.
- Базовая GUI-проверка Praat до изменений: Objects/Picture, создание тона,
  Sound Editor, волна/спектрограмма, выделение и команда воспроизведения.
- Полная сохранность всех функций Praat не доказана исчерпывающе: пройдены
  штатные suites, код обработки звука не изменялся. Микрофон и слышимость
  воспроизведения человеком не проверялись.

## Ограничения

- Include перед debug нужно раскрыть через Convert > Expand include files;
  обычный Run не имеет этого ограничения. Нет source map нескольких файлов.
- Step Into для процедур текущего скрипта; вложенный runScript — атомарный вызов.
- Stop обслуживается между инструкциями; отдельная долгая C++ команда может
  удерживать GUI. Асинхронные pause/demo-сценарии debugger не проверены.
- Пока скрипт активен, редактирование и остальные команды Praat отключены.
- Variables показывает ограниченные превью; отдельного табличного inspector нет.
- Watch — разрешённое read-only подмножество Formula, без сохранения между окнами.
- Процедурные locals отражают штатную модель Praat, включая разделение storage
  рекурсивными вызовами; произвольный выбор frame пока отсутствует.
- Светлая тема и фиксированная нижняя панель; dark mode не реализован.
- Ln/Col считает UTF-16 units; раскраска одной строки ограничена 65534 units.
- При отмене формы параметров нажмите Stop для сброса ожидающей debug-сессии.

Детали: `debugger-architecture.md`; ручные сценарии: `../tests/debugger/README.md`.
