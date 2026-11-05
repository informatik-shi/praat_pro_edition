# Praat Pro на Samsung Galaxy Tab S9 FE через Termux

Эта версия — оконное приложение ARM64 Linux, которое работает внутри
Ubuntu 24.04 (proot-distro), а окна выводит через Termux:X11.
Root на планшете не требуется. Это не APK и не бинарный файл для запуска
непосредственно в Android/Bionic. Ветка разработки: `build/termux-tablet`.

## Установка на планшет

1. Установите актуальный Termux из [F-Droid](https://f-droid.org/packages/com.termux/)
   или [официальных выпусков](https://github.com/termux/termux-app/releases).
2. Установите Android-приложение
   [Termux:X11 — termux-x11-universal-debug.apk](https://github.com/termux/termux-x11/releases/tag/nightly).
   Нужны и приложение, и пакет Termux; установщик ниже ставит пакет.
3. Скачайте файлы из артефакта `praat-pro-termux-aarch64` в папку Download:
   `praat-pro-termux-aarch64.tar.gz`, `praat-pro-termux-aarch64.sha256`,
   `install-praat-pro-termux.sh`. Если GitHub скачал ZIP, сначала распакуйте его.
4. В Termux предоставьте доступ к общей памяти и запустите установку:

```sh
termux-setup-storage
cd ~/storage/downloads
sha256sum -c praat-pro-termux-aarch64.sha256
bash install-praat-pro-termux.sh "$PWD/praat-pro-termux-aarch64.tar.gz"
```

Установщик создаёт отдельную Ubuntu с именем `praat-pro`, устанавливает
библиотеки GTK, оконный менеджер и приложение в `/opt/praat-pro`.
Существующая Ubuntu с другим именем не изменяется. Не размещайте исполняемые
файлы в общей памяти Android: там часто запрещён запуск. Архив в Download
используется только для установки во внутреннюю память Termux.

## Запуск

```sh
praat-pro
```

Команда запускает Termux:X11 на дисплее `:1`, открывает его Android-окно,
запускает отдельную D-Bus-сессию и Openbox, если менеджер окон ещё не работает,
затем Praat. Имеющийся менеджер окон не заменяется. Для закрытия используйте
Praat → Quit. Закрытие Android-окна Termux:X11 само по себе не завершает Praat.

Звук идёт через PulseAudio Termux по UNIX-сокету в приватной папке общей
временной директории. Открытый TCP-сервер для звука не создаётся.
Воспроизведение и микрофон требуют отдельной проверки на устройстве;
наличие звуковых библиотек на сервере сборки не проверяет Android-аудио.

Для работы со своими записями используйте `/sdcard/Download` или другую
доступную папку `/storage`. Например, внутри файлового диалога Praat:

```text
/sdcard/Download/recording.wav
```

Для консольного скрипта без окон внутри Ubuntu:

```sh
proot-distro login praat-pro -- /opt/praat-pro/praat-pro --run /sdcard/Download/test.lua
```

## Окна и управление

Начните с альбомной ориентации и разрешения X11 `1280×800` или больше.
В настройках Android-приложения Termux:X11 выберите подходящее разрешение
экрана; не включайте дополнительное масштабирование GTK одновременно с ним.
Для точной разметки удобны мышь, клавиатура или режим тачпада Termux:X11.
Системные касания и S Pen обрабатываются X-сервером как указатель;
отдельный интерфейс пера и распознавание жестов в Praat не добавлены.

Планшетный запуск выставляет `PRAAT_TABLET=1`. В этом режиме GTK:

- ограничивает начальный размер редакторов рабочей областью экрана;
- добавляет горизонтальную и вертикальную прокрутку большим диалогам,
  сохраняя поля и кнопки в исходном размере;
- корректно скрывает и повторно открывает диалоги с прокруткой.

Изменения размера применяются при создании окна. После смены разрешения
уже открытое окно можно уменьшить средствами Openbox или открыть заново.
Для крупного шрифта можно запускать:

```sh
PRAAT_GDK_DPI_SCALE=1.15 praat-pro
```

## Что включено

Sound, TextGrid, обычная спектрограмма, SpectrogramS, SmartSpectrogram,
траектории частот, совместный редактор слоёв, команды SEG/SBL, Lua и API
Praat, LuaFileSystem, Lua-графики. Редакторы Praat и Lua используют GTK.
Функции, реализованные только через Windows RichEdit/WinAPI (расширенное
автодополнение, сворачивание, панели отладчика, тема INI и настраиваемые
сочетания TextGrid), пока не перенесены. Windows-версия от этой ветки не меняется.

## Проверка окон

`scripts/build-termux.sh` собирает приложение на настоящем ARM64 Ubuntu
с базовой архитектурой ARMv8-A, запускает функциональные тесты и два оконных
прогона: `1280×800` и `800×1280`. Xvfb и Openbox открывают реальные GTK-окна.
Проверяются их границы, наличие изображения и сохранение снимков:

- Objects и Picture;
- Sound с анализами;
- Sound + TextGrid с русскими метками;
- Spectrum и Pitch;
- IntensityTier и FormantGrid;
- SpectrogramS и SmartSpectrogram;
- FrequencyTrajectories и совместный просмотр выбранных слоёв;
- редакторы Praat и Lua, окно Lua-графика;
- большой диалог: прокрутка к последним полям, скрытие и повторное открытие.

Это проверка перечисленных окон в Linux/X11, а не проверка всех возможных
объектов Praat на Android. Пока сам планшет не подключён, нельзя подтвердить
корректность касаний, экранной клавиатуры, файловых разрешений, аудио и поведения
Termux:X11 на нём. Снимки и `windows.json` публикуются отдельным артефактом
`termux-window-checks`; успех определяется результатами запуска, а не наличием сценария.

Проверка на планшете после установки:

1. Откройте Sound; проверьте осциллограмму, спектрограмму и масштабирование.
2. Выберите Sound + TextGrid; вставьте границу и русскую метку, сохраните файл.
3. Откройте траектории, SmartSpectrogram и SpectrogramS вместе со звуком.
4. В Praat → New Lua script и New Praat script проверьте ввод с клавиатуры,
   запуск небольшого скрипта, файловые диалоги и переключение между окнами.
5. Откройте длинный диалог настроек, прокрутите до OK/Cancel, закройте и откройте снова.
6. Проверьте воспроизведение, затем повторите работу в другой ориентации экрана.

## Сборка из исходников

В Ubuntu 24.04 ARM64:

```sh
apt-get update
apt-get install -y build-essential pkg-config libgtk-3-dev libasound2-dev libpulse-dev libjack-jackd2-dev xvfb xauth x11-apps x11-utils xdotool openbox wmctrl dbus-x11 librsvg2-common shared-mime-info fonts-dejavu-core python3-pil
JOBS=4 bash scripts/build-termux.sh
```

На самом планшете для экономии памяти уменьшите `JOBS` до 2.
Результат — `dist/praat-pro-termux-aarch64.tar.gz` плюс установщик и SHA-256.
GitHub Actions выполняет ту же сборку в `.github/workflows/termux-build.yml`.

Основание для схемы запуска: [официальная инструкция Termux:X11](https://github.com/termux/termux-x11#using-with-proot-environment)
и [proot-distro](https://github.com/termux/proot-distro#quick-start).
