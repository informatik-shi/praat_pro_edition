# Praat Custom Script IDE

Рабочее название проекта: Praat Custom (папка `praatrus`). Цель — собственная
сборка Praat со встроенным Script IDE и настоящим debugger интерпретатора.
Русификация не входит в текущий этап.

## Исходная точка

- Официальный источник: https://github.com/praat/praat.github.io.git
- Проверен через https://praat.org/download_sources.html.
- Скачан 2026-09-11 полностью, с историей Git (не shallow clone).
- Версия / upstream tag: `v7.0.02`.
- Commit: `6f3da9ef1d8cce0d5684afc104d02888dfc71b25` (2026-08-28).
- Локальная метка исходного состояния: `custom-base-7.0.02`.
- Ветка разработки: `custom/main`; официальный remote: `upstream`.
- Реализован Windows Script IDE; результат: `dist/praat-custom.exe`.
- Исходная сборка сохранена отдельно как `Praat.exe`.
- Оба batch-набора авторов пройдены: 135 + 78 скриптов, код выхода 0.

## Документация

- [Журнал шагов](project-docs/JOURNAL.md).
- [Сборка и проверка Windows](BUILD_WINDOWS.md).
- [Архитектура debugger](docs/debugger-architecture.md).
- [Приёмочные проверки](docs/acceptance.md).
- [Изменения относительно upstream](CHANGES_CUSTOM.md).
- [План разработки](project-docs/ROADMAP.md).
- [Отчёт базовой сборки](project-docs/BASELINE_WINDOWS.md).
- Оригинальные [README](README.md) и [инструкция авторов](HOW_TO_BUILD_ONE.md).

## Работа с Git

```powershell
git status --short --branch
git log -5 --oneline
git diff custom-base-7.0.02 --stat
```

Каждую завершённую задачу фиксируем отдельным коммитом вместе с записью в
журнале: цель, действия, изменённые файлы, проверки и результат. Перед коммитом
проверяем `git diff --check` и содержимое staged diff. Добавляем файлы явно.

Получение обновлений: `git fetch upstream --tags`. Интеграцию новой версии
выполняем отдельной задачей с повторной сборкой и тестами. Публичный fork пока
не создан; `origin` отсутствует. `push.default=nothing` требует явной команды
для отправки. `core.autocrlf=false` установлен только для этого репозитория.
Используется существующая Git-идентичность пользователя.

Исходные сведения об авторстве и лицензиях сохраняются. Лицензирование Praat
и встроенных библиотек описано в разделе 2.1 оригинального README.
