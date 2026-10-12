table_path$ = "C:/Users/PC/Documents/spbu/corpress/corpress/seg-measurements.tsv"
materials_dir$ = "C:/Users/PC/Documents/spbu/corpress/corpress/careva/careva/cta/cta0001-0010"

# Each recording.sbl needs recording.seg_R2 and recording.seg_Y1 in the same
# folder. The Y1 files in this corpus mix UTF-8 and Windows-1251. Reading with
# UTF-8 followed by Windows Latin-1 lets us distinguish decoded Cyrillic from
# legacy bytes before importing each pair with the appropriate SEG encoding.
Text reading settings: "try UTF-8, then Windows Latin-1"

Create Table with column names: "seg_measurements", 0, "filename intonation_unit words duration intensity pitch"
table = selected ("Table")
row = 0

# A Strings object is a growing folder queue; this also handles nested folders.
folders = Create Strings from tokens: "folder_queue", materials_dir$, newline$
folderIndex = 1
selectObject: folders
numberOfFolders = Get number of strings
writeInfoLine: "Scanning ", materials_dir$

while folderIndex <= numberOfFolders
    selectObject: folders
    folderPath$ = Get string: folderIndex

    files$# = fileNames_caseInsensitive$# (folderPath$ + "/*.sbl")
    for fileIndex to size (files$#)
        fileName$ = files$# [fileIndex]
        stem$ = left$ (fileName$, length (fileName$) - 4)
        basePath$ = folderPath$ + "/" + stem$
        soundPath$ = folderPath$ + "/" + fileName$
        segR2Path$ = basePath$ + ".seg_R2"
        segY1Path$ = basePath$ + ".seg_Y1"

        if fileReadable (segR2Path$) and fileReadable (segY1Path$)
            Read Sound from raw 16-bit Little Endian file: soundPath$
            sound = selected ("Sound")
            Override sampling frequency: 22050
            pitch = To Pitch: 0.0, 75, 600

            y1Text$ = readFile$ (segY1Path$)
            segEncoding$ = "Windows-1251"
            if index_regex (y1Text$, "[А-Яа-яЁё]") > 0
                segEncoding$ = "UTF-8"
            endif
            Read SEG files: segR2Path$, "R2 Y1", segEncoding$
            grid = selected ("TextGrid")
            r2Tier = 0
            y1Tier = 0
            numberOfTiers = Get number of tiers
            for tierIndex to numberOfTiers
                tierName$ = Get tier name: tierIndex
                if tierName$ = "R2"
                    r2Tier = tierIndex
                elsif tierName$ = "Y1"
                    y1Tier = tierIndex
                endif
            endfor

            numberOfR2Intervals = Get number of intervals: r2Tier
            numberOfY1Intervals = Get number of intervals: y1Tier
            for r2Index to numberOfR2Intervals
                selectObject: grid
                r2Start = Get start time of interval: r2Tier, r2Index
                r2End = Get end time of interval: r2Tier, r2Index
                intonationUnit$ = Get label of interval: r2Tier, r2Index

                words$ = ""
                for y1Index to numberOfY1Intervals
                    y1Start = Get start time of interval: y1Tier, y1Index
                    y1End = Get end time of interval: y1Tier, y1Index
                    if y1Start >= r2Start and y1End <= r2End
                        word$ = Get label of interval: y1Tier, y1Index
                        if word$ <> ""
                            if words$ <> ""
                                words$ = words$ + " "
                            endif
                            words$ = words$ + word$
                        endif
                    endif
                endfor

                selectObject: sound
                part = Extract part: r2Start, r2End, "rectangular", 1.0, "no"
                intensityDb = Get intensity (dB)
                removeObject: part

                selectObject: pitch
                pitchHz = Get mean: r2Start, r2End, "Hertz"

                selectObject: table
                Append row
                row = row + 1
                Set string value: row, "filename", stem$
                Set string value: row, "intonation_unit", intonationUnit$
                Set string value: row, "words", words$
                Set numeric value: row, "duration", (r2End - r2Start) * 1000
                Set numeric value: row, "intensity", intensityDb
                Set numeric value: row, "pitch", pitchHz
            endfor

            removeObject: sound, pitch, grid
        else
            appendInfoLine: "Skipping ", soundPath$, ": missing .seg_R2 or .seg_Y1"
        endif
    endfor

    subfolders$# = folderNames$# (folderPath$ + "/*")
    for subfolderIndex to size (subfolders$#)
        selectObject: folders
        Insert string: 0, folderPath$ + "/" + subfolders$# [subfolderIndex]
    endfor
    folderIndex = folderIndex + 1
    selectObject: folders
    numberOfFolders = Get number of strings
endwhile

selectObject: table
slash = rindex (table_path$, "/")
if slash > 1
    createFolder: left$ (table_path$, slash - 1)
endif
Save as tab-separated file: table_path$
removeObject: folders
selectObject: table
appendInfoLine: "Saved ", row, " R2 intervals to ", table_path$
