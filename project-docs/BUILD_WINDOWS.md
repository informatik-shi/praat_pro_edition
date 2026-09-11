# Сборка Windows

Это историческая инструкция baseline. Для текущей кастомной сборки используйте
корневой [BUILD_WINDOWS.md](../BUILD_WINDOWS.md) и `build-windows.ps1`.
Текущие скрипты создают `dist/praat-custom.exe` вместо baseline `Praat.exe`.

Инструкция основана на HOW_TO_BUILD_ONE.md из зафиксированных
исходников. Начальная целевая платформа —
Windows x64, `PRAAT_ARCH=x64v1` для широкой совместимости.

## Подготовка

Установить MSYS2 с https://www.msys2.org/, открыть **CLANG64** shell.
Обновить пакеты командой `pacman -Syu`; если обновление попросит закрыть shell,
перезапустить CLANG64 и повторить обновление. Затем установить инструменты:

```bash
pacman -S --needed make mingw-w64-clang-x86_64-clang mingw-w64-clang-x86_64-pkgconf
```

На этом компьютере MSYS2 установлен в `C:\msys64` 2026-09-11.
Установлены Clang 22.1.8, GNU Make 4.4.1 и pkg-config 3.0.7.

## Базовая сборка

В CLANG64 shell:

```bash
cd /c/Users/PC/Documents/job/praatrus
bash scripts/build-windows.sh
```

Ожидаемый результат — `Praat.exe` в корне. Он, объектные файлы и библиотеки
уже исключены оригинальным .gitignore. В первую очередь проверяем сборку
исходного кода без функциональных изменений. Сохраняем версии инструментов,
команду сборки и результат в JOURNAL.md. Скрипт выполняет
`make PRAAT_ARCH=x64v1 -j4`; число процессов можно задать через `JOBS`.
Логи, версии пакетов и контрольная сумма EXE сохраняются в `.local-build/`.

Из PowerShell:

```powershell
$env:MSYSTEM = 'CLANG64'
& C:\msys64\usr\bin\bash.exe --login -c 'cd /c/Users/PC/Documents/job/praatrus && bash scripts/build-windows.sh'
```

## Автоматические тесты

Из той же CLANG64 shell в корне проекта:

```bash
bash scripts/test-windows.sh
```

Используются `test/runAllTests_batch.praat` и `dwtest/runAllTests_batch.praat`.
Batch-варианты исключают GUI-тесты; первый также исключает ручные и скоростные
тесты. Каждый набор получает отдельный лог в `.local-build/`; сводка —
`test-results.txt`. Код выхода скрипта ненулевой, если любой набор не прошёл.
Предпочтения и плагины пользователя отключены параметрами Praat. Для штатных
наборов передаётся `--FULL-TRUST`: тесты создают и удаляют временные файлы.
Этот параметр не следует переносить на непроверенные сторонние скрипты.

## Проверка приложения

1. Запустить `Praat.exe`, проверить окна Objects и Picture.
2. Записать звук через New → Record mono Sound, открыть View & Edit,
   выделить участок и воспроизвести его.
3. Открыть и выполнить `test/runAllTests.praat`; ожидается график OK.
4. Аналогично выполнить `dwtest/runAllTests.praat`.
5. Записать результаты, ошибки и условия проверки в журнал.

После изменений повторять релевантные проверки; перед выпуском — оба набора
тестов и проверку GUI. Не обозначать неподтверждённые проверки как успешные.
