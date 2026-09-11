# Changes relative to Praat 7.0.02

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
