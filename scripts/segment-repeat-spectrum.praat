audio_file$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010/cta0001.sbl"
start_time = 0.50
end_time = 0.52
raw_sampling_frequency = 22050

# Edit the file path and the two times above. WAV and other formats supported
# by Praat use their own sample rate; .sbl is raw signed 16-bit little-endian.
if start_time < 0 or end_time <= start_time
    exitScript: "Start and end times must satisfy 0 <= start < end."
endif
if not fileReadable (audio_file$)
    exitScript: "Cannot read audio file: ", audio_file$
endif

if endsWith_caseInsensitive (audio_file$, ".sbl")
    Read Sound from raw 16-bit Little Endian file: audio_file$
    original = selected ("Sound")
    Override sampling frequency: raw_sampling_frequency
else
    Read from file: audio_file$
    original = selected ("Sound")
endif

selectObject: original
sound_duration = Get total duration
if end_time > sound_duration
    exitScript: "The requested end time exceeds the sound duration."
endif

segment = Extract part: start_time, end_time, "rectangular", 1.0, "no"
selectObject: segment
Rename: "selected_segment"
segment_samples = Get number of samples
if segment_samples < 1
    exitScript: "The selected interval contains no samples."
endif

# Concatenate the sound with an exact copy of itself until it has >= 2048 samples.
repeated = Copy: "repeated_segment"
repeated_samples = Get number of samples
while repeated_samples < 2048
    selectObject: repeated
    duplicate = Copy: "temporary_copy"
    selectObject: repeated, duplicate
    joined = Concatenate
    removeObject: repeated, duplicate
    repeated = joined
    selectObject: repeated
    repeated_samples = Get number of samples
endwhile

selectObject: repeated
Rename: "repeated_segment"
spectrum = To Spectrum: "yes"
selectObject: spectrum
Rename: "segment_spectrum"

writeInfoLine: "Selected segment: ", segment_samples, " samples."
appendInfoLine: "Repeated segment: ", repeated_samples, " samples."
appendInfoLine: "Objects: original Sound, selected_segment Sound, repeated_segment Sound, segment_spectrum Spectrum."
