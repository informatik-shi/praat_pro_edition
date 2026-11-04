# Compare a real period repeated by Concatenate with the generated Smart frame.
form Compare G1 with manual method
    sentence output_prefix C:/Users/PC/Documents/job/praatrus/.local-build/g1-manual-reference
endform
audio_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl"
seg_file$ = output_prefix$ + ".seg_G1"
smart_file$ = output_prefix$ + ".tsv"
writeFileLine: seg_file$, "[PARAMETERS]"
appendFileLine: seg_file$, "SAMPLING_FREQ=22050"
appendFileLine: seg_file$, "BYTE_PER_SAMPLE=2"
appendFileLine: seg_file$, "CODE=0"
appendFileLine: seg_file$, "N_CHANNEL=1"
appendFileLine: seg_file$, "N_LABEL=3"
appendFileLine: seg_file$, "[LABELS]"
appendFileLine: seg_file$, "0,1,"
appendFileLine: seg_file$, "34252,1,"
appendFileLine: seg_file$, "34560,1,"

# Follow the user's rounded extraction boundaries and concatenate actual copies.
Read sbl file: audio_file$
part = Extract part: 0.776689, 0.783673, "rectangular", 1, "no"
n = Get number of samples
while n < 110250
    selectObject: part
    duplicate = Copy: "manual_copy"
    selectObject: part
    plusObject: duplicate
    chain = Concatenate
    removeObject: part, duplicate
    part = chain
    n = Get number of samples
endwhile
selectObject: part
manual_sound = Extract part: 0, 5, "rectangular", 1, "no"
manual_spectrogram = To Spectrogram: 1, 5000, 0.002, 20, "Gaussian"
manual_slice = To Spectrum (slice): 1
reference = To Ltas (1-to-1)
bins = Get number of bins
first_hz = Get frequency from bin number: 1
step_hz = Get bin width

runScript: "../../scripts/g1-period-smart-preview.praat", audio_file$, seg_file$, smart_file$, 22050, 1, "UTF-8", 100, 5, 5000, -160, 1, 1, 0.002, 20, 1
smart = selected("SmartSpectrogram")
selectObject: "Spectrum g1_spectrum_slice_example"
generated_bins = Get number of bins
assert generated_bins = bins
maximum_db_error = 0
for bin to bins
    expected_db = object [reference, bin]
    if expected_db = undefined or expected_db < -160
        expected_db = -160
    endif
    selectObject: smart
    actual_db = Get value at time and frequency: (34252 + 34560) / 88200, first_hz + (bin - 1) * step_hz
    error_db = abs(actual_db - expected_db)
    if error_db > maximum_db_error
        maximum_db_error = error_db
    endif
endfor
assert maximum_db_error < 0.000001
writeInfoLine: "G1 MANUAL REFERENCE: PASS; bins=", bins, "; maximum error (dB)=", maximum_db_error
