CC = gcc

INCS = -I/Library/Frameworks/SDL2.framework/Headers
FWORKS = -L/Library/Frameworks -F/Library/Frameworks -framework SDL2

CFLAGS = -std=gnu89 -Wall

SRC = lmdave.c
OBJ = lmdave

all : $(SRC)
	$(CC) $(SRC) $(INCS) $(FWORKS) $(CFLAGS) -o $(OBJ)
