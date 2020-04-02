#LIBS  = -lkernel32 -luser32 -lgdi32 -lopengl32
CFLAGS = -Wall

SRC=$(wildcard *.c)

tests: $(SRC)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)
