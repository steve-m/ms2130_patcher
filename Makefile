CC = gcc
CFLAGS = -g -Wall

program : ms2130_patch.c
	$(CC) $(CFLAGS) -o ms2130_patch ms2130_patch.c
clean:
	rm -f ms2130_patch
