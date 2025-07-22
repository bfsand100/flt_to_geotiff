# Compiler and flags
CC = gcc
CFLAGS = -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lgdal

# Targets
TARGETS = flt_to_geotiff geotiff_to_flt

all: $(TARGETS)

flt_to_geotiff: flt_to_geotiff.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

geotiff_to_flt: geotiff_to_flt.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
