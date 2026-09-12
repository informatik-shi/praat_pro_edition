R"LUA(
local native = ...
local M, Figure, Axes = {}, {}, {}
Figure.__index, Axes.__index = Figure, Axes
local palette = {'#1f77b4','#d62728','#2ca02c','#9467bd','#ff7f0e','#17becf'}
function M.figure(options)
    options = options or {}
    return setmetatable({rows=1,cols=1,title=options.title or 'Lua Figure',
        width=options.width or 9,height=options.height or 6,axes={}},Figure)
end
function Figure:add_subplot(rows,cols,index)
    assert(type(rows)=='number' and rows%1==0 and rows>=1 and rows<=4, 'rows: integer 1..4')
    assert(type(cols)=='number' and cols%1==0 and cols>=1 and cols<=4, 'cols: integer 1..4')
    assert(type(index)=='number' and index%1==0 and index>=1 and index<=rows*cols, 'invalid subplot index')
    assert(#self.axes==0 or (self.rows==rows and self.cols==cols), 'subplot grid cannot change')
    for _,a in ipairs(self.axes) do if a.index==index then return a end end
    self.rows,self.cols=rows,cols
    local a=setmetatable({index=index,row=math.floor((index-1)/cols)+1,col=(index-1)%cols+1,
        title='',xlabel='',ylabel='',series={},grid_enabled=true,legend_enabled=false},Axes)
    self.axes[#self.axes+1]=a
    return a
end
function M.subplots(rows,cols,options)
    rows,cols=rows or 1,cols or 1
    local f=M.figure(options)
    f:add_subplot(rows,cols,1) -- validate before iterating over the grid
    local axes={}
    for i=1,rows*cols do axes[i]=f:add_subplot(rows,cols,i) end
    return f, (#axes==1 and axes[1] or axes)
end
local function series(self,kind,x,y,options)
    if y==nil then y=x;x={};for i=1,#y do x[i]=i end end
    options=options or {}
    local s={kind=kind,x=x,y=y,label=options.label or '',
        color=options.color or palette[(#self.series)%#palette+1],
        linewidth=options.linewidth or 1.5,size=options.size or 1.5,width=options.width or 0.8}
    self.series[#self.series+1]=s
    return s
end
function Axes:plot(x,y,options) return series(self,'line',x,y,options) end
function Axes:scatter(x,y,options) return series(self,'scatter',x,y,options) end
function Axes:bar(x,y,options) return series(self,'bar',x,y,options) end
function Axes:imshow(z,options)
    options=options or {}
    self.series[#self.series+1]={kind='heatmap',z=z,extent=options.extent or {0,#z[1],0,#z},label=''}
    return self
end
function Axes:set_title(s) self.title=s;return self end
function Axes:set_xlabel(s) self.xlabel=s;return self end
function Axes:set_ylabel(s) self.ylabel=s;return self end
function Axes:set_xlim(a,b) self.xlim={a,b};return self end
function Axes:set_ylim(a,b) self.ylim={a,b};return self end
function Axes:grid(enabled) self.grid_enabled=enabled~=false;return self end
function Axes:legend(enabled) self.legend_enabled=enabled~=false;return self end
function Axes:clear() self.series={};self.xlim=nil;self.ylim=nil;return self end
function Figure:show() self._id=native.show(self,self._id);return self end
function Figure:savefig(path,dpi) native.save(self,path,dpi or 150);return self end
function Figure:close() if self._id then native.close(self._id);self._id=nil end end
return M
)LUA"
