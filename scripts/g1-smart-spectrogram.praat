audio_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl"
seg_g1_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.seg_G1"
smart_tsv_file$ = "C:/Users/PC/Documents/job/praatrus/dist/cta0001-smart.tsv"
raw_sampling_frequency = 22050
seg_encoding$ = "UTF-8"
minimum_pitch_hz = 100
frame_samples = 2048

# G1 boundaries delimit candidate pitch periods. The pitch of an interval
# is 1 / its duration in seconds. Only intervals above minimum_pitch_hz enter
# the SmartSpectrogram. For raw .sbl files the sampling rate is overridden.
if not fileReadable (audio_file$) or not fileReadable (seg_g1_file$)
    exitScript: "Cannot read the audio file or its seg_G1 file."
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

Read seg files: seg_g1_file$, "G1", seg_encoding$
grid = selected ("TextGrid")
number_of_periods = Get number of intervals: 1

# A 2048-sample real Spectrum has 1025 frequency bins (0 through Nyquist).
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
                # Copy the samples cyclically and cut the result at exactly 2048.
                frame = Create Sound from formula: "period_frame", 1, 0, frame_samples / sampling_frequency, sampling_frequency, ~ object [period, 1, 1 + ((col - 1) mod period_samples)]
                Multiply by window: "Hamming"
                spectrum = To Spectrum: "yes"
                ltas = To Ltas (1-to-1)

                period_center = (period_start + period_end) / 2
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

# SmartSpectrogram stores irregular frame times and one shared frequency grid.
# The 1-to-1 Ltas values are the same dB values shown for a Spectrum in Praat.
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
selectObject: smart
writeInfoLine: "G1 intervals: ", number_of_periods
appendInfoLine: "Periods above ", minimum_pitch_hz, " Hz: ", number_of_frames
appendInfoLine: "SmartSpectrogram frequency bins: ", number_of_bins
appendInfoLine: "TSV: ", smart_tsv_file$
