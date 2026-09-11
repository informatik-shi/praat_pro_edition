x = 1
@calculate: 4
x += 10
assert x = 19
procedure calculate: .value
    .local = .value * 2
    @normalize: .local
endproc
procedure normalize: .value
    x += .value
endproc
