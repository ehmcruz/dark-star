# To compile
# make MYGLIB_TARGET_LINUX=1

CPP = g++

TESTS_SRC := $(wildcard examples/*.cpp)

# get my-lib:
# https://github.com/ehmcruz/my-lib
MYLIB = ../my-lib
MYGLIB = ../my-game-lib
THREADPOOLLIB = ../../git-others/thread-pool

WITH_GPROF = #-pg
WITH_OPENMP = #-fopenmp

CPPFLAGS = -std=c++23 -Wall -O3 $(WITH_OPENMP) $(WITH_GPROF) -g -I$(MYLIB)/include -I$(MYGLIB)/include -I$(THREADPOOLLIB)/include -I./include -DMYGLIB_FP_TYPE=float
LDFLAGS = -std=c++23 $(WITH_OPENMP) $(WITH_GPROF)

# ----------------------------------

ifdef MYGLIB_TARGET_LINUX
	CPPFLAGS +=
	LDFLAGS += -lm

	CPPFLAGS += -DMYGLIB_SUPPORT_SDL=1 `pkg-config --cflags sdl2 SDL2_mixer SDL2_image`
	LDFLAGS += `pkg-config --libs sdl2 SDL2_mixer SDL2_image`

	CPPFLAGS += -DMYGLIB_SUPPORT_OPENGL=1
	LDFLAGS += -lGL -lGLEW
endif

# ----------------------------------

# need to add a rule for each .o/.cpp at the bottom
MYLIB_OBJS = ext/memory-pool.o

SRCS := $(wildcard lib/*.cpp)

HEADERS := $(wildcard include/dark-star/*.h) $(wildcard $(MYLIB)/include/my-lib/*.h) $(wildcard $(MYGLIB)/include/my-game-lib/*.h)

SRCS += $(wildcard $(MYGLIB)/src/*.cpp)

SRCS += $(wildcard $(MYGLIB)/src/sdl/*.cpp)
HEADERS += $(wildcard $(MYGLIB)/include/my-game-lib/sdl/*.h)

SRCS += $(wildcard $(MYGLIB)/src/opengl/*.cpp)
HEADERS += $(wildcard $(MYGLIB)/include/my-game-lib/opengl/*.h)

OBJS := $(patsubst %.cpp,%.o,$(SRCS)) $(MYLIB_OBJS)

TESTS_OBJS := $(patsubst %.cpp,%.o,$(TESTS_SRC))

TESTS_BIN := $(patsubst %.cpp,%.exe,$(TESTS_SRC))

# ----------------------------------

%.o: %.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ $<

all: $(TESTS_BIN)
	@echo "Everything compiled! yes!"

examples/test.exe: $(OBJS) $(TESTS_OBJS)
	$(CPP) -o examples/test.exe examples/test.o $(OBJS) $(LDFLAGS)

# ----------------------------------

ext/memory-pool.o: $(MYLIB)/src/memory-pool.cpp $(HEADERS)
	mkdir -p ext
	$(CPP) $(CPPFLAGS) -c -o ext/memory-pool.o $(MYLIB)/src/memory-pool.cpp

# ----------------------------------

clean:
	- rm -rf $(BIN) $(OBJS) $(TESTS_OBJS) $(TESTS_BIN)