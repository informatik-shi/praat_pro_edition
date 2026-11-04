# Preview requested before adding a Pro menu command.
form G1 periods spectra and formant trajectories
    sentence audio_file C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl
    sentence seg_g1_file C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.seg_G1
    sentence smart_tsv_file C:/Users/PC/Documents/job/praatrus/dist/cta0001-g1-formants-smart.tsv
    positive raw_sampling_frequency 22050
    natural channel 1
    sentence seg_encoding UTF-8
    positive minimum_pitch_hz 100
    positive repeat_duration_s 5
    positive maximum_frequency_hz 5000
    real minimum_db -160
    boolean keep_example_objects 1
    positive analysis_window_s 1
    positive requested_time_step_s 0.002
    positive requested_frequency_step_hz 20
    real slice_time_s 1
    sentence formants_tsv_file C:/Users/PC/Documents/job/praatrus/dist/cta0001-g1-burg-formants.tsv
    natural number_of_formants 5
    positive formant_ceiling_hz 5500
    positive formant_window_s 0.025
    positive formant_time_step_s 0.01
    positive formant_preemphasis_hz 50
    boolean create_regular_spectrogram 1
    positive regular_window_s 0.005
    positive regular_time_step_s 0.002
    positive regular_frequency_step_hz 20
endform

# Burg formants are estimated from the same repeated, unwindowed Sound.
# Each G1 interval is one pitch period. Copy its samples cyclically until the
# requested duration, make a Gaussian-windowed Spectrogram, and take its slice
# at the requested time. Its dB values become one SmartSpectrogram frame.
# Set the desired duration in the startup form (default: five seconds).
if not fileReadable (audio_file$) or not fileReadable (seg_g1_file$)
    exitScript: "Cannot read the audio file or its seg_G1 file."
endif
if fileReadable (smart_tsv_file$)
    exitScript: "Output file already exists; choose a new output path: ", smart_tsv_file$
endif
formants_native_file$ = formants_tsv_file$ + ".FrequencyTrajectories"
measurements_file$ = formants_tsv_file$ + ".measurements.tsv"
if fileReadable(formants_tsv_file$) or fileReadable(formants_native_file$) or fileReadable(measurements_file$)
    exitScript: "A formant output file already exists. Choose a new formants TSV path."
endif
if smart_tsv_file$ = formants_tsv_file$ or smart_tsv_file$ = formants_native_file$ or smart_tsv_file$ = measurements_file$
    exitScript: "Spectrum and formant output paths must be different."
endif
if number_of_formants > 10
    exitScript: "Use at most 10 formant trajectories."
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
sound_start = Get start time
sound_end = Get end time
channels = Get number of channels
if channel > channels
    exitScript: "Selected channel exceeds the number of Sound channels."
endif
if maximum_frequency_hz > sampling_frequency / 2
    exitScript: "Maximum frequency exceeds the Nyquist frequency."
endif
if formant_ceiling_hz > sampling_frequency / 2
    exitScript: "Formant ceiling exceeds the input sound Nyquist frequency."
endif
if create_regular_spectrogram and 2 * regular_window_s > sound_duration
    exitScript: "The source sound is too short for the ordinary Gaussian spectrogram window."
endif
repeat_samples = round (repeat_duration_s * sampling_frequency)
if repeat_samples < 8
    exitScript: "Repeat duration must contain at least 8 samples."
endif
actual_repeat_duration_s = repeat_samples / sampling_frequency
# Gaussian uses a physical window twice its nominal window length.
if 2 * analysis_window_s > actual_repeat_duration_s
    exitScript: "Gaussian analysis requires at least twice the window length. Increase repeat duration or reduce analysis window."
endif
if slice_time_s < 0 or slice_time_s > actual_repeat_duration_s
    exitScript: "Slice time must be inside the repeated sound."
endif

if 2 * formant_window_s >= actual_repeat_duration_s
    exitScript: "The repeated sound must exceed two formant window lengths."
endif
formant_measurement_time_s = actual_repeat_duration_s / 2

Read seg files: seg_g1_file$, "G1", seg_encoding$
grid = selected ("TextGrid")
number_of_periods = Get number of intervals: 1

Create FrequencyTrajectories: "g1_burg_formants", sound_start, sound_end, number_of_formants
trajectories = selected("FrequencyTrajectories")
point_counts# = zero#(number_of_formants)
measurement_rows$ = "time_s" + tab$ + "period_start_s" + tab$ + "period_end_s"
for formant_index to number_of_formants
    measurement_rows$ = measurement_rows$ + tab$ + "F" + string$(formant_index) + "_hz"
endfor
measurement_rows$ = measurement_rows$ + newline$

time_row$ = "time_s"
frame_starts$ = ""
frame_ends$ = ""
number_of_frames = 0
number_of_bins = 0
frequency_start_hz = 0
frequency_step_hz = 0

for period_index to number_of_periods
    selectObject: grid
    period_start = Get start time of interval: 1, period_index
    period_end = Get end time of interval: 1, period_index
    period_duration = period_end - period_start

    if period_duration > 0 and period_start >= sound_start and period_end <= sound_end
        period_pitch_hz = 1 / period_duration
        if period_pitch_hz > minimum_pitch_hz
            selectObject: source
            period = Extract part: period_start, period_end, "rectangular", 1, "no"
            period_samples = Get number of samples
            if period_samples >= 2
                repeated = Create Sound from formula: "g1_repeated", 1, 0, actual_repeat_duration_s, sampling_frequency, ~ object [period, channel, 1 + ((col - 1) mod period_samples)]
                selectObject: repeated
                spectrogram = To Spectrogram: analysis_window_s, maximum_frequency_hz, requested_time_step_s, requested_frequency_step_hz, "Gaussian"
                selectObject: spectrogram
                slice = To Spectrum (slice): slice_time_s
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

                if (number_of_frames + 1) * number_of_bins > 20000000
                    exitScript: "Result exceeds the SmartSpectrogram limit of 20 million values. Increase frequency step or reduce maximum frequency."
                endif

                period_center = (period_start + period_end) / 2
                selectObject: repeated
                formant = To Formant (burg): formant_time_step_s, number_of_formants, formant_ceiling_hz, formant_window_s, formant_preemphasis_hz
                measurement_rows$ = measurement_rows$ + string$(period_center) + tab$ + string$(period_start) + tab$ + string$(period_end)
                for formant_index to number_of_formants
                    selectObject: formant
                    formant_hz = Get value at time: formant_index, formant_measurement_time_s, "Hertz", "linear"
                    measurement_rows$ = measurement_rows$ + tab$
                    if formant_hz <> undefined and formant_hz > 0 and formant_hz <= formant_ceiling_hz
                        selectObject: trajectories
                        Add point: formant_index, period_center, formant_hz
                        point_counts#[formant_index] = point_counts#[formant_index] + 1
                        measurement_rows$ = measurement_rows$ + string$(formant_hz)
                    endif
                endfor
                measurement_rows$ = measurement_rows$ + newline$
                if keep_example_objects and number_of_frames = 0
                    selectObject: formant
                    Rename: "g1_formant_example"
                else
                    removeObject: formant
                endif

                if number_of_frames > 0
                    frame_starts$ = frame_starts$ + tab$
                    frame_ends$ = frame_ends$ + tab$
                endif
                frame_starts$ = frame_starts$ + string$(period_start)
                frame_ends$ = frame_ends$ + string$(period_end)
                selectObject: ltas
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

writeFileLine: smart_tsv_file$, "# SmartSpectrogram 2"
appendFileLine: smart_tsv_file$, "# frequency_start_hz=", string$ (frequency_start_hz)
appendFileLine: smart_tsv_file$, "# frequency_step_hz=", string$ (frequency_step_hz)
appendFileLine: smart_tsv_file$, "# values=db"
appendFileLine: smart_tsv_file$, "# frame_start_s=", frame_starts$
appendFileLine: smart_tsv_file$, "# frame_end_s=", frame_ends$
appendFileLine: smart_tsv_file$, time_row$
# Buffer rows to avoid opening the output file separately for every bin.
output_chunk$ = ""
for bin to number_of_bins
    output_chunk$ = output_chunk$ + db_rows$# [bin] + newline$
    if bin mod 64 = 0 or bin = number_of_bins
        appendFile: smart_tsv_file$, output_chunk$
        output_chunk$ = ""
    endif
endfor

Read SmartSpectrogram from CSV/TSV: smart_tsv_file$
smart = selected ("SmartSpectrogram")
Rename: "g1_period_slices"
selectObject: trajectories
Save as TSV file: formants_tsv_file$
Save as binary file: formants_native_file$
writeFile: measurements_file$, measurement_rows$
regular = 0
if create_regular_spectrogram
    selectObject: source
    regular = To Spectrogram: regular_window_s, maximum_frequency_hz, regular_time_step_s, regular_frequency_step_hz, "Gaussian"
    Rename: "g1_regular_spectrogram"
endif
selectObject: source
plusObject: smart
plusObject: trajectories
if regular > 0
    plusObject: regular
endif
writeInfoLine: "G1 intervals: ", number_of_periods
appendInfoLine: "Accepted periods: ", number_of_frames
appendInfoLine: "Repeated duration: ", actual_repeat_duration_s, " s (", repeat_samples, " samples)"
appendInfoLine: "Gaussian window: ", analysis_window_s, " s; requested time step: ", requested_time_step_s, " s; requested frequency step: ", requested_frequency_step_hz, " Hz; slice at: ", slice_time_s, " s"
appendInfoLine: "Spectrum bins: ", number_of_bins, "; first frequency: ", frequency_start_hz, " Hz; step: ", frequency_step_hz, " Hz"
appendInfoLine: "SmartSpectrogram TSV: ", smart_tsv_file$
appendInfoLine: "Formants measured on repeated sound at: ", formant_measurement_time_s, " s"
for formant_index to number_of_formants
    appendInfoLine: "F", formant_index, " points: ", point_counts#[formant_index], "/", number_of_frames
endfor
appendInfoLine: "Formant TSV: ", formants_tsv_file$
appendInfoLine: "Native trajectories: ", formants_native_file$
appendInfoLine: "Measurements (empty = undefined): ", measurements_file$
appendInfoLine: "Select Sound + FrequencyTrajectories + SmartSpectrogram (+ Spectrogram), then View & Edit selected."
appendInfoLine: "Selected layers switches between SmartSpectrogram and the ordinary Spectrogram."
