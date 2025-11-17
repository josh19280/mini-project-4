#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SYMBOLS 256
#define CODE_LEN 7
#define EOF_MARKER_LEN 7

typedef struct {
    int sym;
    unsigned long cnt;
    double prob;
    char code[16];
} Entry;

void format_symbol(FILE *f, int sym) {
    if (sym == '\n') fprintf(f, "\"\\n\"");
    else if (sym == '\r') fprintf(f, "\"\\r\"");
    else if (sym == '\"') fprintf(f, "\"\\\"\"");
    else if (sym == '\\') fprintf(f, "\"\\\\\"");
    else if (isprint(sym)) fprintf(f, "\"%c\"", sym);
    else fprintf(f, "\"0x%02X\"", sym);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input.txt codebook.csv encoded.bin\n", argv[0]);
        return 1;
    }

    const char *in_fn = argv[1];
    const char *cb_fn = argv[2];
    const char *enc_fn = argv[3];

    FILE *fin = fopen(in_fn, "rb");
    if (!fin) { perror("input open failed"); return 1; }

    unsigned long count[SYMBOLS] = {0};
    unsigned long total = 0;
    int c;
    while ((c = fgetc(fin)) != EOF) {
        count[(unsigned char)c]++;
        total++;
    }
    fclose(fin);

    if (total == 0) {
        fprintf(stderr, "Empty file.\n");
        return 0;
    }

    Entry *entries = malloc(sizeof(Entry) * SYMBOLS);
    int entry_count = 0;
    for (int i = 0; i < SYMBOLS; i++) {
        if (count[i] > 0) {
            entries[entry_count].sym = i;
            entries[entry_count].cnt = count[i];
            entries[entry_count].prob = (double)count[i] / total;
            entry_count++;
        }
    }

    // 排序
    for (int i = 0; i < entry_count; i++) {
        for (int j = i + 1; j < entry_count; j++) {
            if (entries[i].cnt > entries[j].cnt ||
                (entries[i].cnt == entries[j].cnt && entries[i].sym > entries[j].sym)) {
                Entry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    // 生成 7-bit code（簡單遞增編碼）
    for (int i = 0; i < entry_count; i++) {
        int val = i;
        for (int b = CODE_LEN - 1; b >= 0; b--) {
            entries[i].code[CODE_LEN - 1 - b] = ((val >> b) & 1) ? '1' : '0';
        }
        entries[i].code[CODE_LEN] = '\0';
    }

    // 寫入 codebook
    FILE *fcb = fopen(cb_fn, "w");
    if (!fcb) { perror("codebook open failed"); return 1; }
    for (int i = 0; i < entry_count; i++) {
        format_symbol(fcb, entries[i].sym);
        fprintf(fcb, ",%lu,%.7f,\"%s\"\n",
                entries[i].cnt, entries[i].prob, entries[i].code);
    }
    fclose(fcb);

    // 建立查表
    char *table[SYMBOLS] = {0};
    for (int i = 0; i < entry_count; i++)
        table[entries[i].sym] = entries[i].code;

    fin = fopen(in_fn, "rb");
    FILE *fenc = fopen(enc_fn, "wb");
    if (!fin || !fenc) { perror("file open failed"); return 1; }

    unsigned char byte = 0;
    int bit_count = 0;

    void write_bit(int bit) {
        byte = (byte << 1) | (bit & 1);
        bit_count++;
        if (bit_count == 8) {
            fputc(byte, fenc);
            byte = 0;
            bit_count = 0;
        }
    }

    void write_code(const char *code) {
        for (const char *p = code; *p; p++)
            write_bit(*p == '1');
    }

    while ((c = fgetc(fin)) != EOF) {
        if (table[c]) write_code(table[c]);
    }

    // EOF 標記
    for (int i = 0; i < EOF_MARKER_LEN; i++) write_bit(1);

    // 補滿剩餘位元
    if (bit_count > 0) {
        byte <<= (8 - bit_count);
        fputc(byte, fenc);
    }

    fclose(fin);
    fclose(fenc);
    free(entries);
    return 0;
}
