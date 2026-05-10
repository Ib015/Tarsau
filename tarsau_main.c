#include <stdio.h>
#include <string.h>
#include "archive.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanim: tarsau -b [dosyalar] -o [arsiv_adi]\n");
        printf("Kullanim: tarsau -a [arsiv_adi] [opsiyonel_hedef_dizin]\n");
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        char *output_file = "a.sau"; 
        char *input_files[32];       
        int file_count = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) {
                    output_file = argv[i+1];
                    i++; 
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
        return archive_files(file_count, input_files, output_file);
        
    } else if (strcmp(argv[1], "-a") == 0) {
        // -a komutunun argüman kontrolleri
        if (argc < 3 || argc > 4) {
            printf("Kullanım: tarsau -a [arsiv_adi] [opsiyonel_hedef_dizin]\n");
            return 1;
        }
        
        const char *archive_file = argv[2];
        // Eğer 4. argüman varsa dizin adıdır, yoksa NULL gönderiyoruz
        const char *target_dir = (argc == 4) ? argv[3] : NULL;
        
        return extract_archive(archive_file, target_dir);

    } else {
        printf("Bilinmeyen parametre: %s\n", argv[1]);
    }

    return 0;
}