# Native command smoke check, followed by detailed Lua/API tests.
Read SEG files: "sample.seg_B1", "B1", "UTF-8"
assert numberOfSelected("TextGrid") = 1
name$ = Get tier name: 1
assert name$ = "B1"
Remove
Run Lua file: "test.lua"