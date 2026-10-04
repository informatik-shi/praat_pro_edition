# LuaFileSystem 1.9.0

Upstream: https://github.com/lunarmodules/luafilesystem/tree/v1_9_0

`src/lfs.c`, `src/lfs.h`, and `LICENSE` are unmodified files from the
`v1_9_0` tag. Praat links the module into its static Lua library; the build
rule in `external/lua-5.5.1/praat.mk` compiles this C source as C++ and
selects the narrow Windows API expected by upstream LuaFileSystem.
