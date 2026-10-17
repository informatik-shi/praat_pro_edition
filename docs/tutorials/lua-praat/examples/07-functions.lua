local settings = {duration_s=0.5, sample_rate_Hz=44100, amplitude=0.1}
local function makeTone(hz, cfg)
    assert(hz > 0 and hz < cfg.sample_rate_Hz/2, "Invalid frequency")
    assert(cfg.duration_s > 0, "Duration must be positive")
    local formula = string.format("%.17g*sin(2*pi*%.17g*x)", cfg.amplitude, hz)
    return praat.call("Create Sound from formula", "tone_" .. hz,
        1, 0, cfg.duration_s, cfg.sample_rate_Hz, formula)[1]
end
local function rms(id)
    praat.select(id)
    return praat.call("Get root-mean-square", 0, 0)
end
for _, hz in ipairs({100, 200, 400}) do
    local id = makeTone(hz, settings)
    print(hz, rms(id))
    praat.select(id)
    praat.call("Remove")
end
