form Test G1 preview
    sentence output_prefix .local-build/g1-preview-test
endform
sound_file$ = output_prefix$ + ".wav"
seg_file$ = output_prefix$ + ".seg_G1"
smart_file$ = output_prefix$ + ".tsv"
Create Sound from formula: "g1_test", 1, 0, 0.04, 8000, "0.2*sin(2*pi*200*x)"
Save as WAV file: sound_file$
Remove
writeFileLine: seg_file$, "[PARAMETERS]"
appendFileLine: seg_file$, "SAMPLING_FREQ=8000"
appendFileLine: seg_file$, "BYTE_PER_SAMPLE=2"
appendFileLine: seg_file$, "CODE=0"
appendFileLine: seg_file$, "N_CHANNEL=1"
appendFileLine: seg_file$, "N_LABEL=5"
appendFileLine: seg_file$, "[LABELS]"
appendFileLine: seg_file$, "0,2,"
appendFileLine: seg_file$, "80,2,"
appendFileLine: seg_file$, "160,2,"
appendFileLine: seg_file$, "320,2,"
appendFileLine: seg_file$, "640,2,"
runScript: "../../scripts/g1-period-smart-preview.praat", sound_file$, seg_file$, smart_file$, 22050, 1, "UTF-8", 100, 5, 3000, -160, 0, 1, 0.002, 20, 1
smart = selected("SmartSpectrogram")
selectObject: smart
peak = Get value at time and frequency: 0.0025, 200
off_peak = Get value at time and frequency: 0.0025, 1000
assert peak > off_peak + 20
writeInfoLine: "G1 PREVIEW TEST: PASS"
