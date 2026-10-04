# Build upstream Lua as C++: exceptions must unwind Praat's RAII objects.
SOURCES := $(filter-out src/lua.c src/luac.c,$(wildcard src/*.c))
OBJECTS := $(SOURCES:.c=.o) ../luafilesystem-1.9.0/src/lfs.o
all: liblua.a
src/%.o: src/%.c $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -x c++ -c $< -o $@
../luafilesystem-1.9.0/src/lfs.o: ../luafilesystem-1.9.0/src/lfs.c ../luafilesystem-1.9.0/src/lfs.h $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -UUNICODE -U_UNICODE -I src -x c++ -c $< -o $@
liblua.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)
clean:
	$(RM) $(OBJECTS) liblua.a
.PHONY: all clean
