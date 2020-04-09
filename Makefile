#LIBS  = -lkernel32 -luser32 -lgdi32 -lopengl32
CFLAGS = -Wall -Wno-switch

SRC=$(wildcard src/*.c)
SRC_TESTS := $(filter-out src/main.c,  $(SRC))
SRC_MAIN  := $(filter-out src/tests.c, $(SRC))

tests: $(SRC_TESTS)
	gcc -g -o $@ $^ $(CFLAGS) $(LIBS)

main: $(SRC_MAIN)
	gcc -g -o ce $^ $(CFLAGS) $(LIBS)
