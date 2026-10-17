local hz, amplitude = 300, 0.15
local formula = string.format("%.17g*sin(2*pi*%.17g*x)", amplitude, hz)
local sound = praat.call("Create Sound from formula",
    "parameterised", 1, 0, 1, 44100, formula)[1]
praat.call("Formula", "self - 0.01")
praat.call("Subtract mean")
print("rms_Pa", praat.call("Get root-mean-square", 0, 0))
praat.call("Create simple Matrix", "matrix_demo", 2, 3, "row*10+col")
local matrix = praat.call("Get all values")
assert(matrix[2][3] == 23)
print("matrix_2_3", matrix[2][3])
