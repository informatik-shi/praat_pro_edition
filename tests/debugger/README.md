# Проверка debugger

Автоматически (MSYS2 CLANG64): `bash scripts/test-debugger.sh`.
Полная сборка и проверки: `./build-windows.ps1 -Jobs 4 -Test` из PowerShell.
Логи находятся в `.local-build/debugger-tests.log`, `test.log`, `dwtest.log`.

`runner.cpp` запускает реальный InterpreterStack, без GUI-имитации.

| Fixture | Проверка |
|---|---|
| test_variables.praat | number/string/vector/matrix/string vector, Watch x*2, length(name$), запрет stateful функций |
| test_breakpoint.praat | 16 последовательных строк для ручных breakpoint |
| test_loop.praat | повторные попадания в for, while, repeat |
| test_if.praat | остановка только в достигнутой ветви |
| test_procedure.praat | Over, Into, Out, вложенные процедуры, реальный stack |
| test_error.praat | ожидаемая ошибка строки 3 |
| test_stop.praat | бесконечный цикл, кооперативный Stop |
| test_continuation.praat | физические номера continuation-строк |
| test_unicode.praat | кириллица и символ вне BMP |

Ручной сценарий: откройте test_procedure.praat, кликните gutter строки 2,
нажмите F5. x ещё равен 1. F11 остановит на строке 6; Call Stack покажет
main:2 и calculate:6. Watch `.value * 2` равен 8. F10 — строка 7,
F11 — normalize:10; Shift+F11 — calculate:8, повторно — main:3.
F5 завершит скрипт. При новом запуске F10 со строки 2 сразу перейдёт к 3.
Shift+F5 останавливает скрипт, сохраняя работающий Praat.

Откройте test_error.praat, F5: ожидаются Error, строка 3 и исходное сообщение
об unknown variable. Ошибка этого fixture намеренная, не регрессия сборки.

Для проверки редактирования используйте копии fixtures: F9, вставка строк
выше breakpoint, Undo/Redo, Save, повторное открытие. Текст gutter не должен
попасть в сохранённый файл. Для большого файла создайте 12000 строк `x = 1`,
проверьте Ctrl+End, PageUp/PageDown, Ctrl+G и редактирование конца файла.
