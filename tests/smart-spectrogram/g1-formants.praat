form Test repeated-period formants
    sentence output_prefix C:/Users/PC/Documents/job/praatrus/.local-build/g1-formants-test
endform
sound_file$ = output_prefix$ + ".wav"
seg_file$ = output_prefix$ + ".seg_G1"
smart_file$ = output_prefix$ + ".smart.tsv"
tracks_file$ = output_prefix$ + ".tracks.tsv"
Create Sound from formula: "formants_test", 1, 0, 0.04, 16000, "if x < 0.005 then 0.1*sin(2*pi*200*x)+0.2*sin(2*pi*600*x)+0.3*sin(2*pi*1200*x)+0.15*sin(2*pi*2400*x)+0.1*sin(2*pi*4000*x) else 0.1*sin(2*pi*250*x)+0.2*sin(2*pi*750*x)+0.3*sin(2*pi*1250*x)+0.15*sin(2*pi*2500*x)+0.1*sin(2*pi*4000*x) fi"
Save as WAV file: sound_file$
Remove
writeFileLine: seg_file$, "[PARAMETERS]"
appendFileLine: seg_file$, "SAMPLING_FREQ=16000"
appendFileLine: seg_file$, "BYTE_PER_SAMPLE=2"
appendFileLine: seg_file$, "CODE=0"
appendFileLine: seg_file$, "N_CHANNEL=1"
appendFileLine: seg_file$, "N_LABEL=5"
appendFileLine: seg_file$, "[LABELS]"
appendFileLine: seg_file$, "0,2,"
appendFileLine: seg_file$, "160,2,"
appendFileLine: seg_file$, "288,2,"
appendFileLine: seg_file$, "608,2,"
appendFileLine: seg_file$, "1248,2,"
runScript: "../../scripts/g1-period-smart-formants.praat", sound_file$, seg_file$, smart_file$, 22050, 1, "UTF-8", 100, 5, 5000, -160, 0, 1, 0.002, 20, 1, tracks_file$, 5, 5500, 0.025, 0.01, 50, 1, 0.005, 0.002, 20
assert numberOfSelected("Sound") = 1
assert numberOfSelected("SmartSpectrogram") = 1
assert numberOfSelected("FrequencyTrajectories") = 1
assert numberOfSelected("Spectrogram") = 1
tracks = selected("FrequencyTrajectories")
source = selected("Sound")
selectObject: source
reference_part = Extract part: 0.005, 0.009, "rectangular", 1, "no"
n = Get number of samples
reference_sound = Create Sound from formula: "reference_repeat", 1, 0, 5, 16000, ~ object[reference_part,1,1+((col-1) mod n)]
reference_formant = To Formant (burg): 0.01, 5, 5500, 0.025, 50
checked = 0
for formant_index to 5
    selectObject: reference_formant
    expected = Get value at time: formant_index, 2.5, "Hertz", "linear"
    if expected <> undefined and expected > 0 and expected <= 5500
        selectObject: tracks
        actual = Get value at time: formant_index, 0.007
        assert abs(actual - expected) < 0.000001
        checked = checked + 1
    endif
endfor
assert checked >= 3
Read from file: tracks_file$ + ".FrequencyTrajectories"
value = Get value at time: 1, 0.007
selectObject: tracks
original = Get value at time: 1, 0.007
assert abs(value - original) < 0.000001
Read Table from tab-separated file: tracks_file$ + ".measurements.tsv"
rows = Get number of rows
assert rows = 2
columns = Get number of columns
assert columns = 8
time_1 = Get value: 1, "time_s"
time_2 = Get value: 2, "time_s"
assert abs(time_1 - 0.0025) < 0.000000000001
assert abs(time_2 - 0.007) < 0.000000000001
writeInfoLine: "G1 FORMANTS TEST: PASS; independently checked formants=", checked
