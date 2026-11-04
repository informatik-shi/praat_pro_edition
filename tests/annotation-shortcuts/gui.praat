# Run in a separate GUI instance with PRAAT_TEXTGRID_SHORTCUTS_INI pointing into .local-build.
Create Sound from formula: "Annotation_hotkeys_test", 1, 0, 1, 22050, "0.2*sin(2*pi*200*x)"
sound = selected()
To TextGrid: "words phones", ""
grid = selected()
selectObject: sound
plusObject: grid
View & Edit
editor: grid
    Show analyses: "yes", "no", "no", "no", "no", 10
    Select: 0.25, 0.25
    Set keyboard shortcut: "Boundary/Add on selected tier", "Ctrl+Enter"
    key$ = Get keyboard shortcut: "Boundary/Add on selected tier"
    assert key$ = "Ctrl+Enter"
    # Reject conflicts and leave the successfully assigned key intact.
    asserterror conflicts
    Set keyboard shortcut: "Play/Play or stop", "Ctrl+Enter"
    key$ = Get keyboard shortcut: "Boundary/Add on selected tier"
    assert key$ = "Ctrl+Enter"
    Set keyboard shortcut: "Formants/Get fourth formant", "None"
    Set keyboard shortcut: "Play/Play or stop", "F4"
    Set keyboard shortcut: "TextGrid/Select next interval", "Alt+N"
    Reload keyboard shortcuts
    key$ = Get keyboard shortcut: "Play/Play or stop"
    assert key$ = "F4"
    Keyboard shortcuts...
endeditor
# A second editor must load the same persisted assignments.
selectObject: grid
Copy: "Annotation_hotkeys_second"
second = selected()
plusObject: sound
View & Edit
editor: second
    key$ = Get keyboard shortcut: "Boundary/Add on selected tier"
    assert key$ = "Ctrl+Enter"
    key$ = Get keyboard shortcut: "TextGrid/Select next interval"
    assert key$ = "Alt+N"
endeditor
# Changes propagate to editors that are already open.
editor: grid
    Set keyboard shortcut: "Boundary/Add on selected tier", "Ctrl+Shift+Enter"
endeditor
editor: second
    key$ = Get keyboard shortcut: "Boundary/Add on selected tier"
    assert key$ = "Ctrl+Shift+Enter"
endeditor
# Closing an editor also disposes its modeless settings window safely.
editor: grid
    Close
endeditor
editor: second
    key$ = Get keyboard shortcut: "Play/Play or stop"
    assert key$ = "F4"
    Keyboard shortcuts...
endeditor
writeFileLine: "../../.local-build/annotation-shortcuts-gui.ok", "PASS: settings, persistence, conflicts, reload, two editors, propagation, settings lifetime"
