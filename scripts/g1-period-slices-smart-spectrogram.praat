audio_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl"
seg_g1_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.seg_G1"
smart_tsv_file$ = "C:/Users/PC/Documents/job/praatrus/dist/cta0001-g1-slices.tsv"
raw_sampling_frequency = 22050
seg_encoding$ = "UTF-8"
minimum_pitch_hz = 100
repeat_duration_s = 0.1
maximum_frequency_hz = 5000
minimum_db = -160
keep_example_objects = 1

# Each G1 interval is one pitch period. Copy its samples cyclically until the
# requested duration, make a Hamming-windowed Spectrogram, and take its centre
# Spectrum slice. The slice's dB values become one SmartSpectrogram frame.
# Set repeat_duration_s above to the desired duration in seconds.
if not fileReadable (audio_file$) or not fileReadable (seg_g1_file$)
    exitScript: "Cannot read the audio file or its seg_G1 file."
endif
if raw_sampling_frequency <= 0 or repeat_duration_s <= 0
    exitScript: "Sampling frequency and repeat duration must be positive."
endif
if maximum_frequency_hz <= 0
    exitScript: "Maximum frequency must be positive."
endif

if endsWith_caseInsensitive (audio_file$, ".sbl")
    Read Sound from raw 16-bit Little Endian file: audio_file$
    source = selected ("Sound")
    Override sampling frequency: raw_sampling_frequency
else
    Read from file: audio_file$
    source = selected ("Sound")
endif
selectObject: source
sampling_frequency = Get sampling frequency
sound_duration = Get total duration
if maximum_frequency_hz > sampling_frequency / 2
    exitScript: "Maximum frequency exceeds the Nyquist frequency."
endif
repeat_samples = round (repeat_duration_s * sampling_frequency)
if repeat_samples < 8
    exitScript: "Repeat duration must contain at least 8 samples."
endif
actual_repeat_duration_s = repeat_samples / sampling_frequency
# Leave one sample of margin on each side for Praat's analysis window.
analysis_window_s = (repeat_samples - 2) / sampling_frequency
requested_frequency_step_hz = 1 / actual_repeat_duration_s
requested_time_step_s = actual_repeat_duration_s / 4

Read seg files: seg_g1_file$, "G1", seg_encoding$
grid = selected ("TextGrid")
number_of_periods = Get number of intervals: 1

time_row$ = "time_s"
number_of_frames = 0
number_of_bins = 0
frequency_start_hz = 0
frequency_step_hz = 0

for period_index to number_of_periods
    selectObject: grid
    period_start = Get start time of interval: 1, period_index
    period_end = Get end time of interval: 1, period_index
    period_duration = period_end - period_start

    if period_duration > 0 and period_start >= 0 and period_end <= sound_duration
        period_pitch_hz = 1 / period_duration
        if period_pitch_hz > minimum_pitch_hz
            selectObject: source
            period = Extract part: period_start, period_end, "rectangular", 1, "no"
            period_samples = Get number of samples
            if period_samples >= 2
                repeated = Create Sound from formula: "g1_repeated", 1, 0, actual_repeat_duration_s, sampling_frequency, ~ object [period, 1, 1 + ((col - 1) mod period_samples)]
                selectObject: repeated
                spectrogram = To Spectrogram: analysis_window_s, maximum_frequency_hz, requested_time_step_s, requested_frequency_step_hz, "Hamming (raised sine-squared)"
                selectObject: spectrogram
                slice = To Spectrum (slice): actual_repeat_duration_s / 2
                selectObject: slice
                ltas = To Ltas (1-to-1)
                current_bins = Get number of bins

                if number_of_frames = 0
                    number_of_bins = current_bins
                    frequency_start_hz = Get frequency from bin number: 1
                    frequency_step_hz = Get bin width
                    db_rows$# = empty$# (number_of_bins)
                elsif current_bins <> number_of_bins
                    exitScript: "Spectrum slices have different frequency grids."
                endif

                period_center = (period_start + period_end) / 2
                time_row$ = time_row$ + tab$ + string$ (period_center)
                for bin to number_of_bins
                    value_db = object [ltas, bin]
                    if value_db = undefined
                        value_db = minimum_db
                    elsif value_db < minimum_db
                        value_db = minimum_db
                    endif
                    value_db$ = string$ (value_db)
                    if number_of_frames = 0
                        db_rows$# [bin] = value_db$
                    else
                        db_rows$# [bin] = db_rows$# [bin] + tab$ + value_db$
                    endif
                endfor
                number_of_frames = number_of_frames + 1

                removeObject: period, ltas
                if keep_example_objects and number_of_frames = 1
                    selectObject: repeated
                    Rename: "g1_repeated_example"
                    selectObject: spectrogram
                    Rename: "g1_spectrogram_example"
                    selectObject: slice
                    Rename: "g1_spectrum_slice_example"
                else
                    removeObject: repeated, spectrogram, slice
                endif
            else
                removeObject: period
            endif
        endif
    endif
endfor

if number_of_frames = 0
    exitScript: "No usable G1 periods above ", minimum_pitch_hz, " Hz were found."
endif

writeFileLine: smart_tsv_file$, "# SmartSpectrogram 1"
appendFileLine: smart_tsv_file$, "# frequency_start_hz=", string$ (frequency_start_hz)
appendFileLine: smart_tsv_file$, "# frequency_step_hz=", string$ (frequency_step_hz)
appendFileLine: smart_tsv_file$, "# values=db"
appendFileLine: smart_tsv_file$, time_row$
for bin to number_of_bins
    appendFileLine: smart_tsv_file$, db_rows$# [bin]
endfor

Read SmartSpectrogram from CSV/TSV: smart_tsv_file$
smart = selected ("SmartSpectrogram")
Rename: "g1_period_slices"
selectObject: source
plusObject: smart
writeInfoLine: "G1 intervals: ", number_of_periods
appendInfoLine: "Accepted periods: ", number_of_frames
appendInfoLine: "Repeated duration: ", actual_repeat_duration_s, " s (", repeat_samples, " samples)"
appendInfoLine: "Spectrum bins: ", number_of_bins, "; first frequency: ", frequency_start_hz, " Hz; step: ", frequency_step_hz, " Hz"
appendInfoLine: "SmartSpectrogram TSV: ", smart_tsv_file$
appendInfoLine: "Select Sound and SmartSpectrogram, then View & Edit."
