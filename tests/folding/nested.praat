# Keywords inside strings must not create folds.
procedure total
    sum = 0
    for i from 1 to 3
        sum = sum + i
    endfor
endproc
@total
assert sum = 6
writeInfoLine: "FOLDED PRAAT: PASS ", sum
