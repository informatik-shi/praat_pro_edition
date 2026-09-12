local plt = require('praat.plot')
local fig, ax = plt.subplots(2, 2, {title='Lua: custom visualizations', width=10, height=7})
local x, y, y2 = {}, {}, {}
for i=1,201 do
    x[i]=(i-1)/200*2*math.pi
    y[i]=math.sin(x[i])
    y2[i]=math.cos(x[i])
end
ax[1]:plot(x,y,{label='sin(x)'})
ax[1]:plot(x,y2,{color='#d62728',label='cos(x)'})
ax[1]:set_title('Waveforms'):set_xlabel('Time, s'):set_ylabel('Amplitude'):legend()
ax[2]:scatter({1,2,3,4,5},{400,650,480,900,720},{color='#2ca02c',size=2})
ax[2]:set_title('Frequency observations'):set_xlabel('Time, s'):set_ylabel('Frequency, Hz')
ax[3]:bar({1,2,3,4},{3,7,-2,5},{color='#9467bd',width=.6})
ax[3]:set_title('Measurements'):set_xlabel('Group'):set_ylabel('Value')
local z={}
for r=1,40 do
    z[r]={}
    for c=1,60 do z[r][c]=math.sin(c/7)*math.cos(r/5) end
end
ax[4]:imshow(z,{extent={0,2,0,5000}})
ax[4]:set_title('Heatmap'):set_xlabel('Time, s'):set_ylabel('Frequency, Hz')
-- Data remains available after this Lua run ends.
fig:show()
-- Optional: fig:savefig('custom-figure.png',150)
