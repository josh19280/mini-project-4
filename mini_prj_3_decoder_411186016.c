#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOLS 256
#define CODE_LEN 7
#define EOF_MARKER_LEN 7

typedef struct {
    char code[16];
    int sym;
} Entry;

int parse_symbol(const char *s) {
    if (strcmp(s, "\\n") == 0) return '\n';
    if (strcmp(s, "\\r") == 0) return '\r';
    if (strcmp(s, "\\\"") == 0) return '\"';
    if (strcmp(s, "\\\\") == 0) return '\\';
    if (strlen(s) == 1) return s[0];
    return 0;
}

int read_bit(FILE *f) {
    static int bits_left = 0;
    static unsigned char byte = 0;
    if (bits_left == 0) {
        int c = fgetc(f);
        if (c == EOF) return -1;
        byte = (unsigned char)c;
        bits_left = 8;
    }
    int bit = (byte >> 7) & 1;
    byte <<= 1;
    bits_left--;
    return bit;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s decoded.txt codebook.csv encoded.bin\n", argv[0]);
        return 1;
    }

    const char *out_fn = argv[1];
    const char *cb_fn = argv[2];
    const char *enc_fn = argv[3];

    FILE *fcb = fopen(cb_fn, "r");
    if (!fcb) { perror("codebook open failed"); return 1; }

    Entry table[SYMBOLS];
    int entry_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), fcb)) {
        char symbol_str[16], code[16];
        unsigned long cnt;
        double prob;
        if (sscanf(line, "\"%[^\"]\",%lu,%lf,\"%[^\"]\"",
                   symbol_str, &cnt, &prob, code) == 4) {
            table[entry_count].sym = parse_symbol(symbol_str);
            strcpy(table[entry_count].code, code);
            entry_count++;
        }
    }
    fclose(fcb);

    FILE *fenc = fopen(enc_fn, "rb");
    if (!fenc) { perror("encoded.bin open failed"); return 1; }
    FILE *fout = fopen(out_fn, "wb");
    if (!fout) { perror("output open failed"); return 1; }

    char code_buf[128];
    int code_len = 0;
    int bit;

    while ((bit = read_bit(fenc)) != -1) {
        code_buf[code_len++] = bit ? '1' : '0';
        code_buf[code_len] = '\0';

        // 檢查 EOF 標記
        if (code_len >= EOF_MARKER_LEN) {
            int eof_flag = 1;
            for (int i = 0; i < EOF_MARKER_LEN; i++) {
                if (code_buf[code_len - EOF_MARKER_LEN + i] != '1') {
                    eof_flag = 0;
                    break;
                }
            }
            if (eof_flag) break;
        }

        // 嘗試匹配 codebook
        for (int i = 0; i < entry_count; i++) {
            int len = strlen(table[i].code);
            if (code_len >= len &&
                strncmp(code_buf + code_len - len, table[i].code, len) == 0) {
                fputc(table[i].sym, fout);
                code_len -= len;
                if (code_len > 0)
                    memmove(code_buf, code_buf + code_len, code_len);
                code_buf[code_len] = '\0';
                break;
            }
        }
    }

    fclose(fenc);
    fclose(fout);
    return 0;
}
