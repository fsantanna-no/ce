#LIBS  = -lkernel32 -luser32 -lgdi32 -lopengl32
CFLAGS = -Wall -Wno-switch

SRC=$(wildcard *.c)

tests: $(SRC)
	gcc -g -o $@ $^ $(CFLAGS) $(LIBS)
