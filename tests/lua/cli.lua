assert(type(arg) == "table")
assert(type(arg[0]) == "string" and arg[0]:match("cli%.lua$"))
assert(arg[1] == "hello")
print("LUA CLI: PASS")
