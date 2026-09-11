total = 0
for i from 1 to 5
    total += i
endfor
assert total = 15
i = 0
while i < 3
    i += 1
endwhile
repeat
    i -= 1
until i = 0
assert i = 0
