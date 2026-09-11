# Interactive: praat-custom.exe --new-send tests/trajectories/demo.praat
Create Sound from formula: "trajectory_demo", 1, 0, 1, 16000, "0.15*sin(2*pi*(500*x+150*x^2))+0.1*sin(2*pi*(1500*x-100*x^2))+0.08*sin(2*pi*2500*x)"
sound = selected()
Read frequency trajectories from CSV/TSV: "tracks.csv"
plusObject: sound
View & Edit
