#LIBS  = -lkernel32 -luser32 -lgdi32 -lopengl32
CFLAGS = -Wall -Wswitch

SRC=$(wildcard *.c)

tests: $(SRC)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)
