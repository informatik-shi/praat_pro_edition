# Build upstream Lua as C++: exceptions must unwind Praat's RAII objects.
SOURCES := $(filter-out src/lua.c src/luac.c,$(wildcard src/*.c))
OBJECTS := $(SOURCES:.c=.o)
all: liblua.a
src/%.o: src/%.c $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -x c++ -c $< -o $@
liblua.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)
clean:
	$(RM) $(OBJECTS) liblua.a
.PHONY: all clean
