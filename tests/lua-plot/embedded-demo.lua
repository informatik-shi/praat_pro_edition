local plt=require('praat.plot')
local ids=praat.call('Create Sound from formula','Lua embedded demo',1,0,2,44100,
    '0.15*sin(2*pi*440*x)+0.10*sin(2*pi*(1000*x+250*x^2))')
praat.call('View & Edit')
local editor
for _,e in ipairs(plt.editors()) do
    if e.object_id==ids[1] then editor=e;break end
end
assert(editor,'Open the Sound in View & Edit first')
local overlay,ax=plt.subplots()
local t,f={},{}
for i=1,101 do t[i]=(i-1)/50;f[i]=1000+500*t[i] end
ax:plot(t,f,{color='#ff3300',linewidth=3})
ax:plot({0,2},{440,440},{color='#00bbff',linewidth=2})
overlay:attach(editor.id,'spectrogram')
local panel,p=plt.subplots()
local values={}
for i=1,#t do values[i]=math.sin(2*math.pi*t[i]) end
p:plot(t,values,{color='#7f22cc',linewidth=2})
p:set_title('Custom feature'):set_ylabel('relative units'):set_ylim(-1.2,1.2)
panel:attach(editor.id,'panel')
print('Embedded Lua plots attached to editor',editor.id)
-- Zoom/select in the Sound editor: both plots follow its time range.
-- plt.detach(editor.id) removes both; Lua plots menu can hide/show them.
