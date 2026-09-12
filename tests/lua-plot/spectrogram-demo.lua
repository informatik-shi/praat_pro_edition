local plt=require('praat.plot')
praat.call('Create Sound from formula','Lua spectrogram demo',1,0,1,22050,'0.2*sin(2*pi*1200*x)')
local ids=praat.call('To Spectrogram',.005,5000,.002,20,'Gaussian')
praat.call('View')
local editor
for _,e in ipairs(plt.editors()) do
    if e.object_id==ids[1] and e.type=='SpectrogramEditor' then editor=e;break end
end
assert(editor)
local f,a=plt.subplots()
a:plot({0,1},{1200,1200},{color='#ff3300',linewidth=3})
f:attach(editor.id,'spectrogram')
local panel,b=plt.subplots()
b:scatter({.1,.3,.5,.7,.9},{1,2,4,2,1},{color='#2288cc',size=2})
b:set_title('Spectrogram feature'):set_ylim(0,5)
panel:attach(editor.id,'panel')
print('SPECTROGRAM EMBEDDING: PASS',editor.id)
