# Exact sample windows, FFT bins, multichannel power, conversion and native persistence.
Create Sound from formula: "sample_tone", 1, 0, 0.125, 8192, "0.2*sin(2*pi*512*x)"
sound = selected()
To SpectrogramS: 256, 64, 4096, 256, "square (rectangular)"
assert numberOfSelected("SpectrogramS") = 1
samples = Get analysis length in samples
assert samples = 256
ms = Get analysis length in milliseconds
assert ms = 31.25
fs = Get source sampling frequency
assert fs = 8192
hop = Get time step in samples
assert hop = 64
fft = Get FFT size in samples
assert fft = 256
frames = Get number of frames
assert frames = 13
first = Get time from frame number: 1
assert abs(first - 0.015625) < 1e-12
power = Get power at: first, 512
assert abs(power - 2.56) < 1e-10
original = selected()
To Matrix
m_original = selected()
selectObject: original
To Spectrogram
assert numberOfSelected("Spectrogram") = 1
plain = selected()
p = Get power at: first, 512
assert p = power
To SpectrogramS: 8192, 256, "square (rectangular)"
assert numberOfSelected("SpectrogramS") = 1
back = selected()
p = Get power at: first, 512
assert p = power
To Matrix
m_back = selected()
# Compare all cells through a difference matrix, not just a single peak.
Formula: ~ self - object[m_original,row,col]
maximum = Get maximum
minimum = Get minimum
assert maximum = 0 and minimum = 0
selectObject: original
Save as text file: "../../.local-build/sample-spectrum.SpectrogramS"
Save as binary file: "../../.local-build/sample-spectrum.bin"
Read from file: "../../.local-build/sample-spectrum.SpectrogramS"
samples = Get analysis length in samples
assert samples = 256
p = Get power at: first, 512
assert p = power
Read from file: "../../.local-build/sample-spectrum.bin"
fft = Get FFT size in samples
assert fft = 256
p = Get power at: first, 512
assert p = power
# Last sample of an odd window must contribute; next sample must not leak into it.
Create Sound from formula: "odd_impulse", 1, 0, 34/8192, 8192, "if col=17 then 1 else if col=18 then 2 else 0 fi fi"
odd_sound = selected()
To SpectrogramS: 17, 17, 4096, 32, "square (rectangular)"
ms = Get analysis length in milliseconds
assert abs(ms - 1000*17/8192) < 1e-12
first = Get time from frame number: 1
p = Get power at: first, 0
assert abs(p - 1/17) < 1e-12
second = Get time from frame number: 2
p = Get power at: second, 0
assert abs(p - 4/17) < 1e-12
# Independent Gaussian normalization on the same full 17-sample window.
energy = 0
last_weight = 0
for j to 17
    u = (j-0.5)/17
    weight = (exp(-12*(2*u-1)^2)-exp(-12))/(1-exp(-12))
    energy = energy + weight^2
    last_weight = weight
endfor
selectObject: odd_sound
To SpectrogramS: 17, 17, 4096, 32, "Gaussian"
first = Get time from frame number: 1
p = Get power at: first, 0
assert abs(p-last_weight^2/energy) < 1e-14
# Stereo channels in opposite phase must average powers, not cancel amplitudes.
Create Sound from formula: "stereo", 2, 0, 0.125, 8192, "if row=1 then 0.2*sin(2*pi*512*x) else -0.2*sin(2*pi*512*x) fi"
stereo = selected()
To SpectrogramS: 256, 64, 4096, 0, "square (rectangular)"
first = Get time from frame number: 1
p = Get power at: first, 512
assert abs(p - power) < 1e-10
selectObject: stereo
asserterror at least the window length
To SpectrogramS: 256, 64, 4096, 128, "Gaussian"
selectObject: stereo
asserterror Nyquist
To SpectrogramS: 256, 64, 5000, 256, "Gaussian"
selectObject: stereo
asserterror exceeds the number
To SpectrogramS: 2048, 64, 4096, 0, "Gaussian"
# Independent DFT sums for arbitrary odd lengths, explicit padding, and radix-2 FFT.
Create Sound from formula: "dft_reference", 1, 0, 15/8192, 8192, "0.1*sin(2*pi*750*x)+0.03*col"
dft_sound = selected()
for variant to 3
    requested = if variant=1 then 0 else if variant=2 then 21 else 32 fi fi
    transform = if requested=0 then 15 else requested fi
    selectObject: dft_sound
    To SpectrogramS: 15, 15, 4096, requested, "square (rectangular)"
    size = Get transform size in samples
    assert size = transform
    method$ = Get transform method
    if variant<3
        assert method$ = "direct DFT"
    else
        assert method$ = "FFT"
    endif
    first = Get time from frame number: 1
    for k from 0 to floor(transform/2)
        real = 0
        imaginary = 0
        for j from 0 to 14
            amplitude = 0.1*sin(2*pi*750*(j+0.5)/8192)+0.03*(j+1)
            real = real + amplitude*cos(2*pi*k*j/transform)
            imaginary = imaginary - amplitude*sin(2*pi*k*j/transform)
        endfor
        expected = (real^2+imaginary^2)/15
        actual = Get power at: first, k*8192/transform
        assert abs(actual-expected)<1e-10
    endfor
endfor
# Even non-power-of-two length includes the true Nyquist bin and survives native files.
Create Sound from formula: "dft_nyquist", 1, 0, 18/8192, 8192, "if col mod 2=0 then 0.2 else -0.2 fi"
To SpectrogramS: 18, 18, 4096, 0, "square (rectangular)"
size = Get transform size in samples
assert size = 18
method$ = Get transform method
assert method$ = "direct DFT"
first = Get time from frame number: 1
actual = Get power at: first, 4096
assert abs(actual-0.72)<1e-12
Save as text file: "../../.local-build/direct-dft.SpectrogramS"
Save as binary file: "../../.local-build/direct-dft.bin"
Read from file: "../../.local-build/direct-dft.SpectrogramS"
method$ = Get transform method
assert method$ = "direct DFT"
actual = Get power at: first, 4096
assert abs(actual-0.72)<1e-12
Read from file: "../../.local-build/direct-dft.bin"
size = Get transform size in samples
assert size = 18
actual = Get power at: first, 4096
assert abs(actual-0.72)<1e-12
# Imported plain values do not acquire invented analysis metadata.
selectObject: plain
To SpectrogramS: 8192, 0, "Gaussian"
samples = Get analysis length in samples
assert samples = 0
ms = Get analysis length in milliseconds
assert ms = undefined
writeInfoLine: "SPECTROGRAM S TESTS: PASS"
