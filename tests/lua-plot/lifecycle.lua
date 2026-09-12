local plt=require('praat.plot')
local f,a=plt.subplots(1,1,{title='Lua Figure lifecycle: PASS'})
a:plot({1,2,3},{1,4,9})
f:show()
local id=f._id
a:set_title('Updated in the same window')
f:show()
assert(f._id==id)
f:close()
f:close()
f:show()
assert(f._id~=id)
-- This window and its data must remain valid after lua_close.
print('LUA FIGURE LIFECYCLE: PASS')
