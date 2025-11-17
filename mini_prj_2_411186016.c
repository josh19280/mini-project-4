#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define PI 3.14159265359

#pragma pack(1) // 確保 WAV header 沒有 padding
typedef struct {
    char riff[4];        // "RIFF"
    unsigned int size;   // 檔案大小 - 8
    char wave[4];        // "WAVE"
    char fmt[4];         // "fmt "
    unsigned int fmt_len;// Subchunk1 size (16 for PCM)
    unsigned short audio_format; // 1=PCM
    unsigned short num_channels; // 聲道數
    unsigned int sample_rate;    // 取樣率
    unsigned int byte_rate;      // = sample_rate * num_channels * bits/8
    unsigned short block_align;  // = num_channels * bits/8
    unsigned short bits_per_sample; // 取樣位元數
    char data[4];        // "data"
    unsigned int data_size; // 波形資料大小
} WAVHEADER;
#pragma pack()

// 產生不同波形
double generate_wave(const char* type, double t, double f, double A) {
    if(strcmp(type, "sine") == 0) {
        return A * sin(2 * PI * f * t);
    } else if(strcmp(type, "square") == 0) {
        return (sin(2*PI*f*t) >= 0) ? A : -A;
    } else if(strcmp(type, "triangle") == 0) {
        return (2*A/PI) * asin(sin(2*PI*f*t));
    } else if(strcmp(type, "sawtooth") == 0) {
        return (2*A/PI) * (fmod(t*f, 1.0)*PI - PI/2);
    } else {
        return 0.0;
    }
}

int main(int argc, char* argv[]) {
    if(argc < 8) {
        fprintf(stderr,"Usage: %s fs m c wavetype f A T\n", argv[0]);
        return 1;
    }

    // 讀取命令列參數
    double fs = atof(argv[1]);          // 取樣率
    int m = atoi(argv[2]);              // 量化位元數
    int c = atoi(argv[3]);              // 聲道數
    char* wavetype = argv[4];           // 波形類型
    double f = atof(argv[5]);           // 頻率
    double A_input = atof(argv[6]);     // 振幅 0~1
    double L = atof(argv[7]);           // 音訊長度(秒)

    // 計算實際振幅
    double max_amplitude = pow(2, m-1) - 1;
    double A = A_input * max_amplitude;

    size_t N = (size_t)(fs * L);

    short *x = (short*) malloc(sizeof(short)*N*c);
    if(!x) { fprintf(stderr,"Memory allocation error\n"); return 1; }

    double signal_power = 0.0, noise_power = 0.0;

    // 產生波形
    for(size_t n=0; n<N; n++) {
        double t = n / fs;
        double tmp = generate_wave(wavetype, t, f, A);
        double q = floor(tmp + 0.5);
        short sample = (short)q;

        signal_power += tmp*tmp;
        noise_power += (tmp - q)*(tmp - q);

        for(int ch=0; ch<c; ch++) {
            x[n*c + ch] = sample; // 複製到每個聲道
        }
    }

    // 建立 WAV header
    WAVHEADER header;
    memcpy(header.riff,"RIFF",4);
    memcpy(header.wave,"WAVE",4);
    memcpy(header.fmt,"fmt ",4);
    memcpy(header.data,"data",4);

    header.fmt_len = 16;
    header.audio_format = 1;
    header.num_channels = c;
    header.sample_rate = (unsigned int)fs;
    header.bits_per_sample = m;
    header.byte_rate = header.sample_rate * header.num_channels * header.bits_per_sample / 8;
    header.block_align = header.num_channels * header.bits_per_sample / 8;
    header.data_size = (unsigned int)(N * c * (m/8));
    header.size = 36 + header.data_size;

    // 輸出 WAV 到 stdout
    fwrite(&header, sizeof(WAVHEADER),1,stdout);
    fwrite(x, sizeof(short), N*c, stdout);

    // SQNR 到 stderr
    double sqnr_db = 10 * log10(signal_power / noise_power);
    fprintf(stderr,"%.15f\n", sqnr_db);

    free(x);
    return 0;
}
