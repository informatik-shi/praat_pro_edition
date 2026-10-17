math.randomseed(20261017)
local output = assert(io.open("lesson-simulation.tsv", "w"))
output:write("target_Hz\testimated_Hz\terror_Hz\n")
for _, target in ipairs({100, 150, 200, 300, 400}) do
    local formula = string.format("0.2*sin(2*pi*%.17g*x)", target)
    local sound = praat.call("Create Sound from formula",
        "simulation", 1, 0, 0.5, 16000, formula)[1]
    local pitch = praat.call("To Pitch", 0.01, 75, 500)[1]
    local estimate = praat.call("Get quantile", 0, 0, 0.5, "Hertz")
    assert(estimate == estimate and math.abs(estimate-target) < 3)
    output:write(string.format("%.3f\t%.6f\t%.6f\n",
        target, estimate, estimate-target))
    praat.select(sound, pitch)
    praat.call("Remove")
end
output:close()
print("simulation_complete")
