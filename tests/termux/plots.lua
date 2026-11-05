local fig, ax = require('praat.plot').subplots(1, 1, {title='tablet_lua_plot'})
ax:plot({0, 0.25, 0.5, 0.75, 1}, {0, 1, 0, -1, 0})
ax:set_xlabel('Time, s'):set_ylabel('Amplitude')
fig:show()
