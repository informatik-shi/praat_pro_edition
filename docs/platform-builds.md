# Сборки Praat Pro для Linux и macOS

Собираются изменённые исходники этой версии, включая Lua, SEG/SBL,
FrequencyTrajectories, SmartSpectrogram, SpectrogramS и общие редакторы.
Готовая официальная программа Praat не подменяет кастомную сборку.

## Отличия интерфейса от Windows

Пока только Windows поддерживает нашу палитру INI и тёмное оформление,
RichEdit-подсветку/автодополнение/сворачивание, панели отладки Praat и Lua,
общее окно Stop и настройку горячих клавиш TextGrid из INI. Эти части
исходников закрыты условием `_WIN32`. В Linux/macOS используются штатные
GTK/Cocoa-редакторы; Lua Editor, выполнение Lua и API команд доступны,
но расширенные Windows-панели не появляются. Скрипты расчёта, собственные
объекты и Lua-визуализация включены в обе платформенные сборки.

## Linux x86-64

Основная сборка: Ubuntu 22.04, базовая архитектура x86-64 без требования AVX2,
GTK 3, ALSA/PulseAudio/JACK. Требуется glibc 2.35 или новее и библиотеки,
перечисленные в DEPENDENCIES.txt внутри архива. Другие дистрибутивы могут
потребовать установку соответствующих пакетов или пересборку из исходников.

В чистом Linux checkout:

```sh
sudo apt-get update
sudo apt-get install build-essential pkg-config libgtk-3-dev libasound2-dev \
  libpulse-dev libjack-jackd2-dev xvfb xauth librsvg2-common shared-mime-info fonts-dejavu-core
JOBS=4 bash scripts/build-linux.sh
```

Результат: `dist/praat-pro-linux-x86_64.tar.gz`. Распакуйте архив, перейдите
в папку `praat-pro` и запустите `./praat-pro`. Для командной строки:

```sh
./praat-pro --run my-analysis.praat
./praat-pro my-analysis.lua
```

Сценарий запускает проверки SpectrogramS, SmartSpectrogram, траекторий,
SEG, Lua/API/CLI и Lua-графиков. Логи находятся в `.local-build`.
Запускайте сборку из отдельной копии исходников: Windows `.o`/`.a` нельзя
использовать для Linux. `scripts/platforms/source-snapshot.py` архивирует
текущее рабочее дерево вместе с незакоммиченными исходниками, исключая
сгенерированные объекты, EXE, `.git`, `dist` и локальные кэши.

На Windows можно использовать отдельную WSL Ubuntu. В текущей машине
создана среда `PraatBuildLinux`; её диск находится под `.local-build/platforms`.
Она не использует Docker Desktop. Исходники и Linux-объекты собираются
внутри Linux-диска, не поверх Windows-объектов.

## macOS Intel + Apple Silicon

Требуются Mac и Xcode с установленными Command Line Tools. Проект базируется
на официальном `praat7002_xcodeproj.zip` версии 7.0.02; скрипт проверяет SHA-256,
добавляет кастомные исходники и Lua как C++ (для корректного разворачивания
исключений и RAII), а GUI-исходники — как Objective-C++.

```sh
JOBS=4 bash scripts/build-macos.sh
```

Сборка создаёт universal-приложение с `arm64` и `x86_64`, проверяет наличие
обеих архитектур, запускает функциональные тесты на архитектуре Mac,
подписывает приложение локальной ad-hoc подписью и создаёт:

- `dist/praat-pro-macos-universal.zip`;
- `dist/praat-pro-macos-universal.dmg`.

Целевой минимум — macOS 10.15. Локальная подпись не является Developer ID
и не включает нотариализацию Apple. Для публичного нотариализованного
релиза нужны учётная запись разработчика и сертификат владельца сборки.
Сценарий не использует сертификат или команду разработчиков оригинального Praat.

## GitHub Actions

`.github/workflows/platform-builds.yml` запускается при отправке ветки
`build/macos-linux`, а после регистрации задания также вручную через
**Actions → Praat Pro macOS and Linux packages → Run workflow**. Он собирает
Ubuntu x86-64 и macOS universal, запускает тесты и сохраняет пакеты и логи
как артефакты. Можно выбрать `all`, `linux` или `macos` для отдельной
сборки платформы. Автоматической публикации релиза, отправки коммитов или
изменения ветки `develop` в задании нет.

GitHub собирает только код из выбранной ветки. Перед запуском необходимо
отправить именно текущие кастомные исходники и новые файлы сборки; локальные
незакоммиченные изменения на удалённом сервере не видны.

Официальные справочные материалы:
[сборка Praat](https://github.com/praat/praat/blob/master/HOW_TO_BUILD_ONE.md),
[импорт среды WSL](https://learn.microsoft.com/windows/wsl/use-custom-distro),
[Ubuntu Base](https://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/).
