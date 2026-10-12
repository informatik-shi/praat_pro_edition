Create Sound from formula: "combined_three", 1, 0, 1, 16000, "0.2*sin(2*pi*200*x)"
sound = selected ("Sound")
Create TextGrid: 0, 1, "words", ""
grid = selected ("TextGrid")
Create FrequencyTrajectories: "combined_three", 0, 1, 2
tracks = selected ("FrequencyTrajectories")
Add point: 1, 0.2, 300
Add point: 1, 0.7, 500
selectObject: sound
plusObject: grid
plusObject: tracks
View & Edit selected
