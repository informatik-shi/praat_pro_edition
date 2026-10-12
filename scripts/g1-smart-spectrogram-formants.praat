audio_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl"
seg_g1_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.seg_G1"
smart_tsv_file$ = "C:/Users/PC/Documents/job/praatrus/dist/cta0001-smart-formants.tsv"
formants_tsv_file$ = "C:/Users/PC/Documents/job/praatrus/dist/cta0001-formants.tsv"
raw_sampling_frequency = 22050
seg_encoding$ = "UTF-8"
minimum_pitch_hz = 100
frame_samples = 2048
number_of_formants = 5
formant_ceiling_hz = 5500
formant_window_s = 0.025
formant_time_step_s = 0.005
formant_preemphasis_hz = 50

# Each G1 interval is a pitch period. Keep periods with 1 / duration > 100 Hz.
# Repeat the unwindowed period to exactly 2048 samples. Analyze formants on
# this repeated frame, then apply the Hamming window for its Spectrum.
if not fileReadable (audio_file$) or not fileReadable (seg_g1_file$)
    exitScript: "Cannot read the audio file or its seg_G1 file."
endif
if frame_samples <> 2048
    exitScript: "This script requires frame_samples = 2048."
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
frame_duration = frame_samples / sampling_frequency

Read seg files: seg_g1_file$, "G1", seg_encoding$
grid = selected ("TextGrid")
number_of_periods = Get number of intervals: 1

Create FrequencyTrajectories: "g1_formants", 0, sound_duration, number_of_formants
trajectories = selected ("FrequencyTrajectories")

# A real 2048-sample Spectrum has 1025 bins from 0 Hz through Nyquist.
number_of_bins = frame_samples / 2 + 1
db_rows$# = empty$# (number_of_bins)
time_row$ = "time_s"
number_of_frames = 0

for period_index to number_of_periods
    selectObject: grid
    period_start = Get start time of interval: 1, period_index
    period_end = Get end time of interval: 1, period_index
    period_duration = period_end - period_start

    if period_duration > 0 and period_start >= 0 and period_end <= sound_duration
        period_pitch_hz = 1 / period_duration
        if period_pitch_hz > minimum_pitch_hz
            selectObject: source
            period = Extract part: period_start, period_end, "rectangular", 1.0, "no"
            period_samples = Get number of samples

            if period_samples > 0
                frame = Create Sound from formula: "period_frame", 1, 0, frame_duration, sampling_frequency, ~ object [period, 1, 1 + ((col - 1) mod period_samples)]
                period_center = (period_start + period_end) / 2

                # Burg formants are measured at the middle of this unwindowed frame.
                selectObject: frame
                To Formant (burg): formant_time_step_s, number_of_formants, formant_ceiling_hz, formant_window_s, formant_preemphasis_hz
                formant = selected ("Formant")
                for formant_index to number_of_formants
                    formant_hz = Get value at time: formant_index, frame_duration / 2, "Hertz", "linear"
                    if formant_hz > 0
                        selectObject: trajectories
                        Add point: formant_index, period_center, formant_hz
                        selectObject: formant
                    endif
                endfor
                removeObject: formant

                selectObject: frame
                Multiply by window: "Hamming"
                spectrum = To Spectrum: "yes"
                ltas = To Ltas (1-to-1)

                time_row$ = time_row$ + tab$ + string$ (period_center)
                for bin to number_of_bins
                    value_db$ = string$ (object [ltas, bin])
                    if number_of_frames = 0
                        db_rows$# [bin] = value_db$
                    else
                        db_rows$# [bin] = db_rows$# [bin] + tab$ + value_db$
                    endif
                endfor
                number_of_frames = number_of_frames + 1
                removeObject: frame, spectrum, ltas
            endif
            removeObject: period
        endif
    endif
endfor

if number_of_frames = 0
    exitScript: "No G1 periods with pitch above ", minimum_pitch_hz, " Hz were found."
endif

# Write one dB value per frequency bin and frame, with period midpoints as times.
writeFileLine: smart_tsv_file$, "# SmartSpectrogram 1"
appendFileLine: smart_tsv_file$, "# frequency_start_hz=0"
appendFileLine: smart_tsv_file$, "# frequency_step_hz=", string$ (sampling_frequency / frame_samples)
appendFileLine: smart_tsv_file$, "# values=db"
appendFileLine: smart_tsv_file$, time_row$
for bin to number_of_bins
    appendFileLine: smart_tsv_file$, db_rows$# [bin]
endfor

Read SmartSpectrogram from CSV/TSV: smart_tsv_file$
smart = selected ("SmartSpectrogram")
Rename: "g1_period_spectrogram"
selectObject: trajectories
Save as TSV file: formants_tsv_file$
selectObject: source
plusObject: smart
plusObject: trajectories
writeInfoLine: "G1 intervals: ", number_of_periods
appendInfoLine: "Frames above ", minimum_pitch_hz, " Hz: ", number_of_frames
appendInfoLine: "Spectrum bins per frame: ", number_of_bins
appendInfoLine: "Formant tracks: ", number_of_formants
appendInfoLine: "Formant TSV: ", formants_tsv_file$
appendInfoLine: "Select Sound + SmartSpectrogram + FrequencyTrajectories, then View & Edit."
