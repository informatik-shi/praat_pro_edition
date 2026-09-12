# Changes relative to Praat 7.0.02

- Embedded Lua plot panels and frequency overlays in Sound/LongSound and
  Spectrogram editors. `plt.editors`, `fig:attach`, `plt.detach`, synchronized
  time range, editor hide/remove menus and invalidation after source edits.

- Built-in `require('praat.plot')`: figure/axes API, lines, scatter, bars,
  heatmaps, subplots, labels and legends; independent retained figure windows,
  mouse rectangle zoom, reset and PNG export. No Python or additional DLLs.
  See docs/lua-plot.md for the first-stage API and limits.

- Windows Lua Editor debugger: F9/gutter breakpoints, F5 continue, F6 pause,
  F10 over, F11 into, Shift+F11 out and Shift+F5 stop. Real Lua line hooks,
  locals/upvalues/globals, call stack and error snapshot; folded destinations
  expand automatically. See docs/lua-debugger.md for scope and acceptance.

- Code folding in both Windows script editors: nested block markers, View menu,
  F8 toggle, Ctrl+F8 collapse all, Shift+F8 expand all; original source line numbers
  and automatic expansion at debugger/search destinations.

- Embedded Lua 5.5.1 with a typed Praat command bridge, selection/object API and Lua error tracebacks.
- Separate Lua Editor: Windows syntax colouring, line numbers, F5 run, F7 syntax check, selection run and cooperative Stop.
- Lua source modules, UTF-8 strings, scalar/vector/matrix results; local script trust and host lifecycle protections.
- Lua integration tests and packaged demo; see docs/lua.md for API and current limitations.

- FrequencyTrajectories object: named frequency tracks, CSV/TSV import/export, native Praat serialization.
- Sound + FrequencyTrajectories editor: waveform, cached spectrogram, draggable points, Undo/Redo, playback.
- Formant conversion to editable FrequencyTrajectories; example CSV and interactive demo.

- Windows Script Editor opts into system RichEdit; other editors use their original widgets.
- Visible-line syntax colouring, gutter line numbers, breakpoint marks, execution mark and Ln/Col status.
- Debug menu: F5, Shift+F5, F9, F10, F11, Shift+F11.
- Optional interpreter observer before statements and on unhandled runtime errors.
- Actual procedure depth and frames drive stepping and Call Stack.
- Variables panel reads actual interpreter values, with bounded vector/matrix/string previews.
- Watch expressions use a pure-function allowlist and the existing Formula evaluator.
- Cooperative same-thread pause loop; source editing and unrelated commands are disabled during execution.
- Local per-file breakpoint persistence; native text anchors track edits above breakpoints.
- Runtime error details and navigation, retaining the original Praat error reporting.
- Find/Replace retained; Go to line uses Ctrl+G (Ctrl+L remains available), Find Again uses F3.
- Custom About label preserves original version and authors.
- PowerShell build entry, release directory and debugger integration tests.

See docs/debugger-architecture.md for explicit first-version limitations.
