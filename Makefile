CC = gcc
CFLAGS = -O2 -Wall

ENC = encoder.exe
DEC = decoder.exe

IN_FILE = input.txt
CODEBOOK = codebook.csv
ENCODED = encoded.bin
DECODED = decoded.txt

.PHONY: all clean run


all: run

$(ENC): mini_prj_3_encoder_411186016.c
	$(CC) $(CFLAGS) $< -o $@

$(DEC): mini_prj_3_decoder_411186016.c
	$(CC) $(CFLAGS) $< -o $@

run: $(ENC) $(DEC)
	./$(ENC) $(IN_FILE) $(CODEBOOK) $(ENCODED)
	./$(DEC) $(DECODED) $(CODEBOOK) $(ENCODED)
	@diff $(IN_FILE) $(DECODED) && echo "Files identical!" || echo "Files differ!"

clean:
	rm -f $(ENC) $(DEC) $(CODEBOOK) $(ENCODED) $(DECODED)
