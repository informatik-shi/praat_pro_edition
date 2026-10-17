"""Build two self-contained Lua/Praat textbooks and their runnable examples."""
from pathlib import Path
from html import escape
import base64
import json
import textwrap
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/tutorials/lua-praat'
LESSONS = []

def para(text):
    return '<p>' + text + '</p>'

def note(text):
    return '<aside class="note">' + text + '</aside>'

def steps(items):
    return '<ol>' + ''.join('<li>' + s + '</li>' for s in items) + '</ol>'

def table(headers, rows):
    return '<div class="table-wrap"><table><thead><tr>' + ''.join('<th>' + s + '</th>' for s in headers) + '</tr></thead><tbody>' + ''.join('<tr>' + ''.join('<td>' + s + '</td>' for s in row) + '</tr>' for row in rows) + '</tbody></table></div>'

def add(slug, titles, source, intro, code='', after=('', ''), tasks=('', ''), answers=('', ''), mode='batch', filename=None):
    LESSONS.append(dict(slug=slug, titles=titles, source=source, intro=intro,
                        code=textwrap.dedent(code).strip() + '\n' if code else '',
                        after=after, tasks=tasks, answers=answers, mode=mode,
                        filename=filename or slug + '.lua'))

TONE = '''local sound = praat.call("Create Sound from formula",
    "lesson_tone", 1, 0, 1, 44100, "0.2*sin(2*pi*200*x)")[1]
'''
VOWEL = '''local grid = praat.call("Create KlattGrid from vowel",
    "lesson_vowel", 1, 125, 700, 60, 1200, 90,
    2500, 150, 3500, 0.05, 1000)[1]
local sound = praat.call("To Sound")[1]
'''

add('01-start', ('Первый запуск: звук за минуту', 'First run: a sound in one minute'), '1, 2, 4', (
    para('Цель: создать звук, получить длительность и понять, где выполняется Lua. Ваша сборка уже содержит интерпретатор; отдельные Lua, Python и DLL для этих уроков не нужны. Откройте окно <b>Objects → Praat → New Lua script</b>. Это меню Praat в окне Objects. Вставьте код и сначала сохраните его как <code>01-start.lua</code> в отдельной папке курса.') + steps(['Нажмите <b>F7</b>: проверка синтаксиса не создаёт объектов.', 'Нажмите <b>Ctrl+R</b>: выполнится весь файл. Откройте окно Info, если оно скрыто.', 'В Objects появится Sound lesson_tone. Для прослушивания выделите его и нажмите Play. Для просмотра нажмите View &amp; Edit.', 'Запустите ещё раз: появится новый объект. Это нормальное поведение; имена могут совпадать.']),
    para('Goal: create a sound, query its duration, and learn where Lua runs. Your build already embeds the interpreter; these lessons need no separate Lua, Python, or DLL installation. In the <b>Objects window, choose Praat → New Lua script</b>. Paste the code and save it as <code>01-start.lua</code> in a dedicated course folder.') + steps(['Press <b>F7</b>: syntax checking creates no objects.', 'Press <b>Ctrl+R</b> to run the whole file. Open Info if it is hidden.', 'Objects now contains Sound lesson_tone. Select it and press Play to listen, or View &amp; Edit to inspect it.', 'Run again: another object appears. This is expected; names need not be unique.'])),
    TONE + '''print(_VERSION)
print("duration_s", praat.call("Get total duration"))
print("samples", praat.call("Get number of samples"))
print("object_id", sound)
''', (
    para('Проверка: Info показывает <code>Lua 5.5</code>, длительность <code>1</code>, <code>44100</code> отсчётов и положительный ID. Полная встроенная версия библиотеки — 5.5.1; <code>_VERSION</code> выводит только 5.5. Кнопка F5 запускает с отладчиком; для первого знакомства удобнее Ctrl+R.'),
    para('Check: Info reports <code>Lua 5.5</code>, duration <code>1</code>, <code>44100</code> samples, and a positive object ID. The embedded library is 5.5.1; <code>_VERSION</code> only reports 5.5. F5 runs with the debugger; Ctrl+R is convenient for this first lesson.')),
    ('Измените длительность на 0.5 с и частоту на 400 Гц. Сколько получится отсчётов?', 'Change the duration to 0.5 s and the frequency to 400 Hz. How many samples should you get?'),
    ('22050 при частоте дискретизации 44100 Гц. Частота тона меняет число колебаний, но не число отсчётов при неизменных длительности и дискретизации.', '22050 at a sample rate of 44100 Hz. Tone frequency changes the number of cycles, but sample count depends on duration and sample rate.'))

add('02-lua', ('Минимум языка Lua', 'Essential Lua'), '4; A', (
    para('Lua выполняет обычную программную работу: хранит параметры, выбирает ветви, повторяет действия и собирает результаты. Анализ сигнала выполняют команды Praat. Используйте <code>local</code> для переменных. Числа записывают с точкой; строки — в прямых кавычках. Комментарий начинается с <code>--</code>.') + table(['Запись', 'Смысл'], [['<code>== / ~=</code>', 'равно / не равно'], ['<code>..</code>', 'склеить строки'], ['<code>^</code>', 'возведение в степень'], ['<code>#values</code>', 'длина плотной последовательности'], ['<code>and / or / not</code>', 'логические операции']]) + para('Только <code>false</code> и <code>nil</code> считаются ложью: число 0 и пустая строка истинны. Нумерация последовательностей в этом курсе начинается с 1; отрицательный индекс не означает «с конца». <code>ipairs</code> идёт до первого отсутствующего элемента.'),
    para('Lua handles parameters, branches, repetition, and result collection. Praat commands perform signal analysis. Prefer <code>local</code> variables. Use a decimal point in numbers and straight quotation marks in strings. A comment starts with <code>--</code>.') + table(['Syntax', 'Meaning'], [['<code>== / ~=</code>', 'equal / not equal'], ['<code>..</code>', 'concatenate strings'], ['<code>^</code>', 'exponentiation'], ['<code>#values</code>', 'length of a dense sequence'], ['<code>and / or / not</code>', 'logical operations']]) + para('Only <code>false</code> and <code>nil</code> are false: zero and an empty string are true. Sequences in this course start at 1; a negative index does not mean “from the end”. <code>ipairs</code> stops at the first missing element.')),
    '''local frequencies = {100, 200, 400}
local sum = 0
for i, hz in ipairs(frequencies) do
    sum = sum + hz
    local label = hz >= 200 and "high" or "low"
    print(i, hz, label)
end
local function period(hz)
    assert(hz > 0, "Frequency must be positive")
    return 1 / hz
end
print("sum", sum)
print("period_s", period(200))
print("pi", math.pi, "sin(pi/2)", math.sin(math.pi / 2))
local n = 0
repeat n = n + 1 until n == 3
while n > 0 do n = n - 1 end
assert(n == 0)
''', (
    para('Результат: сумма 700, период 0.005 с, синус около 1. <code>local function</code> создаёт функцию; <code>return</code> возвращает значение. Таблица может быть последовательностью или набором полей, например <code>{floor=75, ceiling=500}</code>. <code>pairs</code> перебирает поля без гарантированного порядка. <code>break</code> выходит из цикла. Не применяйте <code>#</code> для подсчёта произвольных полей.'),
    para('Result: sum 700, period 0.005 s, sine approximately 1. <code>local function</code> defines a function; <code>return</code> returns a value. A table can be a sequence or a set of named fields such as <code>{floor=75, ceiling=500}</code>. <code>pairs</code> visits fields in unspecified order. <code>break</code> exits a loop. Do not use <code>#</code> to count arbitrary named fields.')),
    ('Добавьте 800 Гц в таблицу и выведите среднюю частоту.', 'Add 800 Hz to the table and print the mean frequency.'),
    ('<code>frequencies[#frequencies+1] = 800</code>; пересчитайте сумму в цикле и разделите на <code>#frequencies</code>. Получится 375 Гц.', '<code>frequencies[#frequencies+1] = 800</code>; recompute the sum in a loop and divide by <code>#frequencies</code>. The result is 375 Hz.'))

add('03-objects', ('Объекты, выделение и типы результатов', 'Objects, selection, and return types'), '1.5; 4.8', (
    para('Команда зависит от текущего выделения Objects. В Lua объект представлен числовым ID, а не самим звуковым массивом. Команды создания возвращают <b>таблицу ID выбранных после команды объектов</b>. Для команды, создающей один объект, берите <code>[1]</code>. Не путайте ID с номером строки в списке или индексом Lua.'),
    para('A command depends on the current Objects selection. Lua represents a Praat object by a numeric ID, not by its sample array. Creation commands return a <b>table of IDs selected after the command</b>. For a single-object creation, take <code>[1]</code>. An ID is neither a list-row number nor a Lua index.')),
    TONE + '''local spectrum = praat.call("To Spectrum", true)[1]
praat.select(sound)
print("duration_s", praat.call("Get total duration"))
for _, object in ipairs(praat.objects()) do
    print(object.id, object.class, object.name)
end
assert(praat.selected()[1] == sound)
praat.select({sound, spectrum})
assert(#praat.selected() == 2)
praat.select(spectrum)
praat.call("Remove")
praat.select(sound)
''', (
    table(['API', 'Результат / действие'], [['<code>praat.call(name, ...)</code>', 'число, boolean, строка, таблица или отсутствие результата'], ['<code>praat.select(id, ...)</code>', 'заменяет выделение; принимает также таблицу ID'], ['<code>praat.select({})</code>', 'снимает выделение'], ['<code>praat.selected()</code>', 'таблица выделенных ID'], ['<code>praat.objects()</code>', 'все объекты: <code>{id, name, class}</code>; name включает класс']]) + note('Создание нового объекта обычно меняет выделение. Перед каждым запросом нужного типа явно выбирайте сохранённый ID. Удаляйте только собственные промежуточные объекты: удаление всех объектов может затронуть работу пользователя.'),
    table(['API', 'Result / action'], [['<code>praat.call(name, ...)</code>', 'number, boolean, string, table, or no return values'], ['<code>praat.select(id, ...)</code>', 'replaces selection; also accepts a table of IDs'], ['<code>praat.select({})</code>', 'clears selection'], ['<code>praat.selected()</code>', 'table of selected IDs'], ['<code>praat.objects()</code>', 'all objects: <code>{id, name, class}</code>; name includes the class']]) + note('Creating an object usually changes selection. Explicitly select the saved ID before querying the required type. Remove only your own intermediate objects: deleting every object may destroy the user’s work.')),
    ('Создайте Copy звука с тем же именем. Можно ли безопасно выбирать по имени?', 'Create a Copy of the sound with the same name. Is selection by name safe?'),
    ('Нет: имена могут повторяться. <code>local copy = praat.call("Copy", "lesson_tone")[1]</code> даёт отдельный уникальный ID.', 'No: names can repeat. <code>local copy = praat.call("Copy", "lesson_tone")[1]</code> gives a distinct unique ID.'))

add('04-bridge', ('Перевод команд и два языка формул', 'Translating commands and the two formula languages'), '4.1, 4.5–4.10', (
    para('Используйте английское зарегистрированное имя команды, а аргументы передавайте в порядке полей её диалога, сверху вниз. В конце имени многоточие можно опустить, если переданы аргументы. В Lua нет автоматических именованных аргументов для диалогов Praat. Имена и строки передаются в UTF-8.') + para('Снаружи строки работают <code>math.sin</code> и <code>math.pi</code>; внутри формулы Praat — <code>sin</code>, <code>pi</code>, <code>x</code>, <code>self</code>, <code>row</code>, <code>col</code>. Переменная Lua не видна интерпретатору формул. Числовой параметр вставляют в строку через <code>string.format</code>.'),
    para('Use the registered English command name and pass arguments in dialog order, top to bottom. Trailing ellipses can be omitted when arguments are supplied. Lua does not automatically provide named arguments for Praat dialogs. Names and strings use UTF-8.') + para('Outside a string, use <code>math.sin</code> and <code>math.pi</code>; inside a Praat formula, use <code>sin</code>, <code>pi</code>, <code>x</code>, <code>self</code>, <code>row</code>, and <code>col</code>. Lua variables are invisible to the formula interpreter. Insert a numeric parameter with <code>string.format</code>.')),
    '''local hz, amplitude = 300, 0.15
local formula = string.format("%.17g*sin(2*pi*%.17g*x)", amplitude, hz)
local sound = praat.call("Create Sound from formula",
    "parameterised", 1, 0, 1, 44100, formula)[1]
praat.call("Formula", "self - 0.01")
praat.call("Subtract mean")
print("rms_Pa", praat.call("Get root-mean-square", 0, 0))
praat.call("Create simple Matrix", "matrix_demo", 2, 3, "row*10+col")
local matrix = praat.call("Get all values")
assert(matrix[2][3] == 23)
print("matrix_2_3", matrix[2][3])
''', (
    para('Для amplitude 0.15 RMS близок к 0.106066 Па. Вектор и массив строк из команды становятся таблицей с индексами от 1; матрица становится таблицей строк. В обратную сторону обычные таблицы в <code>praat.call</code> не передаются: аргументами могут быть только числа, boolean и строки. Для выбора нескольких объектов таблица разрешена в <code>praat.select</code>.'),
    para('For amplitude 0.15, RMS is approximately 0.106066 Pa. A returned vector or string array becomes a 1-based table; a matrix becomes a table of rows. Ordinary tables cannot be passed into <code>praat.call</code>: its arguments must be numbers, booleans, or strings. Tables are accepted by <code>praat.select</code> for selecting multiple objects.')),
    ('Как записать условие «первые полсекунды — тон, затем тишина» внутри формулы?', 'How do you express “tone for the first half-second, then silence” in a formula?'),
    ('<code>"if x &lt; 0.5 then 0.2*sin(2*pi*200*x) else 0 fi"</code>. Это синтаксис Praat. Условие Lua пишется <code>if ... then ... else ... end</code>.', '<code>"if x &lt; 0.5 then 0.2*sin(2*pi*200*x) else 0 fi"</code>. This is Praat syntax. A Lua conditional ends with <code>end</code>.'))

add('05-sampling', ('Отсчёты, единицы и дискретизация', 'Samples, units, and sampling'), '2, 3; A', (
    para('Звук — последовательность значений амплитуды в определённые моменты времени. Частота дискретизации Fs задаёт число отсчётов в секунду, а не высоту звука. Для тона частоты f период T = 1/f. Частота Найквиста равна Fs/2: при синтезе составляющие должны быть ниже неё. Изменить метаданные Fs и корректно передискретизировать звук — разные операции.') + para('Время отсчёта безопаснее запрашивать у объекта. Первый отсчёт не обязан приходиться на 0; обычно он расположен в центре первого интервала дискретизации. Для каналов нумерация начинается с 1.'),
    para('A sound is a sequence of amplitude values at particular times. The sampling frequency Fs counts samples per second, not pitch. A tone at frequency f has period T = 1/f. The Nyquist frequency is Fs/2: synthesized components must lie below it. Changing sample-rate metadata and properly resampling are different operations.') + para('Query sample times from the object. The first sample need not occur at zero; it usually sits at the centre of the first sampling interval. Channels start at 1.')),
    TONE + '''local fs = praat.call("Get sampling frequency")
local n = praat.call("Get number of samples")
local peak, squareSum = 0, 0
for i = 1, n do
    local value = praat.call("Get value at sample number", 1, i)
    peak = math.max(peak, math.abs(value))
    squareSum = squareSum + value * value
end
print("Fs_Hz", fs, "Nyquist_Hz", fs / 2)
print("first_time_s", praat.call("Get time from sample number", 1))
print("peak_Pa", peak, "rms_Pa", math.sqrt(squareSum / n))
assert(math.abs(math.sqrt(squareSum / n) - 0.2/math.sqrt(2)) < 1e-6)
local resampled = praat.call("Resample", 16000, 50)[1]
assert(praat.call("Get number of samples") == 16000)
''', (
    para('Ожидаются peak около 0.2 Па и RMS около 0.141421 Па. Цикл полезен для обучения, но каждый вызов моста имеет стоимость: для большого файла предпочитайте встроенный Get root-mean-square. Амплитуды WAV после чтения обычно нормированы; физический уровень SPL требует калибровки. Ограничение ±1 относится к обычной нормировке PCM при записи, а не к универсальному внутреннему диапазону Sound.'),
    para('Expect peak near 0.2 Pa and RMS near 0.141421 Pa. The loop is educational, but each bridge call has overhead: use Get root-mean-square for a large file. WAV values are normally normalized on reading; physical SPL requires calibration. The usual ±1 PCM normalization on file output is not a universal limit on internal Sound values.')),
    ('Синтезируйте 900 Гц при Fs=1000 Гц. Почему результат нельзя считать честным тоном 900 Гц?', 'Synthesize 900 Hz at Fs=1000 Hz. Why is the result not a faithfully represented 900 Hz tone?'),
    ('900 Гц выше Найквиста 500 Гц; возникает алиасинг к 100 Гц. Для корректного представления 900 Гц нужна Fs строго выше 1800 Гц и запас для фильтрации.', '900 Hz exceeds the 500 Hz Nyquist frequency and aliases to 100 Hz. Faithful representation needs Fs strictly above 1800 Hz, with practical filtering margin.'))

add('06-files', ('Чтение, запись и рабочая папка', 'Reading, writing, and the working folder'), '3, 4', (
    para('Сохранённый скрипт выполняется относительно своей папки. Новый несохранённый скрипт использует текущую рабочую папку Praat. Перед файловыми уроками сохраняйте файл. В путях Windows удобны прямые косые черты: <code>C:/speech/token.wav</code>; альтернативно используйте длинную строку <code>[[C:\\speech\\token.wav]]</code>.') + para('Пример создаёт собственный WAV, читает его и вырезает участок. <code>Extract part</code> принимает начало, конец, форму окна, относительную ширину и Preserve times. Значение false переносит начало фрагмента к нулю; true сохраняет исходную временную систему.'),
    para('A saved script runs relative to its own folder. A new unsaved script uses Praat’s current working folder. Save the script before file lessons. On Windows, forward slashes are convenient: <code>C:/speech/token.wav</code>; a long string such as <code>[[C:\\speech\\token.wav]]</code> is another option.') + para('The example creates its own WAV, reads it, and extracts a segment. <code>Extract part</code> takes start, end, window shape, relative width, and Preserve times. False resets the segment’s start to zero; true retains the original time coordinates.')),
    TONE + '''praat.call("Save as WAV file", "lesson-tone.wav")
local loaded = praat.call("Read from file", "lesson-tone.wav")[1]
print("loaded_duration_s", praat.call("Get total duration"))
local part = praat.call("Extract part", 0.2, 0.5, "rectangular", 1, false)[1]
assert(math.abs(praat.call("Get total duration") - 0.3) < 1e-9)
praat.call("Save as WAV file", "lesson-part.wav")
local lfs = require("lfs")
print("folder", lfs.currentdir())
print("wav_bytes", lfs.attributes("lesson-tone.wav", "size"))
''', (
    note('Повторный запуск заменяет lesson-tone.wav и lesson-part.wav. Используйте отдельную папку упражнений. Для архивирования анализа сохраняйте также настройки и TextGrid; WAV не содержит объекты Pitch/Formant. LuaFileSystem в Windows ограничен системной кодовой страницей для имён файлов: для путей за её пределами используйте файловые команды Praat или простые латинские имена.'),
    note('Rerunning replaces lesson-tone.wav and lesson-part.wav. Use a dedicated exercise folder. Archive settings and TextGrid alongside audio; WAV does not contain Pitch/Formant objects. On Windows, LuaFileSystem filenames are limited by the system code page: use Praat file commands or simple ASCII names for paths outside it.')),
    ('Вырежьте тот же участок с Preserve times=true. Каковы начало и конец?', 'Extract the same segment with Preserve times=true. What are its start and end times?'),
    ('Около 0.2 и 0.5 с. Длительность остаётся 0.3 с. Координаты исходного TextGrid подходят только при согласованной временной системе.', 'Approximately 0.2 and 0.5 s. Duration stays 0.3 s. Original TextGrid coordinates are usable only when the time systems agree.'))

add('07-functions', ('Функции, параметры и проверка входных данных', 'Functions, parameters, and input validation'), '4.4, 4.10–4.12', (
    para('Выносите повторяющийся анализ в функции, а параметры — в таблицу в начале файла. Имя с единицами, например <code>duration_s</code>, уменьшает риск путаницы между секундами и миллисекундами. <code>assert</code> останавливает скрипт с объяснением, если условие не выполнено. Функции должны возвращать результат явно и предсказуемо управлять выделением.'),
    para('Put repeated analysis into functions and parameters into a table at the top. Names with units, such as <code>duration_s</code>, help avoid confusing seconds and milliseconds. <code>assert</code> stops execution with an explanation when a condition fails. Functions should return results explicitly and manage selection predictably.')),
    '''local settings = {duration_s=0.5, sample_rate_Hz=44100, amplitude=0.1}
local function makeTone(hz, cfg)
    assert(hz > 0 and hz < cfg.sample_rate_Hz/2, "Invalid frequency")
    assert(cfg.duration_s > 0, "Duration must be positive")
    local formula = string.format("%.17g*sin(2*pi*%.17g*x)", cfg.amplitude, hz)
    return praat.call("Create Sound from formula", "tone_" .. hz,
        1, 0, cfg.duration_s, cfg.sample_rate_Hz, formula)[1]
end
local function rms(id)
    praat.select(id)
    return praat.call("Get root-mean-square", 0, 0)
end
for _, hz in ipairs({100, 200, 400}) do
    local id = makeTone(hz, settings)
    print(hz, rms(id))
    praat.select(id)
    praat.call("Remove")
end
''', (
    para('Все три RMS близки к 0.070711 Па. Lua не понимает форму <code>form ... endform</code> из обычного Praat-скрипта. Для простого интерактивного сценария меняйте таблицу settings; для пакетного запуска используйте <code>arg</code>. Чистые Lua-модули можно вынести в соседний файл, вернуть из него таблицу функций и загрузить через <code>require("analysis")</code>. Каждый новый запуск загружает их заново в новое состояние.'),
    para('All three RMS values are approximately 0.070711 Pa. Lua does not understand the native Praat <code>form ... endform</code> syntax. For simple interactive use, edit settings; for batch runs, use <code>arg</code>. Pure Lua modules can live in a neighbouring file that returns a function table, loaded with <code>require("analysis")</code>. Each new run loads them into a fresh state.')),
    ('Добавьте проверку положительной частоты дискретизации и ограничение амплитуды от 0 до 0.9.', 'Validate a positive sample rate and constrain amplitude to 0 through 0.9.'),
    ('<code>assert(cfg.sample_rate_Hz &gt; 0)</code> и <code>assert(cfg.amplitude &gt;= 0 and cfg.amplitude &lt;= 0.9)</code>. Эти ограничения — выбранная политика генератора, а не ограничение типа Sound.', '<code>assert(cfg.sample_rate_Hz &gt; 0)</code> and <code>assert(cfg.amplitude &gt;= 0 and cfg.amplitude &lt;= 0.9)</code>. These are generator policies, not Sound type limits.'))

add('08-pitch', ('Основная частота F0 и пропуски', 'Fundamental frequency F0 and missing values'), '5', (
    para('F0 описывает повторение колебаний, а форманты — резонансы речевого тракта. Это разные величины. Пример использует простой <code>To Pitch</code> с тремя аргументами: шаг времени, нижняя граница и верхняя граница. Нулевой шаг означает автоматический выбор. Эти параметры задаются для воспроизводимого опыта; настройки отображения Pitch в SoundEditor не заменяют их.') + para('На глухих участках и в тишине F0 может быть undefined. Числовой undefined приходит как NaN: он не равен самому себе. Проверять только <code>if value then</code> нельзя, поскольку NaN в Lua истинен. Не превращайте пропуски в нули при расчёте средней F0.'),
    para('F0 describes repetition of oscillations, whereas formants describe vocal-tract resonances. They are different quantities. This example uses the simple three-argument <code>To Pitch</code>: time step, pitch floor, and pitch ceiling. Zero time step selects automatic spacing. Explicit analysis settings make the experiment reproducible; Pitch display settings in SoundEditor do not replace them.') + para('Unvoiced regions and silence may have undefined F0. Numeric undefined arrives as NaN, which is unequal to itself. <code>if value then</code> is insufficient because NaN is true in Lua. Never replace missing values by zeros when calculating mean F0.')),
    TONE + '''local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
local n = praat.call("Get number of frames")
local valid, total = 0, 0
for frame = 1, n do
    local hz = praat.call("Get value in frame", frame, "Hertz")
    if hz == hz and hz > 0 and hz < math.huge then
        valid, total = valid + 1, total + hz
    end
end
assert(valid > 0, "No voiced frames")
local mean = total / valid
print("voiced_frames", valid, "all_frames", n)
print("frame_mean_F0_Hz", mean)
print("median_F0_Hz", praat.call("Get quantile", 0, 0, 0.5, "Hertz"))
assert(math.abs(mean - 200) < 2)
''', (
    para('На учебном тоне средняя и медиана близки к 200 Гц. Для речи подбирайте диапазон по говорящему и проверяйте октавные ошибки: скачки примерно в 2 раза могут быть ошибкой трекера. Доля валидных кадров — характеристика результата алгоритма при данных настройках, а не готовая оценка доли звонкой речи. В главе 5 источника обсуждаются кандидаты и выбор траектории; здесь Lua автоматизирует применение анализа.'),
    para('On the tutorial tone, mean and median are close to 200 Hz. For speech, choose a speaker-appropriate range and inspect octave errors: jumps by a factor of about two may be tracking errors. The valid-frame fraction describes the algorithm’s output under these settings, not a ready-made measure of voiced-speech proportion. Source chapter 5 discusses candidates and path selection; Lua automates the analysis here.')),
    ('Замените формулу звука на "0". Как должен вести себя скрипт?', 'Replace the sound formula by "0". What should the script do?'),
    ('Сообщить No voiced frames вместо деления на ноль или отчёта F0=0. Для корпуса запишите NA и продолжите обработку остальных файлов.', 'Report No voiced frames rather than divide by zero or report F0=0. In a corpus workflow, record NA and continue with the other files.'))

add('09-intensity', ('Интенсивность и обнаружение пауз', 'Intensity and silence detection'), '6', (
    para('Интенсивность — сглаженная энергетическая характеристика, выраженная в децибелах. Параметр pitch floor здесь управляет длиной окна, а не измеряет F0. При удвоении амплитуды неизменного сигнала уровень повышается примерно на 6.02 дБ. Физический SPL корректен только для калиброванных амплитуд.') + para('Для среднего укажите способ усреднения: <code>energy</code> усредняет энергию перед переводом в дБ, а <code>dB</code> усредняет числа в децибелах. Они обычно различаются. Порог тишины в следующей команде задаётся в дБ относительно максимума интенсивности.'),
    para('Intensity is a smoothed energy-related measure in decibels. Its pitch-floor parameter controls the analysis-window length; it does not measure F0. Doubling the amplitude of the same signal raises the level by about 6.02 dB. Physical SPL is meaningful only for calibrated amplitudes.') + para('Specify the averaging method: <code>energy</code> averages energy before conversion to dB, while <code>dB</code> averages the decibel values. They generally differ. The silence threshold in the following command is in dB relative to maximum intensity.')),
    '''local sound = praat.call("Create Sound from formula", "pause_demo",
    1, 0, 1, 44100,
    "if x>0.25 and x<0.75 then 0.2*sin(2*pi*200*x) else 0 fi")[1]
local intensity = praat.call("To Intensity", 75, 0.01, true)[1]
print("energy_mean_dB", praat.call("Get mean", 0, 0, "energy"))
print("median_dB", praat.call("Get quantile", 0, 0, 0.5))
local grid = praat.call("To TextGrid (silences)",
    -25, 0.10, 0.10, "silent", "sounding")[1]
print("intervals", praat.call("Get number of intervals", 1))
praat.call("Save as text file", "lesson-silences.TextGrid")
''', (
    para('Появятся Intensity и TextGrid с участками silent/sounding. Границы будут около 0.25 и 0.75 с, но сглаживание и минимальные длительности смещают их. Это энергетическая сегментация, а не распознавание слов или надёжное определение границ фонем. Прослушайте и исправьте результат перед измерениями.'),
    para('The output includes Intensity and a TextGrid with silent/sounding intervals. Boundaries lie near 0.25 and 0.75 s, but smoothing and minimum-duration rules shift them. This is energy-based segmentation, not word recognition or reliable phoneme-boundary detection. Listen and correct it before making measurements.')),
    ('Что произойдёт при увеличении минимальной длительности тишины до 0.4 с?', 'What happens if minimum silence duration is increased to 0.4 s?'),
    ('Короткие паузы могут быть объединены с соседними звучащими участками. Проверяйте итоговые интервалы, а не только отсутствие ошибки выполнения.', 'Short silent intervals may merge into neighbouring sounding regions. Inspect the resulting intervals, not just the absence of an execution error.'))

add('10-spectrum', ('Спектр, фаза и разрешение', 'Spectrum, phase, and resolution'), '7', (
    para('Spectrum описывает частотный состав всего выбранного звука. Он не показывает, когда появилась частота. Для стационарного отрезка длительности T характерный масштаб разрешения порядка 1/T. Дополнение нулями для FFT делает сетку частот плотнее, но не добавляет информации и не разделяет близкие компоненты само по себе.') + para('Spectrum хранит действительную и мнимую части. Их модуль полезен для сравнений внутри одинаковой процедуры, но без правильной нормировки его нельзя подписывать как спектральную плотность в дБ/Гц.'),
    para('A Spectrum describes the frequency content of the entire selected sound. It does not show when a frequency occurred. For a stationary segment of duration T, the characteristic resolution scale is roughly 1/T. FFT zero-padding densifies the frequency grid but adds no information and does not by itself resolve nearby components.') + para('Spectrum stores real and imaginary components. Their magnitude is useful for comparisons within the same procedure, but it must not be labelled spectral density in dB/Hz without appropriate normalization.')),
    '''local sound = praat.call("Create Sound from formula", "mixture",
    1, 0, 1, 16000, "0.2*sin(2*pi*200*x)+0.1*sin(2*pi*600*x)")[1]
local spectrum = praat.call("To Spectrum", true)[1]
local n = praat.call("Get number of bins")
local bestHz, bestMagnitude = 0, -1
for bin = 1, n do
    local re = praat.call("Get real value in bin", bin)
    local im = praat.call("Get imaginary value in bin", bin)
    local magnitude = math.sqrt(re*re + im*im)
    if magnitude > bestMagnitude then
        bestMagnitude = magnitude
        bestHz = praat.call("Get frequency from bin number", bin)
    end
end
print("largest_bin_Hz", bestHz)
assert(math.abs(bestHz - 200) < 2)
''', (
    para('Самый большой бин находится около 200 Гц; второй компонент — около 600 Гц. Уменьшите длительность до 0.02 с и посмотрите, как расширяются пики. Не смешивайте «максимум спектра», F0 и F1: в сложном звуке они могут быть разными.'),
    para('The strongest bin is near 200 Hz; the second component is near 600 Hz. Shorten the duration to 0.02 s and inspect peak broadening. Do not equate the largest spectral peak, F0, and F1: in a complex signal they may differ.')),
    ('Замените синус 600 Гц на косинус. Что изменится?', 'Replace the 600 Hz sine by a cosine. What changes?'),
    ('Меняется фаза этого компонента и временная форма смеси. При согласованных условиях модуль соответствующего спектрального пика останется практически тем же.', 'That component’s phase and the mixture’s waveform change. Under matching conditions, its spectral-peak magnitude remains practically the same.'))

add('11-spectrogram', ('Спектрограмма и компромисс окна', 'Spectrograms and the window trade-off'), '8', (
    para('Спектрограмма — последовательность локальных спектров. Короткое окно лучше показывает быстрые события, длинное — близкие частоты и гармоники. Меньший time step даёт больше временных кадров, но не отменяет размытие, заданное окном. Меньший frequency step уплотняет сетку, но не заменяет длинное окно.') + para('Аргументы <code>To Spectrogram</code>: длина окна в секундах, максимальная частота в герцах, шаг времени в секундах, шаг частоты в герцах, форма окна. Пример создаёт линейный частотный свип: в фазе нужен член x², поскольку мгновенная частота — производная фазы, делённая на 2π.'),
    para('A spectrogram is a sequence of local spectra. A short window localizes rapid events; a long window better separates nearby frequencies and harmonics. Smaller time steps add frames but do not remove window-induced smearing. Smaller frequency steps densify the grid but do not substitute for a longer window.') + para('<code>To Spectrogram</code> arguments are window length in seconds, maximum frequency in Hz, time step in seconds, frequency step in Hz, and window shape. This example creates a linear frequency sweep: phase needs an x² term because instantaneous frequency is the derivative of phase divided by 2π.')),
    '''local sound = praat.call("Create Sound from formula", "chirp",
    1, 0, 1, 16000, "0.2*sin(2*pi*(200*x+900*x*x))")[1]
local wide = praat.call("To Spectrogram", 0.005, 4000, 0.002, 20, "Gaussian")[1]
praat.select(sound)
local narrow = praat.call("To Spectrogram", 0.04, 4000, 0.002, 20, "Gaussian")[1]
print("spectrogram_ids", wide, narrow)
praat.call("Save as text file", "lesson-spectrogram.txt")
''', (
    para('В Objects выделите каждую Spectrogram и нажмите View для сравнения. Начальная частота около 200 Гц, конечная около 2000 Гц. Для речи сравните широкополосное и узкополосное представления одного участка. Интерактивная спектрограмма SoundEditor и созданный объект Spectrogram имеют отдельные настройки.'),
    para('In Objects, select each Spectrogram and press View to compare. Frequency starts near 200 Hz and ends near 2000 Hz. For speech, compare wideband and narrowband views of the same segment. SoundEditor’s interactive spectrogram and the created Spectrogram object have separate settings.')),
    ('Почему уменьшение шага частоты с 20 до 5 Гц не гарантирует разделение двух тонов через 10 Гц?', 'Why does reducing frequency step from 20 to 5 Hz not guarantee separating two tones 10 Hz apart?'),
    ('Плотность сетки и реальное разрешение различаются. Нужно достаточно длинное окно, подходящая форма окна и стационарность сигнала внутри него.', 'Grid density and actual resolution differ. You need a sufficiently long window, a suitable window shape, and adequate stationarity within it.'))

add('12-filters', ('Фильтрация, шум и импульсный отклик', 'Filtering, noise, and impulse responses'), '2.7, 7.5, 11', (
    para('Фильтр меняет амплитуду и фазу частотных составляющих. Полосовой фильтр сохраняет выбранную полосу; formant filter моделирует резонанс. Сначала работайте с копией или результатом преобразования и сравнивайте его с исходником. Нормализация после фильтра полезна для прослушивания, но скрывает изменение уровня: измеряйте RMS до нормализации.') + para('В pass Hann band аргументы — нижняя граница, верхняя граница и ширина сглаживания в герцах. Фильтр возвращает новый Sound. Impulse response показывает реакцию системы на короткий импульс; не путайте его с произвольным шумовым откликом.'),
    para('A filter changes frequency-component amplitude and phase. A band-pass filter retains a selected band; a formant filter models a resonance. Work on a copy or a converted result and compare it with the original. Normalizing afterwards helps listening but conceals level changes: measure RMS before normalization.') + para('Pass Hann band takes lower frequency, upper frequency, and smoothing width in Hz. It returns a new Sound. An impulse response shows the system’s response to a short impulse; it is not an arbitrary noise response.')),
    '''local original = praat.call("Create Sound from formula", "two_tones",
    1, 0, 1, 16000, "0.2*sin(2*pi*200*x)+0.2*sin(2*pi*2000*x)")[1]
local before = praat.call("Get root-mean-square", 0, 0)
local filtered = praat.call("Filter (pass Hann band)", 100, 500, 50)[1]
local after = praat.call("Get root-mean-square", 0, 0)
print("RMS_before_after", before, after)
assert(after < before)
praat.call("Save as WAV file", "lesson-filtered.wav")
local impulse = praat.call("Create Sound from formula", "impulse",
    1, 0, 0.1, 16000, "if col=1 then 1 else 0 fi")[1]
local response = praat.call("Filter (one formant)", 800, 80)[1]
praat.call("Save as WAV file", "lesson-response.wav")
''', (
    para('После фильтра смесь сохраняет в основном 200 Гц; RMS уменьшается с примерно 0.2 до 0.141 Па. Отклик резонатора содержит затухающие колебания около 800 Гц. При сравнении фильтров указывайте параметры переходной полосы и способ обработки краёв. Для звуковых демонстраций шум можно создать формулой Praat <code>randomGauss(0,0.05)</code>; это отдельный генератор от <code>math.random</code> Lua.'),
    para('After filtering, mainly the 200 Hz component remains; RMS drops from about 0.2 to 0.141 Pa. The resonator response contains decaying oscillations near 800 Hz. When comparing filters, report transition-band settings and edge handling. For demonstrations, Praat formula <code>randomGauss(0,0.05)</code> generates noise; it uses a different generator from Lua’s <code>math.random</code>.')),
    ('Увеличьте полосу резонатора с 80 до 160 Гц. Как изменится затухание?', 'Increase resonator bandwidth from 80 to 160 Hz. How does decay change?'),
    ('Затухание становится быстрее: большая полоса соответствует менее длительному звону. Сравнивайте огибающие на одинаковом временном масштабе.', 'Decay becomes faster: a wider bandwidth corresponds to shorter ringing. Compare envelopes on identical time scales.'))

add('13-textgrid', ('TextGrid: интервалы и метки', 'TextGrid: intervals and labels'), '9', (
    para('TextGrid хранит разметку, а не звук. IntervalTier содержит интервалы, PointTier — отдельные события. <code>To TextGrid</code> создаёт ярусы по именам: первый аргумент перечисляет все ярусы, второй — те из них, которые должны быть точечными. Пустая вторая строка даёт только интервальные ярусы.') + para('Для учебного тона отметим два участка вручную заданными координатами. Для настоящей речи границы нужно выбирать по звуку и контексту. Совпадение с переходом через ноль может уменьшить щелчок при вырезании, но не определяет лингвистическую границу.'),
    para('A TextGrid stores annotations, not audio. An IntervalTier contains intervals; a PointTier contains individual events. <code>To TextGrid</code> creates tiers by name: the first argument lists all tiers, and the second lists those that should be point tiers. An empty second string creates only interval tiers.') + para('For the tutorial tone, annotate two regions using predetermined times. For real speech, choose boundaries by listening and context. A zero crossing can reduce a cut-induced click but does not define a linguistic boundary.')),
    TONE + '''local grid = praat.call("To TextGrid", "segments events", "events")[1]
praat.call("Insert boundary", 1, 0.4)
praat.call("Set interval text", 1, 1, "part_A")
praat.call("Set interval text", 1, 2, "part_B")
praat.call("Insert point", 2, 0.4, "transition")
local count = praat.call("Get number of intervals", 1)
for interval = 1, count do
    local a = praat.call("Get start time of interval", 1, interval)
    local b = praat.call("Get end time of interval", 1, interval)
    local label = praat.call("Get label of interval", 1, interval)
    print(label, a, b, b-a)
end
praat.call("Save as text file", "lesson-annotation.TextGrid")
''', (
    para('Результат: part_A длится 0.4 с, part_B — 0.6 с. Для ручного редактирования после завершения скрипта выделите Sound и TextGrid вместе и нажмите View &amp; Edit. В пользовательском общем редакторе View &amp; Edit selected TextGrid может быть только слоем просмотра; для изменения меток используйте штатный TextGridEditor.'),
    para('Result: part_A lasts 0.4 s and part_B lasts 0.6 s. To edit manually after the script finishes, select Sound and TextGrid together and press View &amp; Edit. In the custom View &amp; Edit selected combined editor, TextGrid may be a viewing layer; use the standard TextGridEditor to change labels.')),
    ('Почему Get number of intervals нельзя применять к ярусу events?', 'Why should Get number of intervals not be applied to the events tier?'),
    ('Это PointTier. Для него нужны команды точек, например Get number of points с номером яруса, а не команды интервалов.', 'It is a PointTier. Use point-tier commands, such as Get number of points with the tier number, rather than interval commands.'))

add('14-intervals', ('Измерения по размеченным участкам', 'Measuring annotated regions'), '9; 13.3.5', (
    para('Главная связка корпусного анализа: прочитать границы из TextGrid, выбрать объект анализа и запросить значение на том же временном участке. Не пересоздавайте Pitch отдельно для каждого короткого интервала без причины: полный анализ сохраняет контекст окна. Координаты TextGrid должны совпадать с координатами Sound и Pitch.') + para('Пример измеряет медиану F0 на двух интервалах. Он каждый раз явно меняет выделение между TextGrid и Pitch; команда с одинаковым именем Get mean может иметь совершенно разные аргументы для разных классов.'),
    para('The core corpus-analysis pattern is to read boundaries from TextGrid, select the analysis object, and query the same time region. Avoid recreating Pitch for each short interval without a reason: full-sound analysis retains window context. TextGrid coordinates must match Sound and Pitch coordinates.') + para('The example measures median F0 over two intervals. It explicitly switches selection between TextGrid and Pitch; the same command name, such as Get mean, can have very different arguments for different object classes.')),
    TONE + '''local grid = praat.call("To TextGrid", "segments", "")[1]
praat.call("Insert boundary", 1, 0.5)
praat.call("Set interval text", 1, 1, "early")
praat.call("Set interval text", 1, 2, "late")
praat.select(sound)
local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
praat.select(grid)
local n = praat.call("Get number of intervals", 1)
for i = 1, n do
    praat.select(grid)
    local label = praat.call("Get label of interval", 1, i)
    local a = praat.call("Get start time of interval", 1, i)
    local b = praat.call("Get end time of interval", 1, i)
    if label ~= "" and b > a then
        praat.select(pitch)
        local hz = praat.call("Get quantile", a, b, 0.5, "Hertz")
        local value = hz == hz and string.format("%.3f", hz) or "NA"
        print(label, b-a, value)
    end
end
''', (
    para('Обе медианы близки к 200 Гц. Пропуск обозначается NA. Для настоящих файлов согласуйте политику: пропускать пустые метки, исключать паузы по конкретным меткам или сохранять их с NA. Не выбирайте эту политику неявно: пустая метка и метка silent — разные случаи.'),
    para('Both medians are near 200 Hz. Missing values are represented as NA. For real data, specify a policy: skip empty labels, exclude particular silence labels, or retain them with NA. Do not make that policy implicit: an empty label and a label named silent are different cases.')),
    ('Вычислите F0 в середине каждого интервала вместо медианы.', 'Query F0 at each interval midpoint instead of its median.'),
    ('<code>praat.call("Get value at time", (a+b)/2, "Hertz", "linear")</code> при выбранном Pitch. Это точечная оценка, она может отличаться от медианы интервала.', '<code>praat.call("Get value at time", (a+b)/2, "Hertz", "linear")</code> with Pitch selected. This is a point estimate and can differ from the interval median.'))

add('15-synthesis', ('Гласные, VowelEditor и KlattGrid', 'Vowels, VowelEditor, and KlattGrid'), '10, 12', (
    para('VowelEditor удобен для ручного исследования пространства гласных. В этом мосте нет общего API команд окон редакторов: не пытайтесь передавать редакторские жесты через praat.call. Для воспроизводимого синтеза из Lua используйте команды объектов KlattGrid.') + para('Модель «источник–фильтр» отделяет колебания источника (F0) от резонансов (F1, F2 и далее). Пример создаёт устойчивый гласноподобный звук с заданными резонансами. Это учебная модель, а не эталон конкретной фонемы или говорящего.'),
    para('VowelEditor is useful for manually exploring vowel space. This bridge does not provide a general editor-window command API: do not try to send editor gestures through praat.call. Use KlattGrid object commands for reproducible synthesis from Lua.') + para('The source–filter model separates source oscillation (F0) from resonances (F1, F2, and higher formants). The example creates a steady vowel-like sound with specified resonances. It is a teaching model, not a reference pronunciation for a phoneme or speaker.')),
    VOWEL + '''praat.call("Save as WAV file", "lesson-vowel.wav")
praat.select(grid)
praat.call("Add pitch point", 0.8, 180)
local rising = praat.call("To Sound")[1]
praat.call("Save as WAV file", "lesson-rising-vowel.wav")
print("sound_ids", sound, rising)
''', (
    table(['Параметры Create KlattGrid from vowel', 'Значения в примере'], [['имя; длительность; F0', 'lesson_vowel; 1 с; 125 Гц'], ['F1, B1; F2, B2; F3, B3', '700, 60; 1200, 90; 2500, 150 Гц'], ['F4; доля полосы; интервал следующих формант', '3500 Гц; 0.05; 1000 Гц']]) + para('После добавления точки F0 к 0.8 с приближается к 180 Гц; резонансы сохранены. Для сложного KlattGrid есть отдельные ярусы озвончения, придыхания, фрикации, ротовых и носовых формант и антиформант. Заполняйте необходимые ярусы осмысленно: пустая сетка сама по себе не обязана звучать.'),
    table(['Create KlattGrid from vowel parameters', 'Example values'], [['name; duration; F0', 'lesson_vowel; 1 s; 125 Hz'], ['F1, B1; F2, B2; F3, B3', '700, 60; 1200, 90; 2500, 150 Hz'], ['F4; bandwidth fraction; higher-formant spacing', '3500 Hz; 0.05; 1000 Hz']]) + para('After adding the pitch point, F0 approaches 180 Hz at 0.8 s while resonances remain unchanged. A full KlattGrid has separate voicing, aspiration, frication, oral/nasal formant, and antiformant tiers. Populate the required tiers deliberately: an empty grid need not produce sound.')),
    ('Измените F2 с 1200 до 1800 Гц, оставив F0=125 Гц. Что сравнивать?', 'Change F2 from 1200 to 1800 Hz while keeping F0=125 Hz. What should you compare?'),
    ('Тембр и положение резонанса F2 на спектрограмме, а не только высоту голоса. Затем измерьте F0 и F2 разными командами.', 'Compare timbre and the F2 resonance in the spectrogram, not just perceived pitch. Then measure F0 and F2 with separate commands.'))

add('16-formants', ('Форманты: измерение и проверка', 'Formants: measurement and validation'), '13', (
    para('Формантный анализ оценивает резонансы моделью, а не просто находит яркие полосы. Синтетический гласный даёт контролируемые исходные параметры, с которыми удобно сравнить оценку. Аргументы Burg: шаг времени, максимальное число формант, потолок частот, длина окна, предыскажение от указанной частоты.') + para('Настройки 5 формант и 5500 Гц — учебная отправная точка. Для реальных говорящих проверяйте потолок, порядок модели, устойчивость F1/F2 и спектрограмму. Высокий F0, носовые резонансы и шум затрудняют оценку. Оценённые значения не обязаны точно совпадать с заданными в синтезе.'),
    para('Formant analysis estimates resonances with a model rather than simply finding bright bands. A synthetic vowel provides controlled input settings for comparing estimates. Burg arguments are time step, maximum number of formants, frequency ceiling, window length, and pre-emphasis frequency.') + para('Five formants and a 5500 Hz ceiling are a teaching starting point. For real speakers, inspect ceiling, model order, F1/F2 stability, and the spectrogram. High F0, nasal resonances, and noise complicate estimation. Estimated values need not exactly reproduce synthesis settings.')),
    VOWEL + '''local formant = praat.call("To Formant (burg)", 0.005, 5, 5500, 0.025, 50)[1]
for _, t in ipairs({0.25, 0.5, 0.75}) do
    local f1 = praat.call("Get value at time", 1, t, "Hertz", "linear")
    local f2 = praat.call("Get value at time", 2, t, "Hertz", "linear")
    print("time_F1_F2", t, f1, f2)
    assert(f1 == f1 and f2 == f2, "Missing formant estimate")
    assert(f1 > 400 and f1 < 1000 and f2 > 900 and f2 < 1600)
end
''', (
    para('Смотрите на устойчивые оценки около F1=700 и F2=1200 Гц. Для динамики измеряйте в одинаковых относительных точках интервала, например 20%, 50%, 80%: <code>t=a+q*(b-a)</code>. Наклон между двумя точками <code>(F_end-F_start)/(t_end-t_start)</code> имеет единицы Гц/с. Две точки не показывают нелинейную форму траектории. Нормализация гласных требует данных по говорящему; не выполняйте z-преобразование по одному токену.'),
    para('Look for stable estimates near F1=700 and F2=1200 Hz. For trajectories, measure matched relative interval positions such as 20%, 50%, and 80%: <code>t=a+q*(b-a)</code>. A two-point slope <code>(F_end-F_start)/(t_end-t_start)</code> is in Hz/s. Two points cannot describe a nonlinear contour. Vowel normalization requires speaker-level data; do not z-normalize a single token.')),
    ('Повторите анализ с потолками 4500, 5000 и 5500 Гц. Как выбирать результат?', 'Repeat analysis with ceilings of 4500, 5000, and 5500 Hz. How should you choose a result?'),
    ('Сравните устойчивость, спектрограмму и ожидаемые резонансы модели. Для речи задайте правило до массового анализа; нельзя выбирать потолок отдельно ради желаемого вывода.', 'Compare stability, the spectrogram, and the model’s intended resonances. For speech, define a selection rule before batch analysis; do not pick ceilings separately to obtain a desired conclusion.'))

add('17-lpc', ('LPC и предыскажение', 'LPC and pre-emphasis'), '16', (
    para('LPC приближает текущий отсчёт линейной комбинацией предыдущих. Из коэффициентов можно получить сглаженную спектральную огибающую и кандидаты резонансов. Порядок модели связан с числом полюсов, а не равен числу формант. Перед анализом согласуйте частоту дискретизации с интересующим диапазоном.') + para('Пример явно передискретизирует синтетический гласный до 11000 Гц и строит LPC порядка 10. В отличие от To Formant (burg), здесь параметры LPC задаются напрямую. Предыскажение усиливает высокочастотную часть; это этап анализа, а не улучшение оригинальной записи.'),
    para('LPC predicts a sample as a linear combination of preceding samples. Its coefficients can yield a smoothed spectral envelope and resonance candidates. Model order is related to the number of poles, not identical to the number of formants. Match the sampling rate to the frequency range of interest before analysis.') + para('The example explicitly resamples a synthetic vowel to 11000 Hz and fits an order-10 LPC model. Unlike To Formant (burg), this exposes LPC parameters directly. Pre-emphasis boosts higher frequencies for analysis; it does not improve the original recording.')),
    VOWEL + '''local lowRate = praat.call("Resample", 11000, 50)[1]
local lpc = praat.call("To LPC (burg)", 10, 0.025, 0.005, 50)[1]
print("lpc_frames", praat.call("Get number of frames"))
praat.call("Save as text file", "lesson.LPC")
''', (
    para('Получается объект LPC с несколькими временными кадрами. Слишком высокий порядок может моделировать гармоники или шум вместо нужной огибающей; слишком низкий — объединять резонансы. Сравнивайте спектр, LPC-огибающую и форманты на одном временном участке. Этот практикум не заменяет выводы Z-преобразования и устойчивости фильтров из главы 16.'),
    para('The result is an LPC object containing multiple time frames. Excessive order may model harmonics or noise rather than the intended envelope; insufficient order may merge resonances. Compare the spectrum, LPC envelope, and formants over the same time region. This lab does not replace chapter 16’s derivations of the Z-transform and filter stability.')),
    ('Какие параметры нужно записать для воспроизведения LPC-анализа?', 'Which settings are needed to reproduce the LPC analysis?'),
    ('Частоту дискретизации и передискретизацию, метод, порядок, окно, шаг времени, предыскажение и участок исходного файла.', 'Sampling rate and resampling settings, method, order, window length, time step, pre-emphasis, and the original file region.'))

add('18-pulses', ('PointProcess и периодичность', 'PointProcess and periodicity'), '5.2, 15', (
    para('PointProcess хранит времена событий. При анализе голоса это оценки периодических импульсов. Они нужны для изучения периодов и jitter. Результат зависит от метода выделения импульсов и ограничений допустимых периодов; простое число точек не равно длительности звонкой речи.') + para('Jitter local — относительная вариативность соседних периодов. Для чистого синтетического тона ожидается малая величина. Это учебное измерение, не медицинский диагноз. Shimmer дополнительно требует исходный Sound и оценивает вариативность амплитуд.'),
    para('A PointProcess stores event times. For voice analysis, these may be estimated periodic pulses. They support period and jitter measurements. Results depend on pulse extraction and period constraints; a point count is not the duration of voiced speech.') + para('Local jitter measures relative variation between neighbouring periods. A clean synthetic tone should give a small value. This is a teaching measurement, not a medical diagnosis. Shimmer also requires the original Sound and measures amplitude variation.')),
    TONE + '''local pulses = praat.call("To PointProcess (periodic, cc)", 75, 500)[1]
print("pulse_count", praat.call("Get number of points"))
local jitter = praat.call("Get jitter (local)", 0, 0, 0.0001, 0.02, 1.3)
print("jitter_fraction", jitter, "jitter_percent", 100*jitter)
assert(jitter == jitter and jitter < 0.01)
''', (
    para('Период тона 200 Гц равен 0.005 с, а число точек будет около 200 с потерей части событий у краёв. Jitter возвращается долей; умножайте на 100 для процентов. Если валидных периодов недостаточно, возможен undefined. Для сравнений используйте одинаковые метод и ограничения, проверяйте импульсы на осциллограмме.'),
    para('A 200 Hz tone has a 0.005 s period and roughly 200 points, with some edge losses. Jitter is returned as a fraction; multiply by 100 for percent. Too few valid periods can produce undefined. Use identical methods and constraints for comparisons and inspect pulse locations against the waveform.')),
    ('Почему нельзя напрямую сравнивать jitter при разных ограничениях допустимого периода?', 'Why should jitter values from different allowed-period limits not be compared directly?'),
    ('Ограничения меняют набор включённых периодов и соседних пар. Изменяется сама выборка, по которой рассчитан показатель.', 'The limits change which periods and neighbouring pairs are included, so the underlying measurement sample changes.'))

add('19-manipulation', ('Изменение F0 и ресинтез', 'Changing F0 and resynthesizing'), '2, 5; практическое расширение / practical extension', (
    para('Для независимого управления F0 используйте Manipulation и PitchTier. Простое изменение частоты дискретизации одновременно меняет длительность и частоты всех компонентов. PSOLA-ресинтез решает другую задачу: меняет периодичность с меньшим влиянием на длительность и спектральную огибающую, хотя артефакты возможны.') + para('Объект PitchTier нужно извлечь, изменить и явно вернуть в Manipulation. Выделение двух объектов выполняется через два ID. Это пример команды, доступной только для сочетания классов.'),
    para('Use Manipulation and PitchTier to control F0 separately. Simply changing the sampling rate changes duration and all component frequencies together. PSOLA resynthesis addresses a different task: altering periodicity with less effect on duration and spectral envelope, although artifacts are possible.') + para('Extract the PitchTier, modify it, and explicitly replace it in Manipulation. Select the two objects by ID. This demonstrates a command available only for a particular class combination.')),
    VOWEL + '''local manipulation = praat.call("To Manipulation", 0.01, 75, 500)[1]
local tier = praat.call("Extract pitch tier")[1]
praat.call("Multiply frequencies", 0, 0, 1.2)
praat.select(manipulation, tier)
praat.call("Replace pitch tier")
praat.select(manipulation)
local result = praat.call("Get resynthesis (overlap-add)")[1]
praat.call("Save as WAV file", "lesson-resynthesized.wav")
print("duration_s", praat.call("Get total duration"))
''', (
    para('Длительность остаётся около 1 с; целевая F0 повышается с 125 примерно до 150 Гц. Прослушайте оригинал и результат, затем независимо измерьте Pitch. Не проверяйте успех только по значению множителя в коде.'),
    para('Duration stays near 1 s; target F0 rises from 125 to about 150 Hz. Listen to the original and result, then independently measure Pitch. Do not verify success solely from the multiplier written in the code.')),
    ('Как повысить F0 на 3 полутона?', 'How do you raise F0 by 3 semitones?'),
    ('Используйте множитель <code>2^(3/12)</code>, около 1.1892. Сохраните длительность и сравните измеренную F0 до и после.', 'Use the factor <code>2^(3/12)</code>, approximately 1.1892. Preserve duration and compare measured F0 before and after.'))

add('20-dtw', ('MFCC и динамическое выравнивание', 'MFCC and dynamic time warping'), '17', (
    para('MFCC сжимают локальную спектральную форму в коэффициенты. Это признаки, а не формантные частоты. DTW сопоставляет временные последовательности с разной скоростью, находя путь через матрицу локальных различий. Малое расстояние само по себе не доказывает одинаковую фонетическую категорию.') + para('Сначала используйте самосравнение одного гласного: это проверяет весь маршрут без корпуса. To MFCC принимает число коэффициентов, окно, шаг, первую частоту фильтра, расстояние между фильтрами и максимум; последние три величины заданы в mel. Нулевой максимум выбирает доступный диапазон.'),
    para('MFCC compress local spectral shape into coefficients. They are features, not formant frequencies. DTW aligns time sequences at different speaking rates by finding a path through a local-distance matrix. A small distance alone does not establish the same phonetic category.') + para('Start by comparing a vowel with a copy of itself: this checks the entire workflow without a corpus. To MFCC takes coefficient count, window length, time step, first filter frequency, filter spacing, and maximum frequency; the last three use mel. A zero maximum selects the available range.')),
    VOWEL + '''local a = praat.call("To MFCC", 12, 0.015, 0.005, 100, 100, 0)[1]
local b = praat.call("Copy", "mfcc_copy")[1]
praat.select(a, b)
local dtw = praat.call("To DTW", 1, 0, 0, 0, 0.056,
    true, true, "no restriction")[1]
local distance = praat.call("Get distance (weighted)")
print("self_weighted_distance", distance)
assert(distance == distance and distance >= 0 and distance < math.huge)
local diagonal = praat.call("Get distance value", 0.5, 0.5)
local mapped = praat.call("Get y time from x time", 0.5)
print("diagonal_distance", diagonal, "mapped_time_s", mapped)
assert(math.abs(diagonal) < 1e-8 and math.abs(mapped-0.5) < 0.01)
''', (
    para('Локальное расстояние в точке (0.5, 0.5) равно нулю, а 0.5 с отображается примерно в 0.5 с. В проверенной сборке Get distance (weighted) для этого самосравнения возвращает около 0.07822, поэтому проверять его на строгое равенство нулю нельзя: это отдельный итоговый показатель найденного пути. Аргументы DTW: вес кепстра, вес log-energy, вес регрессии, вес регрессии log-energy, окно регрессии, совпадение начала, совпадение конца, ограничение наклона. В двух MFCC используйте одинаковые настройки и сопоставимые записи. Порядок выбранных объектов следует порядку Objects, а не обязательно порядку аргументов praat.select; проверяйте, какая запись стала осью x и какая y.'),
    para('Local distance at (0.5, 0.5) is zero, and 0.5 s maps approximately to 0.5 s. In the checked build, Get distance (weighted) returns about 0.07822 for this self-comparison, so do not assert that it is exactly zero: it is a separate summary of the computed path. DTW arguments are cepstral weight, log-energy weight, regression weight, regression log-energy weight, regression window, match starts, match ends, and slope constraint. Use identical MFCC settings and comparable recordings. Selection follows Objects-list order, not necessarily the argument order of praat.select; verify which recording is x and which is y.')),
    ('Сравните два синтеза с разными F2. Какие настройки нужно сохранить вместе с расстоянием?', 'Compare two syntheses with different F2. Which settings should accompany the distance?'),
    ('Параметры MFCC, веса DTW, ограничения пути, обработку громкости и края записей, а также направления x/y. Расстояния с разными настройками не находятся на общей универсальной шкале.', 'MFCC parameters, DTW weights and path constraints, level processing, recording boundaries, and x/y orientation. Distances with different settings do not share a universal scale.'))

add('21-tables', ('Таблицы, строки и экспорт TSV', 'Tables, strings, and TSV export'), '14', (
    para('Для измерений с именами столбцов удобен Table. TableOfReal ориентирован на числовую матрицу; Strings — на последовательность строк; Permutation — на перестановку индексов. Таблица Lua и объект Table в Praat — разные сущности. Не пытайтесь передать таблицу Lua целиком в praat.call.') + para('Пример создаёт Table и заполняет ячейки по одной. Заголовки содержат единицы. TSV легко открыть в табличном редакторе и проверить обычным текстовым просмотром. Для произвольных меток с табуляцией или переводами строк нужен оговорённый способ экранирования.'),
    para('Use Table for measurements with named columns. TableOfReal represents a numeric matrix; Strings represents a sequence of strings; Permutation represents an index permutation. A Lua table and a Praat Table object are different entities. Do not pass a whole Lua table into praat.call.') + para('The example creates a Table and populates individual cells. Headers include units. TSV opens in spreadsheet software and can also be checked as plain text. Arbitrary labels containing tabs or line breaks need an explicit escaping policy.')),
    '''local result = praat.call("Create Table with column names", "measurements",
    3, "token frequency_Hz rms_Pa")[1]
for i, hz in ipairs({100, 200, 400}) do
    praat.select(result)
    praat.call("Set string value", i, "token", "tone_" .. hz)
    praat.call("Set numeric value", i, "frequency_Hz", hz)
    praat.call("Set numeric value", i, "rms_Pa", 0.2/math.sqrt(2))
end
praat.call("Save as tab-separated file", "lesson-table.tsv")
praat.call("Create Strings as tokens", "alpha beta gamma", " ")
local words = praat.call("List all strings")
assert(words[2] == "beta")
print(table.concat(words, ", "))
''', (
    para('Получатся три строки значений и заголовок. Здесь RMS вписан по аналитической формуле для чистого синуса: это явно обозначенные демонстрационные данные. В исследовании заполняйте столбцы фактическими результатами команд и отдельно сохраняйте настройки анализа.'),
    para('The output contains three data rows and a header. Here RMS is entered from the analytic pure-sine formula: these are explicitly demonstration data. In research, populate columns with actual query results and archive analysis settings separately.')),
    ('Добавьте столбец period_s и заполните его.', 'Add a period_s column and populate it.'),
    ('Включите period_s в список столбцов при создании и задавайте <code>praat.call("Set numeric value", i, "period_s", 1/hz)</code>.', 'Include period_s in the creation column list and call <code>praat.call("Set numeric value", i, "period_s", 1/hz)</code>.'))

add('22-errors', ('Ошибки, очистка и состояние сеанса', 'Errors, cleanup, and session state'), '4.12; расширение сборки / build extension', (
    para('Каждый запуск создаёт новое состояние Lua: переменные и require-кэш предыдущего запуска исчезают. Объекты Praat остаются. Ошибка и Stop не откатывают уже выполненные команды и записанные файлы. Поэтому длительный анализ должен иметь явную очистку своих временных объектов.') + para('<code>pcall</code> возвращает true и результаты функции либо false и ошибку. Проверяйте, является ли число конечным, отдельно от успешного выполнения: команда может вернуть NaN без исключения. Следующий приём отслеживает только объекты, созданные между снимком и завершением операции.'),
    para('Every run creates a fresh Lua state: variables and the previous require cache disappear. Praat objects remain. Errors and Stop do not roll back completed commands or written files. A long analysis therefore needs explicit cleanup of its own temporary objects.') + para('<code>pcall</code> returns true plus function results, or false plus an error. Check numeric finiteness separately from execution success: a command may return NaN without raising an exception. The following pattern tracks only objects created between a snapshot and completion of an operation.')),
    '''local before, previous = {}, praat.selected()
for _, object in ipairs(praat.objects()) do before[object.id] = true end
local ok, message = pcall(function()
    praat.call("Create Sound from formula", "temporary", 1, 0, 1, 16000, "0")
    praat.call("This command deliberately does not exist")
end)
local created = {}
for _, object in ipairs(praat.objects()) do
    if not before[object.id] then created[#created+1] = object.id end
end
if #created > 0 then praat.select(created); praat.call("Remove") end
praat.select(previous)
assert(not ok)
print("handled_error", message)
''', (
    note('Такой снимок подходит синхронной операции в текущей сборке, если внутри неё не удаляются исходные объекты. В рабочем коде ещё точнее хранить ID каждого созданного объекта. Stop, перехваченный pcall, тоже является ошибкой Lua; не стройте бесконечный цикл повторов, поглощающий остановку.'),
    note('This snapshot pattern fits a synchronous operation in the current build provided it does not delete pre-existing objects. Production code can be more precise by tracking every created ID directly. Stop caught by pcall is also a Lua error; do not create an endless retry loop that swallows stop requests.')),
    ('Почему успешный pcall не гарантирует корректное измерение F0?', 'Why does a successful pcall not guarantee a valid F0 measurement?'),
    ('На участке без периодичности запрос может законно вернуть undefined/NaN. Проверяйте <code>v == v and math.abs(v) &lt; math.huge</code> и подходящий диапазон.', 'A nonperiodic region can legitimately yield undefined/NaN. Check <code>v == v and math.abs(v) &lt; math.huge</code> and an appropriate range.'))

add('23-batch', ('Обработка папки и командная строка', 'Folder processing and the command line'), '4, 14', (
    para('Подготовка: рядом со скриптом создайте папку <code>corpus</code> и положите в неё WAV. Для первого опыта скопируйте туда lesson-tone.wav и lesson-part.wav из урока 6. Скрипт читает только файлы WAV верхнего уровня, сортирует имена и пишет новый <code>lesson-batch.tsv</code>. Другую папку можно передать первым аргументом командной строки.') + para('Скрипт удаляет созданные при обработке каждого файла объекты и восстанавливает прежнее выделение. Если отдельный файл не читается, строка содержит ERROR и обработка продолжается. Пропуск измерения получает NA. Подробная ошибка выводится в Info.'),
    para('Preparation: create a <code>corpus</code> folder beside the script and place WAV files in it. For a first run, copy lesson-tone.wav and lesson-part.wav from lesson 6. The script reads only top-level WAV files, sorts names, and writes a new <code>lesson-batch.tsv</code>. A different folder can be supplied as the first command-line argument.') + para('The script removes objects created while processing each file and restores previous selection. A file that cannot be read gets an ERROR row and processing continues. A missing measurement gets NA. Detailed errors go to Info.')),
    '''local lfs = require("lfs")
local folder = (arg and arg[1]) or "corpus"
assert(lfs.attributes(folder, "mode") == "directory", "Create the corpus folder first")
local files = {}
for name in lfs.dir(folder) do
    if name:lower():match("%.wav$") and
       lfs.attributes(folder .. "/" .. name, "mode") == "file" then
        files[#files+1] = name
    end
end
table.sort(files)
assert(#files > 0, "No WAV files found")
local function cell(value)
    return tostring(value):gsub("[\\t\\r\\n]", " ")
end
local function number(value)
    if value == value and math.abs(value) < math.huge then
        return string.format("%.6f", value)
    end
    return "NA"
end
local output = assert(io.open("lesson-batch.tsv", "w"))
output:write("file\\tduration_s\\tmedian_F0_Hz\\tstatus\\n")
for _, name in ipairs(files) do
    local before, previous = {}, praat.selected()
    for _, o in ipairs(praat.objects()) do before[o.id] = true end
    local ok, duration, median = pcall(function()
        praat.call("Read from file", folder .. "/" .. name)
        local seconds = praat.call("Get total duration")
        praat.call("To Pitch", 0.01, 75, 500)
        return seconds, praat.call("Get quantile", 0, 0, 0.5, "Hertz")
    end)
    local temporary = {}
    for _, o in ipairs(praat.objects()) do
        if not before[o.id] then temporary[#temporary+1] = o.id end
    end
    if #temporary > 0 then praat.select(temporary); praat.call("Remove") end
    praat.select(previous)
    if not ok and tostring(duration):find("Lua execution stopped", 1, true) then
        output:close()
        error(duration, 0)
    end
    if ok then
        output:write(cell(name), "\\t", number(duration), "\\t", number(median), "\\tOK\\n")
    else
        output:write(cell(name), "\\tNA\\tNA\\tERROR\\n")
        print("ERROR", name, duration)
    end
    output:flush()
end
output:close()
print("files_processed", #files)
''', (
    para('Запуск в PowerShell: <code>&amp; "C:/path/praat-custom.exe" --utf8 --run "C:/course/23-batch.lua" "C:/speech/corpus"</code>. Здесь замените пути на свои. <code>arg[0]</code> содержит путь к скрипту, <code>arg[1]</code> — первую строку аргументов. В редакторе таблица arg может отсутствовать. Выходной TSV остаётся рядом со скриптом, даже если корпус находится в другой папке.') + note('Если файл был успешно прочитан, status=OK означает выполнение процедуры, но не фонетическую достоверность. Очень короткий звук может не подходить для To Pitch. При Stop сохранится частичный TSV. После очистки обработчик повторно выбрасывает штатную ошибку Lua execution stopped, чтобы не проглотить отмену и не перейти к следующему файлу.'),
    para('PowerShell invocation: <code>&amp; "C:/path/praat-custom.exe" --utf8 --run "C:/course/23-batch.lua" "C:/speech/corpus"</code>. Replace paths with your own. <code>arg[0]</code> holds the script path; <code>arg[1]</code> holds the first argument string. The editor may have no arg table. Output TSV stays beside the script even when the corpus lives elsewhere.') + note('For a readable file, status=OK means the procedure executed, not that its phonetic interpretation is valid. Very short sounds may not support To Pitch. Stop leaves a partial TSV. After cleanup, the handler rethrows the host’s Lua execution stopped error so cancellation is not swallowed and processing does not advance to the next file.')),
    ('Добавьте ошибочный файл broken.wav и проверьте, что остальные строки не теряются.', 'Add an invalid broken.wav and verify that other rows are retained.'),
    ('Для broken.wav будет ERROR и подробность в Info, для остальных — результаты. Число строк данных должно совпасть с числом входных WAV. Папки с суффиксом .wav не включаются.', 'broken.wav produces ERROR and details in Info; the remaining files produce results. Data-row count should match input WAV count. Directories with a .wav suffix are excluded.'))

add('24-plots', ('Графики и PNG через praat.plot', 'Plots and PNG with praat.plot'), '1.4; расширение сборки / build extension', (
    para('Встроенный модуль <code>require("praat.plot")</code> строит графики на движке Praat. Это собственный API, не matplotlib. Lua-таблицы передаются методам графика напрямую. Сначала создайте Figure и Axes, затем добавьте данные, подписи и экспортируйте. Методы Axes вызывают через двоеточие, передающее объект как self.') + para('Пример читает небольшую часть Sound и рисует осциллограмму. Для большого файла проредите точки для отображения или используйте команды Praat: миллион отдельных вызовов моста будет медленным. Прореживание картинки не должно незаметно менять данные измерений.'),
    para('The embedded <code>require("praat.plot")</code> module draws with Praat’s rendering engine. It is a custom API, not matplotlib. Pass Lua tables directly to plot methods. Create Figure and Axes, add data and labels, then export. Call Axes methods with a colon, which supplies the object as self.') + para('The example reads a small Sound and draws its waveform. For a large file, reduce display points or use built-in Praat commands: a million individual bridge calls will be slow. Display decimation must not silently change measurement data.')),
    '''local plt = require("praat.plot")
local sound = praat.call("Create Sound from formula", "plot_tone",
    1, 0, 0.02, 16000, "0.2*sin(2*pi*200*x)")[1]
local time, amplitude = {}, {}
for i = 1, praat.call("Get number of samples") do
    time[i] = praat.call("Get time from sample number", i)
    amplitude[i] = praat.call("Get value at sample number", 1, i)
end
local fig, ax = plt.subplots(1, 1, {title="Waveform", width=8, height=4})
ax:plot(time, amplitude, {color="#166a73", linewidth=2, label="200 Hz"})
ax:set_xlabel("Time (s)"):set_ylabel("Amplitude (Pa)"):legend()
fig:savefig("lesson-waveform.png", 150)
-- fig:show() -- Uncomment when running in the graphical application.
''', (
    table(['Возможность', 'Метод'], [['Линии; точки; столбцы', '<code>ax:plot(x,y,opts)</code>; <code>scatter</code>; <code>bar</code>'], ['Границы; сетка', '<code>set_xlim(a,b)</code>; <code>set_ylim(a,b)</code>; <code>grid(false)</code>'], ['Подграфики', '<code>plt.subplots(2,2)</code> даёт плоскую таблицу axes[1]…axes[4]'], ['Окно', '<code>fig:show()</code>; <code>fig:close()</code>'], ['Экспорт', 'только PNG; DPI 100, 150, 300, 600']]) + para('Получится PNG 1200×600 пикселей: 8×4 дюйма при 150 DPI. Для окна раскомментируйте show; он требует GUI. Окно сохраняет копию данных после завершения Lua. Выделение прямоугольника приближает; Ctrl+H возвращает исходный вид. PNG экспортируется из модели, а не из ручного зума окна. Для Picture-окна доступны штатные команды рисования; это отдельный путь визуализации.'),
    table(['Feature', 'Method'], [['Lines; points; bars', '<code>ax:plot(x,y,opts)</code>; <code>scatter</code>; <code>bar</code>'], ['Limits; grid', '<code>set_xlim(a,b)</code>; <code>set_ylim(a,b)</code>; <code>grid(false)</code>'], ['Subplots', '<code>plt.subplots(2,2)</code> returns a flat axes[1]…axes[4] table'], ['Window', '<code>fig:show()</code>; <code>fig:close()</code>'], ['Export', 'PNG only; DPI 100, 150, 300, 600']]) + para('The output is a 1200×600 PNG: 8×4 inches at 150 DPI. Uncomment show for a window; it requires a GUI. The window retains copied data after Lua exits. Drag a rectangle to zoom; Ctrl+H resets the view. PNG export uses the model, not manual window zoom. Native drawing commands provide a separate route through the Picture window.')),
    ('Сделайте два подграфика: осциллограмма и квадраты амплитуды.', 'Create two subplots: waveform and squared amplitude.'),
    ('Используйте <code>local fig, axes = plt.subplots(2,1)</code>; заполните второй массив <code>power[i]=amplitude[i]^2</code>. Единицы второго графика — Па².', 'Use <code>local fig, axes = plt.subplots(2,1)</code>; fill <code>power[i]=amplitude[i]^2</code> for the second plot. Its units are Pa².'))

add('25-heatmap', ('Тепловая карта настоящей спектрограммы', 'A heatmap of an actual spectrogram'), '8; расширение сборки / build extension', (
    para('Тепловую карту можно построить из числовых значений Spectrogram, преобразовав объект в Matrix. Строки Matrix соответствуют частоте, столбцы — времени; в imshow первая строка рисуется снизу. Не переворачивайте её автоматически по привычке работы с изображениями.') + para('Значения спектрограммы — энергетические; для демонстрационной шкалы переводим их как 10·log10(P) с нижним ограничением 10⁻³⁰. Карта имеет собственную нормировку цветов. Это не точное воспроизведение настроек динамического диапазона, предыскажения и автоподстройки штатного окна Praat.'),
    para('Build a heatmap from numeric Spectrogram values by converting it to Matrix. Matrix rows correspond to frequency and columns to time; imshow draws the first row at the bottom. Do not automatically flip it as you might with a raster image.') + para('Spectrogram values are power-related; for a demonstration scale, use 10·log10(P) with a floor of 10⁻³⁰. The heatmap normalizes its colours independently. This does not exactly reproduce the native Praat display’s dynamic-range, pre-emphasis, and autoscaling settings.')),
    '''local plt = require("praat.plot")
local sound = praat.call("Create Sound from formula", "heatmap_chirp",
    1, 0, 0.5, 16000, "0.2*sin(2*pi*(200*x+1800*x*x))")[1]
praat.call("To Spectrogram", 0.01, 4000, 0.005, 50, "Gaussian")
praat.call("To Matrix")
local values = praat.call("Get all values")
for _, row in ipairs(values) do
    for j, power in ipairs(row) do
        row[j] = 10 * math.log(math.max(power, 1e-30), 10)
    end
end
local fig, ax = plt.subplots(1, 1, {title="Spectrogram", width=8, height=4})
ax:imshow(values, {extent={0, 0.5, 0, 4000}})
ax:set_xlabel("Time (s)"):set_ylabel("Frequency (Hz)")
fig:savefig("lesson-spectrogram.png", 150)
''', (
    para('На рисунке должна подниматься полоса от низких к высоким частотам. Extent здесь использует границы области звука для обзорной иллюстрации; точные центры кадров и частотных бинов не обязаны совпасть с краями 0/0.5/0/4000. Для совмещения с измерениями учитывайте координаты Matrix и половины шага ячеек. В модуле пока нет colorbar и общего clim; нельзя сравнивать абсолютную мощность двух карт только по цвету.'),
    para('The image should show a rising band from low to high frequencies. Extent uses the sound-domain boundaries for an overview illustration; exact frame and frequency-bin centres need not coincide with 0/0.5/0/4000 edges. For measurement overlays, use Matrix coordinates and half-cell steps. The module currently has no colour bar or shared clim; colour alone cannot compare absolute power across two heatmaps.')),
    ('Почему две карты разной громкости могут выглядеть почти одинаково?', 'Why can heatmaps of signals at different levels look nearly identical?'),
    ('Каждая imshow нормируется по собственному минимуму и максимуму. Экспортируйте числовые значения или используйте единый согласованный способ отображения.', 'Each imshow normalizes to its own minimum and maximum. Export numeric values or use an explicitly shared display method.'))

add('26-overlay', ('Слои и панель в редакторе звука', 'Overlays and panels in the sound editor'), 'расширение сборки / build extension', (
    para('Этот пример запускается <b>только в графическом Praat</b>. Он создаёт звук, открывает SoundEditor и прикрепляет оценку F0 поверх спектрограммы, а также график в нижней панели. Здесь ID редактора и ID объекта — разные идентификаторы. <code>plt.editors()</code> возвращает снимок открытых поддерживаемых редакторов.') + para('Для слоя x измеряется в секундах, y — в герцах. Убедитесь, что в SoundEditor включена спектрограмма и её диапазон включает 200 Гц. Если редактор уже открыт, выбирайте его по object_id; при нескольких окнах одного объекта проверяйте конкретный editor.id.'),
    para('Run this example <b>only in graphical Praat</b>. It creates a sound, opens SoundEditor, and attaches estimated F0 over the spectrogram plus a lower-panel graph. Editor IDs and object IDs are different identifiers. <code>plt.editors()</code> returns a snapshot of supported open editors.') + para('Overlay x is time in seconds and y is frequency in Hz. Ensure the SoundEditor spectrogram is enabled and its range includes 200 Hz. For an existing window, choose by object_id; if several windows show one object, inspect the specific editor.id.')),
    TONE + '''local plt = require("praat.plot")
local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
local time, hz = {}, {}
for i = 1, praat.call("Get number of frames") do
    local value = praat.call("Get value in frame", i, "Hertz")
    if value == value and value > 0 and value < math.huge then
        time[#time+1] = praat.call("Get time from frame number", i)
        hz[#hz+1] = value
    end
end
assert(#time > 0, "No valid pitch values")
praat.select(sound)
praat.call("View & Edit")
local editor
for _, e in ipairs(plt.editors()) do
    if e.object_id == sound then editor = e; break end
end
assert(editor, "SoundEditor not found")
local overlay, ax = plt.subplots()
ax:scatter(time, hz, {color="#dd5511", size=1.5})
overlay:attach(editor.id, "spectrogram")
local panel, bx = plt.subplots()
bx:plot(time, hz, {color="#166a73"})
bx:set_ylim(150, 250):set_ylabel("F0 (Hz)")
panel:attach(editor.id, "panel")
''', (
    para('Точки не соединены линией, чтобы пропуски не выглядели как измеренная непрерывная F0. На настоящей речи линии через пропуски нужно разбивать на отдельные серии. Слой допускает линии и точки; панель также столбцы и карты. Для встраивания нужна одна Axes. При правке Sound прикрепления сбрасываются; пересчитайте и прикрепите заново. Меню Lua plots скрывает или удаляет слои. <code>plt.detach(editor.id,"all")</code> удаляет оба. Закрытие отдельного окна Figure не удаляет встраивание.'),
    para('Points are not connected, so missing regions are not shown as measured continuous F0. On real speech, split line contours into separate series across gaps. An overlay accepts lines and points; a panel also accepts bars and heatmaps. Attachment requires one Axes. Editing Sound resets attachments: recompute and reattach. Lua plots menus hide or remove layers. <code>plt.detach(editor.id,"all")</code> removes both slots. Closing a standalone Figure window does not remove attachments.')),
    ('После прикрепления измените звук. Почему прежняя траектория исчезает?', 'Edit the sound after attachment. Why does the old contour disappear?'),
    ('Она была статической копией результатов и стала неактуальной. Сброс предотвращает показ старых измерений поверх нового сигнала.', 'It was a static copy of analysis results and is now stale. Resetting prevents old measurements from appearing over a changed signal.'), mode='gui')

add('27-debugger', ('Отладчик и остановка', 'Debugger and stopping execution'), '4.12; расширение сборки / build extension', (
    para('В Windows Lua Editor отладчик останавливается <b>перед</b> выполнением строки. В примере поставьте F9 на строке <code>value = addOne(value)</code>. F5 дойдёт до неё, значение будет 10. F11 войдёт в функцию с x=10; F10 выполнит строку вычисления y. Shift+F11 вернёт управление вызывающей функции. После присваивания value станет 11.') + table(['Клавиша', 'Действие'], [['F5 / Ctrl+R', 'запуск с отладчиком / без отладчика'], ['F9 / F6', 'точка останова / запрос паузы'], ['F10 / F11 / Shift+F11', 'шаг с обходом / вход / выход'], ['F7 / Ctrl+T', 'проверить синтаксис / выполнить выделение'], ['Shift+F5', 'остановить']]),
    para('In the Windows Lua Editor, the debugger stops <b>before</b> executing a line. In the example, press F9 on <code>value = addOne(value)</code>. F5 stops there with value=10. F11 enters the function with x=10; F10 executes the y calculation. Shift+F11 returns to the caller. After assignment, value becomes 11.') + table(['Key', 'Action'], [['F5 / Ctrl+R', 'run with / without debugger'], ['F9 / F6', 'breakpoint / request pause'], ['F10 / F11 / Shift+F11', 'step over / into / out'], ['F7 / Ctrl+T', 'check syntax / run selection'], ['Shift+F5', 'stop']])),
    '''local function addOne(x)
    local y = x + 1
    return y
end
local value = 10
value = addOne(value)
print("final_value", value)
assert(value == 11)
''', (
    para('Variables показывает локальные значения, upvalues и пользовательские глобальные переменные; Call Stack — Lua-кадры; Output / Error — вывод и ошибку. Это ограниченный снимок, не вычисление произвольных выражений: Watch и редактирование значений не реализованы. Точки относятся к текущему файлу и не сохраняются на диск.') + note('Команды praat.call и блокирующие io/os выполняются целиком; остановка сработает после возврата к Lua. В обычном запуске Windows есть окно Stop script. Уже созданные объекты и записанные файлы сохраняются. Ctrl+T выполняет выделение в новом состоянии: local из непопавших в выделение строк недоступен.'),
    para('Variables shows locals, upvalues, and user globals; Call Stack shows Lua frames; Output / Error shows output and errors. This is a bounded snapshot, not arbitrary expression evaluation: Watch and value editing are not implemented. Breakpoints belong to the current file and are not saved to disk.') + note('praat.call and blocking io/os operations execute as whole calls; stopping takes effect after control returns to Lua. Normal Windows runs show a Stop script window. Created objects and written files remain. Ctrl+T executes selection in a new state: locals defined outside the selection are unavailable.')),
    ('Почему точка останова на комментарии не срабатывает?', 'Why does a breakpoint on a comment not stop?'),
    ('Комментарий не выполняется и не создаёт событие строки Lua. Ставьте точку на исполняемой строке.', 'A comment is not executed and produces no executable Lua line event. Put the breakpoint on an executable line.'))

add('28-simulation', ('Моделирование и воспроизводимость', 'Simulation and reproducibility'), '18; A', (
    para('Симуляция позволяет сравнить алгоритм с известными параметрами сигнала. Здесь меняем частоту чистого тона и проверяем оценку F0. Сначала задайте ожидаемый результат, затем выполните анализ, сохраните отклонение и только после этого интерпретируйте. Не оценивайте качество трекера исключительно на чистых синусах.') + para('Для случайных процедур Lua используйте math.randomseed с фиксированным seed. Он не задаёт seed генератора случайных чисел внутри формул Praat. Воспроизводимость также зависит от версии программы, параметров, входных файлов и порядка обработки.'),
    para('Simulation lets you compare an algorithm against known signal parameters. Here we vary pure-tone frequency and inspect F0 estimates. Define the expected result first, run the analysis, record the discrepancy, and then interpret it. Clean sinusoids alone cannot establish tracking quality.') + para('For random Lua procedures, use math.randomseed with a fixed seed. This does not seed Praat’s formula random-number generator. Reproducibility also depends on software version, settings, input files, and processing order.')),
    '''math.randomseed(20261017)
local output = assert(io.open("lesson-simulation.tsv", "w"))
output:write("target_Hz\\testimated_Hz\\terror_Hz\\n")
for _, target in ipairs({100, 150, 200, 300, 400}) do
    local formula = string.format("0.2*sin(2*pi*%.17g*x)", target)
    local sound = praat.call("Create Sound from formula",
        "simulation", 1, 0, 0.5, 16000, formula)[1]
    local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
    local estimate = praat.call("Get quantile", 0, 0, 0.5, "Hertz")
    assert(estimate == estimate and math.abs(estimate-target) < 3)
    output:write(string.format("%.3f\\t%.6f\\t%.6f\\n",
        target, estimate, estimate-target))
    praat.select(sound, pitch)
    praat.call("Remove")
end
output:close()
print("simulation_complete")
''', (
    para('Пять строк должны содержать оценки около заданных частот с ошибкой менее 3 Гц. Это разумный допуск для данного упражнения, не паспортная точность алгоритма. Расширение эксперимента: варьируйте длительность, уровень шума, диапазон F0 и сохраняйте частоту октавных ошибок и долю пропусков.'),
    para('Five rows should show estimates close to the target frequencies with errors below 3 Hz. This is a reasonable tolerance for this exercise, not a general accuracy specification. Extend the experiment by varying duration, noise level, and F0 range, and record octave-error and missing-value rates.')),
    ('Почему одного среднего абсолютного отклонения недостаточно?', 'Why is mean absolute error alone insufficient?'),
    ('Оно скрывает пропуски, октавные ошибки и распределение по условиям. Сохраняйте исходные оценки, число валидных случаев и параметры каждой симуляции.', 'It hides missing estimates, octave errors, and condition-specific patterns. Retain individual estimates, valid-case counts, and each simulation’s settings.'))

add('29-project', ('Итоговый проект: от звука к отчёту', 'Final project: from sound to report'), '4–9, 12–14', (
    para('Соберём полный маршрут: синтез гласного → TextGrid → Pitch и Formant → измерения интервалов → TSV и PNG. Пример автономен и сохраняет входные файлы, чтобы отчёт можно было проверить. Три интервала обозначают части одного устойчивого гласного, а не три разных фонемы.') + para('Сначала выполните код на синтетике. Затем замените только создание входов чтением согласованной пары Sound/TextGrid. Выберите интервальный ярус, политику исключения меток и настройки анализа до массового запуска. При работе с настоящей речью анализируйте также пропуски и качество каждой траектории.'),
    para('Combine the workflow: vowel synthesis → TextGrid → Pitch and Formant → interval measurements → TSV and PNG. The example is self-contained and saves its inputs for inspection. Its three intervals are parts of one steady vowel, not three different phonemes.') + para('Run on synthetic data first. Then replace only input creation with reading a matched Sound/TextGrid pair. Choose the interval tier, label-exclusion policy, and analysis settings before batch processing. On real speech, also inspect missing values and trajectory quality.')),
    VOWEL + '''local plt = require("praat.plot")
praat.call("Save as WAV file", "project-vowel.wav")
local annotation = praat.call("To TextGrid", "segments", "")[1]
praat.call("Insert boundary", 1, 0.33)
praat.call("Insert boundary", 1, 0.66)
for i, label in ipairs({"early", "middle", "late"}) do
    praat.call("Set interval text", 1, i, label)
end
praat.call("Save as text file", "project-vowel.TextGrid")
praat.select(sound)
local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
praat.select(sound)
local formant = praat.call("To Formant (burg)", 0.005, 5, 5500, 0.025, 50)[1]
local function finite(v) return v == v and math.abs(v) < math.huge end
local function value(v) return finite(v) and string.format("%.4f",v) or "NA" end
local output = assert(io.open("project-measurements.tsv", "w"))
output:write("label\\tstart_s\\tend_s\\tduration_s\\tmedian_F0_Hz\\tF1_mid_Hz\\tF2_mid_Hz\\n")
local times, f1s, f2s = {}, {}, {}
praat.select(annotation)
local n = praat.call("Get number of intervals", 1)
for i = 1, n do
    praat.select(annotation)
    local label = praat.call("Get label of interval", 1, i)
    local a = praat.call("Get start time of interval", 1, i)
    local b = praat.call("Get end time of interval", 1, i)
    if label ~= "" and b > a then
        local midpoint = (a+b)/2
        praat.select(pitch)
        local f0 = praat.call("Get quantile", a, b, 0.5, "Hertz")
        praat.select(formant)
        local f1 = praat.call("Get value at time", 1, midpoint, "Hertz", "linear")
        local f2 = praat.call("Get value at time", 2, midpoint, "Hertz", "linear")
        output:write(table.concat({label, value(a), value(b), value(b-a),
            value(f0), value(f1), value(f2)}, "\\t"), "\\n")
        if finite(f1) and finite(f2) then
            times[#times+1], f1s[#f1s+1], f2s[#f2s+1] = midpoint, f1, f2
        end
    end
end
output:close()
assert(#times > 0, "No finite formants for the figure")
local fig, ax = plt.subplots(1, 1, {title="Interval midpoint formants", width=8, height=4})
ax:scatter(times, f1s, {label="F1", color="#166a73", size=2})
ax:scatter(times, f2s, {label="F2", color="#bd5b2e", size=2})
ax:set_xlabel("Time (s)"):set_ylabel("Frequency (Hz)"):legend()
fig:savefig("project-formants.png", 150)
local settings = assert(io.open("project-settings.txt", "w"))
settings:write("Praat Custom; embedded Lua 5.5.1\\n",
    "Pitch: dt=0.01 s; floor=75 Hz; ceiling=500 Hz\\n",
    "Formant burg: dt=0.005 s; max=5; ceiling=5500 Hz; window=0.025 s; preemphasis=50 Hz\\n",
    "Tier=1; exclude empty labels; missing=NA; F0=interval median; F1/F2=midpoint\\n")
settings:close()
praat.select(sound)
print("project_complete", n)
''', (
    para('Получатся <b>пять</b> выходных файлов: project-vowel.wav, project-vowel.TextGrid, project-measurements.tsv, project-formants.png, project-settings.txt. TSV содержит три строки; F0 близка к 125 Гц, F1/F2 — к 700/1200 Гц. На PNG три пары точек. Код намеренно сохраняет объекты в Objects для проверки.'),
    para('The workflow creates <b>five</b> output files: project-vowel.wav, project-vowel.TextGrid, project-measurements.tsv, project-formants.png, and project-settings.txt. TSV contains three rows; F0 is near 125 Hz, and F1/F2 are near 700/1200 Hz. PNG contains three pairs of points. The code deliberately retains Objects for inspection.')),
    ('Адаптируйте проект к двум реальным записям одного гласного. Какие критерии приёмки использовать?', 'Adapt the project to two real recordings of one vowel. What acceptance criteria should you use?'),
    ('Совпадают временные области Sound/TextGrid; число строк соответствует включённым меткам; единицы и NA однозначны; настройки сохранены; границы проверены слухом; F0 и форманты проверены на спектрограмме; повторный запуск даёт тот же результат при тех же входных данных.', 'Sound/TextGrid time domains match; row count matches included labels; units and NA are unambiguous; settings are archived; boundaries are checked by listening; F0 and formants are inspected against the spectrogram; reruns agree for identical inputs.'))

def reference(lang):
    ru = lang == 'ru'
    return table(['Книга Weenink (печатные страницы)' if ru else 'Weenink book (printed pages)', 'Практика Lua' if ru else 'Lua practice'], [
        ['1–4 · pp. 3–100', '01–07, 22–23, 27'], ['5–6 · pp. 103–115', '08–09, 14, 18'],
        ['7–8 · pp. 117–153', '10–12, 25'], ['9 · pp. 155–160', '13–14, 29'],
        ['10–12 · pp. 161–195', '12, 15'], ['13–14 · pp. 197–221', '16, 21, 23, 29'],
        ['15–18 · pp. 225–253', '17–20, 28'], ['A · pp. 255–305', '02, 05, 10, 12, 28'],
        ['B · pp. 307–319', '23, 29'], ['C · pp. 321–324', '02–04, 07'], ['D · p. 325', 'C'],
    ])

CSS = '''
:root{--ink:#172d38;--muted:#536774;--accent:#176b73;--paper:#fffdf8;--line:#d9dfdd;--wash:#eef4f1;--warm:#b85730}
*{box-sizing:border-box}[hidden]{display:none!important}html{scroll-behavior:smooth;scroll-padding-top:28px}body{margin:0;background:var(--paper);color:var(--ink);font:17px/1.7 "Segoe UI",Arial,sans-serif}a{color:var(--accent);text-underline-offset:3px}button,input{font:inherit}button{cursor:pointer}button:focus-visible,a:focus-visible,input:focus-visible,summary:focus-visible{outline:3px solid #c88224;outline-offset:3px}
.rail{position:fixed;inset:0 auto 0 0;width:290px;padding:30px 24px;background:#edf2ef;border-right:1px solid var(--line);overflow:auto;z-index:2}.brand{font-size:12px;font-weight:800;letter-spacing:2px;text-transform:uppercase}.rail h2{font-size:22px;line-height:1.3;margin:12px 0 22px}.rail input{width:100%;border:1px solid #b4c7c4;background:var(--paper);border-radius:6px;padding:9px 10px;font-size:14px}.rail nav{margin-top:15px}.rail nav a{display:block;text-decoration:none;font-size:13px;line-height:1.4;padding:7px 8px;border-radius:4px}.rail nav a:hover{background:#dae8e1}.rail .minor{font-size:12px;line-height:1.5;color:var(--muted);margin-top:22px}.rail button{margin-top:12px;border:1px solid #9aafaa;background:transparent;border-radius:4px;padding:6px 12px;font-size:13px}
main{max-width:1110px;margin-left:290px;padding:65px 64px 90px}.eyebrow{font-size:12px;text-transform:uppercase;letter-spacing:2px;font-weight:800;color:var(--warm)}h1,h2{font-family:Georgia,"Times New Roman",serif;font-weight:500;line-height:1.15}h1{font-size:clamp(40px,4.5vw,66px);letter-spacing:-2px;max-width:820px;margin:18px 0 24px}h2{font-size:34px;letter-spacing:-.6px;margin:14px 0 25px}h3{font-size:20px;line-height:1.4;margin:28px 0 10px}.lead{font-size:21px;line-height:1.6;color:var(--muted);max-width:760px}.meta{display:flex;gap:12px;flex-wrap:wrap;font-size:12px;margin:28px 0}.meta span{border:1px solid #c5d2cf;border-radius:20px;padding:5px 12px}.route{padding:24px 28px;background:var(--ink);color:#fffdf8;border-radius:10px;margin:34px 0}.route a{color:#b3e0d9}.route h3{margin:0 0 10px;color:#e8c58f}.route p{margin:8px 0}.book-section{border-top:1px solid var(--line);padding-top:45px;margin-top:58px}.section-no{font-size:12px;letter-spacing:1.8px;text-transform:uppercase;font-weight:800;color:var(--warm)}p{margin:15px 0}li{margin:9px 0}code{font: .88em/1.5 Consolas,"Courier New",monospace;background:#eaf0ec;border-radius:3px;padding:2px 4px;overflow-wrap:anywhere}pre{margin:0;overflow:auto;padding:22px 24px;background:#122c38;color:#e6f2ee;font:14px/1.65 Consolas,"Courier New",monospace;tab-size:4}pre code{font:inherit;background:none;padding:0;color:inherit;overflow-wrap:normal}.listing{border-radius:9px;overflow:hidden;margin:25px 0;border:1px solid #254651}.listing-head{display:flex;flex-wrap:wrap;align-items:center;gap:8px;padding:9px 14px;background:#254651;color:#e3efec;font:12px/1.5 Consolas,monospace}.listing-head span{margin-right:auto}.listing-head button{border:1px solid #6b8c91;color:#f2faf7;background:transparent;border-radius:4px;padding:4px 9px;font:12px "Segoe UI",sans-serif}.note{padding:17px 22px;border-left:4px solid #be6f3b;background:#f6eee1;border-radius:0 6px 6px 0;margin:25px 0;font-size:15px}.table-wrap{overflow:auto;margin:24px 0}table{border-collapse:collapse;width:100%;font-size:14px;line-height:1.6}th{text-align:left;background:#e9f0eb;color:#284f52}td,th{padding:12px 14px;border-bottom:1px solid #d8e1da;vertical-align:top}td:first-child{min-width:140px}details{background:var(--wash);padding:15px 20px;border-radius:6px;margin:15px 0}summary{font-weight:600;cursor:pointer}details p{font-size:15px}.exercise{border-top:1px dashed #b4c7c1;margin-top:30px;padding-top:6px}.footer{margin-top:65px;padding-top:25px;border-top:1px solid var(--line);font-size:13px;color:var(--muted)}.validation{font-size:13px;color:var(--muted)}.status{min-height:22px;font-size:13px;color:var(--accent)}.mobile-toc{display:none}
.result{margin:28px 0;padding:14px;border:1px solid var(--line);border-radius:8px;background:white;break-inside:avoid}.result img{display:block;width:100%;height:auto}.result figcaption{font-size:13px;line-height:1.6;color:var(--muted);padding:12px 8px 4px}
@media(min-width:1600px){main{margin-left:max(290px,calc((100vw - 1000px)/2));max-width:1060px}.rail{width:310px}}
@media(max-width:980px){.rail{width:235px;padding:22px 16px}main{margin-left:235px;padding:40px 30px}h2{font-size:30px}}
@media(max-width:700px){.rail{position:static;width:auto;border-right:0;border-bottom:1px solid var(--line);padding:20px}.rail h2{margin:8px 0}.rail nav{max-height:220px;overflow:auto}.rail .minor{display:none}main{margin:0;padding:34px 20px 60px}h1{font-size:42px;letter-spacing:-1px}h2{font-size:30px}.lead{font-size:18px}.book-section{margin-top:42px;padding-top:30px}pre{padding:18px 16px;font-size:12px}.route{padding:20px}body{font-size:16px}}
@media print{body{background:white;font:10.5pt/1.5 Georgia,serif;color:#000}.rail,.listing-head button,.status{display:none}main{margin:0;max-width:none;padding:0}h1{font-size:32pt}h2{font-size:22pt;break-after:avoid}h3{break-after:avoid}.book-section{break-before:page;margin-top:0;border:0;padding-top:0}.route{color:#000;background:#eef2ee}.route a,.route h3{color:#000}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#f2f4f2;color:#000;font:8.5pt/1.45 Consolas,monospace;padding:12px}.listing{overflow:visible;border:1px solid #aaa;break-inside:auto}.listing-head{background:#e7e7e7;color:#000}.table-wrap{overflow:visible}tr,aside,details{break-inside:avoid}details>p{display:block!important}details:not([open])>*:not(summary){display:block}a{color:#000}a[href^="https"]::after{content:" (" attr(href) ")";font-size:8pt;overflow-wrap:anywhere}@page{margin:18mm}}
'''

def appendix(lang):
    ru = lang == 'ru'
    if ru:
        return (
            '<section class="book-section" id="reference"><div class="section-no">Приложение A</div><h2>Карта книги и границы курса</h2>' +
            para('Учебная программа опирается на темы David Weenink, <i>Speech Signal Processing with Praat</i>, версия PDF от 14 сентября 2025 года (342 страницы файла). Ниже указаны печатные страницы, а не номера страниц PDF. Это самостоятельный практикум для Lua в Praat Custom: текст, код и задания написаны заново. Он не является переводом книги и не воспроизводит все её теоретические выводы, рисунки и упражнения.') + reference(lang) +
            para('Главы о записи и оборудовании сохраняют значение: Lua не исправляет клиппинг, плохой микрофон, компрессию с потерями и несогласованную запись. Запись и слуховая проверка выполняются средствами Praat и оборудования. Разделы о VowelEditor изучаются вручную; для программного синтеза курс использует KlattGrid. Приложение B книги посвящено расширенной автоматизации, а C — синтаксису Praat; здесь программные конструкции заменены конструкциями Lua. Математические выводы Фурье, LPC, фильтров, нормализации и DTW следует изучать по исходной книге.') +
            '<h3>Источники</h3><ol><li><a href="https://www.fon.hum.uva.nl/david/sspbook/sspbook.pdf">David Weenink — Speech Signal Processing with Praat (PDF)</a>: тематическая основа и теория.</li><li><a href="https://praat.org/manual/">Официальное руководство Praat</a>: справка о командах и параметрах. Для этой сборки при расхождении проверяйте встроенную Help и локальные диалоги.</li><li><a href="https://www.lua.org/manual/5.5/">Lua 5.5 Reference Manual</a>: язык и стандартная библиотека.</li></ol>' +
            para('Сведения о конкретной сборке проверены по docs/lua.md, docs/lua-plot.md, docs/lua-debugger.md, sys/LuaRuntime.cpp и интеграционным тестам репозитория. Локальный контекст: Praat Custom на базе 7.0.02; Lua 5.5.1; LuaFileSystem 1.9.0. Дата подготовки: 17 октября 2026 года. Старые поставки EXE могут иметь другой набор возможностей.') +
            '</section><section class="book-section" id="troubleshooting"><div class="section-no">Приложение B</div><h2>Справочник и типичные затруднения</h2>' +
            table(['Симптом', 'Проверка'], [['Command not available', 'Выбран ли правильный класс/набор объектов? Верно ли английское имя? Это команда Objects или только окна редактора?'], ['Expected number / wrong arguments', 'Сверьте порядок и количество полей диалога; секунды и Гц; true/false; строки вариантов.'], ['attempt to index a number', 'Числовой запрос не возвращает таблицу. [1] нужен для команды создания, а не Get total duration.'], ['attempt to index a nil value', 'Проверьте пустое выделение, отсутствие arg в GUI и забытый local в отдельном запуске выделения.'], ['undefined / NaN / пустой график', 'Проверьте длительность, периодичность, диапазон анализа. Исключите NaN/Inf из массивов графика; NA храните отдельно.'], ['Не найден файл', 'Сохраните скрипт и проверьте его папку, кавычки и расширение. Для lfs/io учитывайте кодовую страницу Windows.'], ['show requires GUI', 'В пакетном режиме используйте savefig; окно открывайте из GUI.'], ['После запуска много объектов', 'Добавьте удаление собственных временных ID. Новое состояние Lua не очищает Objects.'], ['PNG заменён', 'savefig заменяет существующий файл; создайте отдельную папку/имя для следующего опыта.'], ['Stop не действует сразу', 'Дождитесь возврата длительной команды Praat или блокирующего io/os; это выполнение в одном потоке.']]) +
            '<h3>Ограничения моста</h3>' + para('Поддерживаются команды Objects и соответствующие глобальные меню. Общий API команд редакторов, автодополнение, Lua callbacks для графиков и прямое редактирование массивов Sound через таблицы не реализованы. os.exit, подмена debug.sethook и загрузка сторонних нативных Lua DLL отключены; чистые Lua-модули, lfs и praat.plot доступны. Вложенный запуск Run Lua file из Lua запрещён. Стандартные io/os имеют доступ к локальным файлам и процессам; запускайте понятные вам скрипты.') +
            '<h3>Ограничения графиков</h3>' + para('Сетка до 4×4; до 128 серий на Axes и 1 000 000 числовых значений на Figure. Массивы плотные, непустые и конечные; длины x/y совпадают. Матрица прямоугольная, до 1000×1000 в общем лимите. Легенда показывает до 8 серий. Размер Figure 3–24 дюйма; PNG до 40 мегапикселей. Нет 3D, log-осей, PDF/SVG, colorbar, общего clim и анимации. Настройки Axes возвращают Axes; plot/scatter/bar возвращают серию, поэтому не продолжайте после них цепочку set_xlabel.') +
            '<h3>Переход с Praat-скрипта на Lua</h3>' + table(['Praat', 'Lua в этой сборке'], [['<code>writeInfoLine:</code>', '<code>print(...)</code>'], ['<code>selectObject:</code>', '<code>praat.select(id)</code>'], ['<code>sound = Create Sound ...</code>', '<code>local sound = praat.call("Create Sound ...", ...)[1]</code>'], ['<code>duration = Get total duration</code>', '<code>local duration = praat.call("Get total duration")</code>'], ['<code>procedure / endproc</code>', '<code>local function name(...) ... end</code>'], ['<code>for ... endfor</code>', '<code>for i=1,n do ... end</code>'], ['<code>form ... endform</code>', 'таблица параметров или аргументы CLI; прямой замены форм нет'], ['<code>Run Lua file: "demo.lua"</code>', 'команда оболочки Praat, не код Lua']]) +
            '</section><section class="book-section" id="glossary"><div class="section-no">Приложение C</div><h2>Термины и самопроверка</h2>' +
            table(['Термин', 'Значение'], [['sample / frame / bin', 'отсчёт / временной кадр анализа / частотная ячейка'], ['sample rate / pitch / formant', 'дискретизация / основная частота / резонансная частота'], ['floor / ceiling / bandwidth', 'нижняя граница / верхняя граница / ширина полосы'], ['selection / ID', 'текущее выделение / уникальный идентификатор объекта'], ['undefined / NA', 'неопределённое числовое значение / текстовая отметка пропуска'], ['tier / interval / point', 'ярус / интервал / точечное событие']]) +
            steps(['Могу создать и прочитать Sound, получить Fs и длительность.', 'Различаю Lua и формулы Praat; сохраняю ID и явно меняю выделение.', 'Могу получить F0, интенсивность, спектр, спектрограмму и форманты с записанными настройками.', 'Извлекаю интервалы TextGrid и сохраняю измерения с единицами и пропусками.', 'Проверяю синтез, LPC и DTW на контролируемом материале.', 'Обрабатываю папку, изолирую ошибки файлов и очищаю временные объекты.', 'Строю PNG и отличаю статические слои от живого анализа.', 'Проверяю вывод слухом, визуально и повторным запуском.']) + '</section>'
        )
    return (
        '<section class="book-section" id="reference"><div class="section-no">Appendix A</div><h2>Book map and course scope</h2>' +
        para('The curriculum follows topics in David Weenink’s <i>Speech Signal Processing with Praat</i>, PDF dated 14 September 2025 (342 file pages). References below use printed page numbers, not PDF indices. This is an independently written Lua practical course for Praat Custom. Text, code, and exercises are original; it is not a translation and does not reproduce all the book’s derivations, figures, or exercises.') + reference(lang) +
        para('Recording and equipment chapters remain relevant: Lua cannot repair clipping, a poor microphone, lossy compression, or inconsistent recording conditions. Record and perform listening checks with Praat and the equipment. Explore VowelEditor manually; this course uses KlattGrid for programmable synthesis. Book appendix B covers advanced automation, and C describes native Praat syntax; programming constructs are replaced by Lua here. Study the original book for mathematical derivations of Fourier analysis, LPC, filters, normalization, and DTW.') +
        '<h3>Sources</h3><ol><li><a href="https://www.fon.hum.uva.nl/david/sspbook/sspbook.pdf">David Weenink — Speech Signal Processing with Praat (PDF)</a>: curriculum and theory.</li><li><a href="https://praat.org/manual/">Official Praat manual</a>: command and parameter reference. If versions differ, check this build’s Help and local dialogs.</li><li><a href="https://www.lua.org/manual/5.5/">Lua 5.5 Reference Manual</a>: language and standard library.</li></ol>' +
        para('Build-specific information was checked against docs/lua.md, docs/lua-plot.md, docs/lua-debugger.md, sys/LuaRuntime.cpp, and repository integration tests. Local context: Praat Custom based on 7.0.02; Lua 5.5.1; LuaFileSystem 1.9.0. Prepared 17 October 2026. Older executable distributions may offer a different feature set.') +
        '</section><section class="book-section" id="troubleshooting"><div class="section-no">Appendix B</div><h2>Quick reference and troubleshooting</h2>' +
        table(['Symptom', 'Check'], [['Command not available', 'Correct object class/combination selected? Correct English name? Is this an Objects command or editor-only command?'], ['Expected number / wrong arguments', 'Check dialog field order and count, seconds versus Hz, true/false, and option strings.'], ['attempt to index a number', 'A numeric query does not return a table. [1] belongs on a creation result, not Get total duration.'], ['attempt to index a nil value', 'Check empty selection, absent arg in GUI, and locals missing from an independently run selection.'], ['undefined / NaN / empty plot', 'Check duration, periodicity, and analysis range. Exclude NaN/Inf from plot arrays; store NA separately.'], ['File not found', 'Save the script and check its folder, quotes, and extension. Account for the Windows code page with lfs/io.'], ['show requires GUI', 'Use savefig in batch mode; open windows from the GUI.'], ['Too many objects after runs', 'Remove your own temporary IDs. A fresh Lua state does not clear Objects.'], ['PNG overwritten', 'savefig replaces an existing file; use a separate folder/name for another experiment.'], ['Stop is delayed', 'Wait for the active Praat analysis or blocking io/os call to return; execution shares one thread.']]) +
        '<h3>Bridge limitations</h3>' + para('Objects commands and relevant global menus are supported. A general editor-command API, autocomplete, plot Lua callbacks, and direct Sound-array editing through tables are not implemented. os.exit, debug.sethook replacement, and third-party native Lua DLL loading are disabled; pure Lua modules, lfs, and praat.plot are available. Nested Run Lua file calls from Lua are forbidden. Standard io/os can access local files and processes; run scripts whose actions you understand.') +
        '<h3>Plot limits</h3>' + para('Up to a 4×4 grid, 128 series per Axes, and 1,000,000 numeric values per Figure. Arrays must be dense, nonempty, and finite; x/y lengths must match. Matrices must be rectangular, at most 1000×1000 within the global limit. Legends show up to 8 series. Figure dimensions are 3–24 inches; PNGs are limited to 40 megapixels. No 3D, logarithmic axes, PDF/SVG, colour bars, shared clim, or animation. Axes configuration returns Axes; plot/scatter/bar return a series, so do not chain set_xlabel after them.') +
        '<h3>Moving from Praat scripts to Lua</h3>' + table(['Native Praat', 'Lua in this build'], [['<code>writeInfoLine:</code>', '<code>print(...)</code>'], ['<code>selectObject:</code>', '<code>praat.select(id)</code>'], ['<code>sound = Create Sound ...</code>', '<code>local sound = praat.call("Create Sound ...", ...)[1]</code>'], ['<code>duration = Get total duration</code>', '<code>local duration = praat.call("Get total duration")</code>'], ['<code>procedure / endproc</code>', '<code>local function name(...) ... end</code>'], ['<code>for ... endfor</code>', '<code>for i=1,n do ... end</code>'], ['<code>form ... endform</code>', 'settings table or CLI arguments; no direct form replacement'], ['<code>Run Lua file: "demo.lua"</code>', 'a native Praat wrapper command, not Lua code']]) +
        '</section><section class="book-section" id="glossary"><div class="section-no">Appendix C</div><h2>Terms and self-assessment</h2>' +
        table(['Term', 'Meaning'], [['sample / frame / bin', 'waveform value / analysis time frame / frequency cell'], ['sample rate / pitch / formant', 'sampling frequency / fundamental frequency / resonance frequency'], ['floor / ceiling / bandwidth', 'lower limit / upper limit / frequency-band width'], ['selection / ID', 'current active objects / unique object identifier'], ['undefined / NA', 'undefined numeric result / textual missing-value marker'], ['tier / interval / point', 'annotation layer / time span / individual event']]) +
        steps(['I can create/read Sound and query Fs and duration.', 'I distinguish Lua from Praat formulas, retain IDs, and explicitly switch selection.', 'I can measure F0, intensity, spectrum, spectrogram, and formants with recorded settings.', 'I can read TextGrid intervals and export measurements with units and missing values.', 'I can validate synthesis, LPC, and DTW on controlled material.', 'I can process a folder, isolate file errors, and clean temporary objects.', 'I can export PNG and distinguish static overlays from live analysis.', 'I check results by listening, visual inspection, and rerunning.']) + '</section>'
    )

JS = '''
const ru=document.documentElement.lang==='ru';
const status=document.getElementById('status');
document.querySelectorAll('[data-copy]').forEach(button=>button.addEventListener('click',async()=>{
 const code=document.getElementById(button.dataset.copy).textContent;
 try{if(navigator.clipboard&&window.isSecureContext){await navigator.clipboard.writeText(code)}else{
 const area=document.createElement('textarea');area.value=code;area.style.position='fixed';area.style.left='-9999px';document.body.appendChild(area);area.select();const ok=document.execCommand('copy');area.remove();if(!ok)throw Error('copy');}
 button.textContent=ru?'Скопировано':'Copied';setTimeout(()=>button.textContent=ru?'Копировать':'Copy',1800);
 }catch(e){status.textContent=ru?'Выделите текст примера и скопируйте вручную.':'Select the example text and copy it manually.';}
}));
document.querySelectorAll('[data-download]').forEach(button=>button.addEventListener('click',()=>{
 const text=document.getElementById(button.dataset.download).textContent;
 const url=URL.createObjectURL(new Blob([text],{type:'text/plain;charset=utf-8'}));
 const a=document.createElement('a');a.href=url;a.download=button.dataset.filename;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);
}));
document.getElementById('search').addEventListener('input',event=>{
 const query=event.target.value.trim().toLocaleLowerCase();let count=0;
 document.querySelectorAll('nav a').forEach(link=>{const section=document.getElementById(link.hash.slice(1));const match=!query||(section?section.textContent:link.textContent).toLocaleLowerCase().includes(query);link.hidden=!match;if(match)count++;});
 status.textContent=query?(ru?'Найдено разделов: ':'Matching sections: ')+count:'';
});
let printClosed=[];
window.addEventListener('beforeprint',()=>{
 printClosed=[...document.querySelectorAll('details:not([open])')];printClosed.forEach(d=>d.open=true);
});
window.addEventListener('afterprint',()=>{
 printClosed.forEach(d=>d.open=false);printClosed=[];
});
document.getElementById('print').addEventListener('click',()=>window.print());
'''

def illustration(slug, lang):
    figures = {
        '24-plots': ('waveform.png',
                     'Результат примера: четыре периода тона 200 Гц за 0.02 с. PNG получен непосредственно вашей сборкой Praat.',
                     'Example output: four periods of a 200 Hz tone in 0.02 s. This PNG was generated by your Praat build.'),
        '25-heatmap': ('spectrogram.png',
                      'Результат примера: частотный свип от 200 до примерно 2000 Гц. Цвета нормированы внутри этой карты.',
                      'Example output: a frequency sweep from 200 to approximately 2000 Hz. Colours are normalized within this heatmap.'),
        '29-project': ('formants.png',
                      'Результат итогового проекта: F1 и F2 в серединах трёх интервалов синтетического гласного. Это отдельные измерения, а не непрерывная траектория.',
                      'Final-project output: F1 and F2 at the midpoints of three synthetic-vowel intervals. These are individual measurements, not a continuous contour.'),
    }
    if slug not in figures:
        return ''
    filename, ru, en = figures[slug]
    caption = ru if lang == 'ru' else en
    data = base64.b64encode((OUT / 'figures' / filename).read_bytes()).decode('ascii')
    return '<figure class="result"><img loading="lazy" width="1200" height="600" src="data:image/png;base64,' + data + '" alt="' + escape(caption, quote=True) + '"><figcaption>' + caption + '</figcaption></figure>'

def render(lang):
    i = 0 if lang == 'ru' else 1
    ru = i == 0
    title = 'Lua в вашей сборке Praat' if ru else 'Lua in your Praat build'
    nav = ''.join(f'<a href="#{x["slug"]}">{n:02d} · {x["titles"][i]}</a>' for n,x in enumerate(LESSONS,1))
    nav += '<a href="#reference">' + ('A · Карта книги и источники' if ru else 'A · Book map and sources') + '</a>'
    nav += '<a href="#troubleshooting">' + ('B · Справочник и ошибки' if ru else 'B · Reference and troubleshooting') + '</a>'
    nav += '<a href="#glossary">' + ('C · Термины и самопроверка' if ru else 'C · Terms and self-assessment') + '</a>'
    body = ''
    for n, lesson in enumerate(LESSONS, 1):
        body += f'<section class="book-section" id="{lesson["slug"]}" aria-labelledby="h-{lesson["slug"]}"><div class="section-no">' + ('Урок' if ru else 'Lesson') + f' {n:02d} · ' + ('Книга: ' if ru else 'Book: ') + lesson['source'] + f'</div><h2 id="h-{lesson["slug"]}">{lesson["titles"][i]}</h2>' + lesson['intro'][i]
        if lesson['code']:
            codeid = 'code-' + lesson['slug']
            label = 'Только GUI' if lesson['mode'] == 'gui' and ru else 'GUI only' if lesson['mode'] == 'gui' else 'Lua'
            body += f'<div class="listing"><div class="listing-head"><span>{lesson["filename"]} · {label}</span><button type="button" data-copy="{codeid}">' + ('Копировать' if ru else 'Copy') + f'</button><button type="button" data-download="{codeid}" data-filename="{lesson["filename"]}">' + ('Скачать .lua' if ru else 'Download .lua') + f'</button></div><pre><code id="{codeid}" class="language-lua">{escape(lesson["code"])}</code></pre></div>'
        body += illustration(lesson['slug'], lang)
        body += lesson['after'][i]
        body += '<div class="exercise"><h3>' + ('Практика' if ru else 'Exercise') + '</h3><p>' + lesson['tasks'][i] + '</p><details><summary>' + ('Ответ и критерий проверки' if ru else 'Answer and check') + '</summary><p>' + lesson['answers'][i] + '</p></details></div></section>'
    if ru:
        hero = para('Практический учебник: от первого скрипта до анализа речевого корпуса. Те же задачи Praat — управление на Lua, с проверяемыми примерами для вашей сборки.')
        route = '<h3>Как пройти курс</h3>' + para('<b>Начало:</b> 01–07 — язык, объекты, формулы и файлы. <b>Анализ:</b> 08–16 — F0, интенсивность, спектры, разметка и форманты. <b>Дальше:</b> 17–23 — LPC, ресинтез, DTW и корпус. <b>Визуализация и проект:</b> 24–29.') + para('Каждый основной пример — отдельный скрипт. Он содержит своё создание входного звука, если не оговорена подготовка (урок 23) или GUI (урок 26). Сохраняйте примеры в отдельной рабочей папке. Используйте кнопку «Скачать .lua», затем откройте файл в Praat.')
        intro = para('Материал рассчитан на исследователя речи без опыта Lua. Полезно уметь открыть звук в Praat и читать осциллограмму. Рекомендуемый темп — 6–8 занятий с выполнением заданий, а не только чтением кода. Английские имена команд оставлены без перевода: именно их принимает мост Lua.') + note('HTML — учебник, а не среда выполнения Praat. Кнопки копируют и скачивают текст; анализ выполняйте в вашей программе. Документ работает без интернета; внешние ссылки нужны только для чтения источников. Примеры записи файлов при повторном запуске заменяют свои выходные файлы.')
        validation = 'Основные пакетные примеры проверены на локальном dist/praat-custom.exe; пример слоёв 26 проверяется отдельно в GUI. В приложении указаны границы API. Проверка выполнения не заменяет слуховую и визуальную проверку фонетических выводов.'
    else:
        hero = para('A practical course from your first script to a speech-corpus analysis. Familiar Praat tasks, controlled with Lua and illustrated by checkable examples for your build.')
        route = '<h3>Your route through the course</h3>' + para('<b>Start:</b> 01–07 cover language, objects, formulas, and files. <b>Analysis:</b> 08–16 cover F0, intensity, spectra, annotation, and formants. <b>Continue:</b> 17–23 cover LPC, resynthesis, DTW, and corpora. <b>Visualization and project:</b> 24–29.') + para('Each main example is a separate script. It creates its own input sound unless preparation (lesson 23) or a GUI (lesson 26) is specified. Save examples in a dedicated working folder. Use “Download .lua”, then open the file in Praat.')
        intro = para('This course is for speech researchers without Lua experience. Familiarity with opening sounds in Praat and reading waveforms is helpful. Allow 6–8 study sessions including exercises, rather than reading code alone. Commands retain their English names because those are the names accepted by the Lua bridge.') + note('HTML is the textbook, not a Praat execution environment. Buttons copy or download text; run analysis in your application. The document works offline; external links are only for sources. File-writing examples replace their own output files on rerun.')
        validation = 'Main batch examples were checked with local dist/praat-custom.exe; overlay example 26 requires a separate GUI check. Appendices describe API limits. Successful execution does not replace listening and visual validation of phonetic conclusions.'
    return f'''<!doctype html>
<html lang="{lang}"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="description" content="{'Практический учебник Lua для Praat Custom с примерами и заданиями' if ru else 'A practical Lua textbook for Praat Custom with examples and exercises'}"><title>{title} — Praat Custom</title><style>{CSS}</style></head>
<body><aside class="rail"><div class="brand">Praat Custom / Lua 5.5.1</div><h2>{'Практика обработки речи' if ru else 'Speech processing in practice'}</h2><label for="search">{'Поиск по разделам' if ru else 'Search sections'}</label><input id="search" type="search" placeholder="{'Например: форманты' if ru else 'Try: formants'}" aria-describedby="status"><div id="status" class="status" role="status" aria-live="polite"></div><nav aria-label="{'Оглавление' if ru else 'Contents'}">{nav}</nav><button type="button" id="print">{'Печать / PDF' if ru else 'Print / PDF'}</button><p class="minor">{'29 уроков · задания и ответы<br>Автономный HTML · 17.10.2026' if ru else '29 lessons · exercises and answers<br>Standalone HTML · 17 Oct 2026'}<br><a href="lua-praat-{'en' if ru else 'ru'}.html">{'English edition' if ru else 'Русская версия'}</a></p></aside>
<main><header id="top"><div class="eyebrow">{'Учебник / русский выпуск' if ru else 'Textbook / English edition'}</div><h1>{title}</h1><div class="lead">{hero}</div><div class="meta"><span>29 {'уроков' if ru else 'lessons'}</span><span>{'Lua + фонетика' if ru else 'Lua + phonetics'}</span><span>{'От скрипта к корпусу' if ru else 'From script to corpus'}</span></div></header><div class="route">{route}</div>{intro}<p class="validation">{validation}</p>{body}{appendix(lang)}<footer class="footer">{'Самостоятельный практикум для Praat Custom. Тематическая основа — книга David Weenink. Код и пояснения соответствуют локальной сборке на дату подготовки.' if ru else 'An independent practical course for Praat Custom. Curriculum informed by David Weenink’s book. Code and explanations target the local build as of the preparation date.'}<p><a href="#top">{'Вернуться к началу' if ru else 'Back to top'}</a></p></footer></main><script>{JS}</script></body></html>'''

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    examples = OUT / 'examples'
    examples.mkdir(exist_ok=True)
    for lesson in LESSONS:
        if lesson['code']:
            (examples / lesson['filename']).write_text(lesson['code'], encoding='utf-8', newline='\n')
    for lang in ('ru', 'en'):
        (OUT / f'lua-praat-{lang}.html').write_text(render(lang), encoding='utf-8', newline='\n')
    manifest = [{k:x[k] for k in ('slug', 'filename', 'mode')} for x in LESSONS]
    (OUT / 'examples-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8', newline='\n')
    archive = ROOT / 'dist/lua-praat-textbook.zip'
    archive.parent.mkdir(exist_ok=True)
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as bundle:
        for lang in ('ru', 'en'):
            bundle.write(OUT / f'lua-praat-{lang}.html', f'lua-praat-{lang}.html')
        for lesson in LESSONS:
            bundle.write(examples / lesson['filename'], 'examples/' + lesson['filename'])
    print(f'Built {len(LESSONS)} lessons in each language, {len(LESSONS)} Lua examples.')

if __name__ == '__main__':
    main()
