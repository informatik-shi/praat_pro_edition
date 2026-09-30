local call=praat.call
local function read(file,levels,encoding)
    local ids=call('Read SEG files',file,levels or '',encoding or 'UTF-8')
    assert(#ids==1)
    praat.select(ids)
    return ids
end
local function remove() call('Remove') end
local function near(x,y) assert(math.abs(x-y)<1e-10,tostring(x)..' != '..tostring(y)) end
local function fail(file,levels,encoding,pattern)
    local before=#praat.objects()
    local ok,err=pcall(read,file,levels,encoding)
    assert(not ok and tostring(err):find(pattern,1,true),tostring(err))
    assert(#praat.objects()==before,'Failed import must not publish a partial TextGrid')
end
read('sample.seg_B1')
assert(call('Get number of tiers')==2)
assert(call('Get tier name',1)=='B1' and call('Get tier name',2)=='R1')
for i=1,2 do local v=call('Is interval tier',i); assert(v==1 or v==true) end
near(call('Get start time'),0); near(call('Get end time'),1)
assert(call('Get number of intervals',1)==2)
assert(call('Get label of interval',1,1)=='Привет, мир')
assert(call('Get label of interval',1,2)=='Ёж — №1')
near(call('Get end time of interval',1,1),.5)
assert(call('Get number of intervals',2)==3)
assert(call('Get label of interval',2,1)=='')
assert(call('Get label of interval',2,2)=='слово')
assert(call('Get label of interval',2,3)=='')
near(call('Get start time of interval',2,2),.1)
near(call('Get end time of interval',2,2),.7)
call('Save as text file','../../.local-build/seg-roundtrip.TextGrid')
remove()
call('Read from file','../../.local-build/seg-roundtrip.TextGrid')
assert(call('Get label of interval',1,1)=='Привет, мир'); remove()
read('sample.seg_G1')
assert(call('Get number of tiers')==2); near(call('Get end time'),1); remove()
read('sample.seg_B1','seg_R1, b1;B1')
assert(call('Get number of tiers')==2 and call('Get tier name',1)=='R1'); remove()
read('sample.seg_B1','G1')
assert(call('Get number of tiers')==1 and call('Get tier name',1)=='G1')
near(call('Get end time'),2); remove()
read('sample.seg_B1','* G1')
assert(call('Get number of tiers')==3); near(call('Get end time'),2); remove()
read('only.seg_G1','G1'); remove()
fail('only.seg_G1','','UTF-8','No SEG tiers selected')
read('ignored.seg_B1'); assert(call('Get number of tiers')==1); remove()
fail('ignored.seg_B1','G1','UTF-8','Expected [PARAMETERS]')
fail('sample.seg_B1','Y4','UTF-8','Requested SEG tier not found')
fail('sample.seg_B1','../B1','UTF-8','Use tier names')
for _,case in ipairs({{'single','At least two'},{'badcount','N_LABEL'},
    {'badrate','must be positive'},{'duplicate','strictly increasing'},
    {'backwards','strictly increasing'},{'overflow','too large'},{'extra','N_LABEL'}}) do
    fail(case[1]..'.seg_B1','','UTF-8',case[2])
end
read('fraction.seg_B1'); near(call('Get start time of interval',1,1),0)
near(call('Get end time'),1); remove()
read('names.seg_Z9'); assert(call('Get tier name',1)=='Z9'); remove()
read('legacy.seg_B1','','Windows-1251')
assert(call('Get label of interval',1,2)=='Ёж — №1'); remove()
fail('legacy.seg_B1','','UTF-8','UTF-8')
for _,case in ipairs({{'unicode','UTF-8'},{'bom','UTF-8'},
    {'utf16le','UTF-16LE'},{'utf16be','UTF-16BE'},{'utf16bom','UTF-16LE'}}) do
    read(case[1]..'.seg_B1','',case[2])
    assert(call('Get label of interval',1,1)=='Привет 𝄞 😀'); remove()
end
for _,case in ipairs({{'koi8','KOI8-R'},{'dos','CP866'}}) do
    read(case[1]..'.seg_B1','',case[2]); assert(call('Get label of interval',1,1)=='Привет ёж'); remove()
end
read('latin.seg_B1','','ISO-8859-1'); assert(call('Get label of interval',1,1)=='café'); remove()
fail('badutf8.seg_B1','','UTF-8','Invalid UTF-8')
fail('badutf16.seg_B1','','UTF-16LE','Truncated character')
fail('utf16bom.seg_B1','','UTF-16BE','wrong byte order')
fail('invalidnul.seg_B1','','UTF-8','NUL character')
assert(#praat.objects()==0)
print('SEG TESTS: PASS')