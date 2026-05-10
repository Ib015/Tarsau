#include <stdio.h>
#include <string.h>
#include "archive.h"

int main(int argc, char *argv[]) {
    // En az 2 argüman olmalı: "./tarsau -b" veya "./tarsau -a"
    if (argc < 2) {
        printf("Kullanım: tarsau -b [dosyalar] -o [arsiv_adi]\n");
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        char *output_file = "a.sau"; // Varsayılan çıktı dosyası
        char *input_files[32];       // Maksimum 32 dosya sınırı
        int file_count = 0;

        // Argümanları tara
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                // Eğer -o varsa, bir sonraki kelime arşiv adıdır
                if (i + 1 < argc) {
                    output_file = argv[i+1];
                    i++; // Arşiv adını dosya olarak okumamak için döngüyü 1 atlat
                }
            } else {
                if (file_count < 32) {
                    input_files[file_count++] = argv[i];
                } else {
                    printf("Hata: En fazla 32 dosya girebilirsiniz.\n");
                    return 1;
                }
            }
        }
        
        // Parametreleri topladık, şimdi işi fonksiyonumuza devrediyoruz
        return archive_files(file_count, input_files, output_file);
        
    } else if (strcmp(argv[1], "-a") == 0) {
        printf("Çıkarma işlemi (-a) henüz kodlanmadı.\n");
    } else {
        printf("Bilinmeyen parametre: %s\n", argv[1]);
    }

    return 0;
}