.PHONY: clean

pavolume: pavolume.c
	$(CC) -Wall --std=c99 -O2 $(shell pkg-config --cflags --libs libpulse) -o $@ $^

clean:
	rm -f pavolume
