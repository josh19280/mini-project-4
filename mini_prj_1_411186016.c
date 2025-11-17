#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ASCII 128
#define MAX_UTF8 1024
#define MAX_RECORDS (MAX_ASCII + MAX_UTF8)

typedef struct {
    unsigned char ch[4]; // 儲存 UTF-8 bytes
    int length;           // UTF-8 長度
    int count;
    double prob;
    int is_ascii;         // 是否 ASCII
} CharRecord;

// 判斷 UTF-8 字元長度
int utf8Length(unsigned char byte) {
    if ((byte & 0x80) == 0x00) return 1;
    if ((byte & 0xE0) == 0xC0) return 2;
    if ((byte & 0xF0) == 0xE0) return 3;
    if ((byte & 0xF8) == 0xF0) return 4;
    return 1; // fallback
}

// 統計字元
void countCharacters(FILE *f, CharRecord records[], int *totalCount) {
    int c;
    while ((c = fgetc(f)) != EOF) {
        unsigned char byte = (unsigned char)c;
        int len = utf8Length(byte);

        if (byte < MAX_ASCII) { // ASCII
            if (!records[byte].is_ascii) {
                records[byte].ch[0] = byte;
                records[byte].length = 1;
                records[byte].is_ascii = 1;
                records[byte].count = 1;
            } else {
                records[byte].count++;
            }
        } else { // UTF-8 多位元
            unsigned char temp[4] = {byte};
            for (int i = 1; i < len; i++) {
                temp[i] = (unsigned char)fgetc(f);
            }

            // 尋找是否已存在
            int found = 0;
            for (int i = MAX_ASCII; i < MAX_RECORDS; i++) {
                if (records[i].count > 0 && records[i].length == len &&
                    memcmp(records[i].ch, temp, len) == 0) {
                    records[i].count++;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                for (int i = MAX_ASCII; i < MAX_RECORDS; i++) {
                    if (records[i].count == 0) {
                        memcpy(records[i].ch, temp, len);
                        records[i].length = len;
                        records[i].count = 1;
                        records[i].is_ascii = 0;
                        break;
                    }
                }
            }
        }
        (*totalCount)++;
    }
}

// 計算機率
void calculateProbability(CharRecord records[], int totalCount) {
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (records[i].count > 0)
            records[i].prob = (double)records[i].count / totalCount;
    }
}

// 排序函數：機率大到小
int compareRecords(const void *a, const void *b) {
    const CharRecord *ca = (const CharRecord*)a;
    const CharRecord *cb = (const CharRecord*)b;
    if (cb->prob > ca->prob) return 1;
    if (cb->prob < ca->prob) return -1;
    return memcmp(ca->ch, cb->ch, ca->length);
}

// CSV 欄位輸出，特殊字元自動加引號
void printCSVField(CharRecord *rec) {
    if (rec->is_ascii) {
        unsigned char ch = rec->ch[0];
        // 如果是特殊符號或空格，使用雙引號括起
        if (ch == ' ' || ch == ',' || ch == '"' || ch == ';' ||
            ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '-' || ch == '?') {
            if (ch == '"') {
                // 雙引號要用兩個引號表示
                printf("\"\"\"\"");
            } else {
                printf("\"%c\"", ch);
            }
        }
        // 換行符號特別處理
        else if (ch == '\n') printf("\"\\n\"");
        else if (ch == '\r') printf("\"\\r\"");
        else printf("%c", ch);
    } else {
        // UTF-8 多位元字元
        printf("\"");
        for (int i = 0; i < rec->length; i++)
            printf("%c", rec->ch[i]);
        printf("\"");
    }
}

// 輸出 CSV
void outputCSV(CharRecord records[]) {
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (records[i].count == 0) continue;
        printCSVField(&records[i]);
        printf(",%d,%.15f\n", records[i].count, records[i].prob);
    }
}

int main() {
    CharRecord records[MAX_RECORDS] = {0};
    int totalCount = 0;

    FILE *fp = fopen("input.txt", "rb"); // 二進位讀檔
    if (!fp) { printf("File open error!\n"); return 1; }

    countCharacters(fp, records, &totalCount);
    fclose(fp);

    calculateProbability(records, totalCount);
    qsort(records, MAX_RECORDS, sizeof(CharRecord), compareRecords);

    outputCSV(records);
    return 0;
}
