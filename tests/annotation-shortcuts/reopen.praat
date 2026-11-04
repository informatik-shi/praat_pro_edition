# Run after gui.praat, in a fresh process with the same test INI.
Create Sound from formula: "Annotation_reopen", 1, 0, 0.2, 22050, "0"
sound = selected()
To TextGrid: "words", ""
grid = selected()
plusObject: sound
View & Edit
editor: grid
    key$ = Get keyboard shortcut: "Boundary/Add on selected tier"
    assert key$ = "Ctrl+Shift+Enter"
    key$ = Get keyboard shortcut: "Play/Play or stop"
    assert key$ = "F4"
    key$ = Get keyboard shortcut: "Formants/Get fourth formant"
    assert key$ = "None"
    Close
endeditor
writeFileLine: "../../.local-build/annotation-shortcuts-reopen.ok", "PASS: persisted keys loaded in a fresh process"
