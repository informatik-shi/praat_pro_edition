# Batch integration test. Run from repository root with --FULL-TRUST.
asserterror unexpected symbol
Run Lua file: "syntax-error.lua"
asserterror expected Lua runtime failure
Run Lua file: "runtime-error.lua"
Run Lua file: "test.lua"
assert numberOfSelected() = 0
writeInfoLine: "LUA WRAPPER: PASS"
