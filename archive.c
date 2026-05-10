#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // stat() için gerekli
#include "archive.h"

// Dosyanın metin dosyası olup olmadığını kontrol eden yardımcı fonksiyon
// Basitçe dosyanın içinde NULL bayt (\0) arar. Varsa binary kabul eder.
int is_text_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    int ch;
    // Performans için sadece ilk 1024 baytı kontrol edelim
    for(int i = 0; i < 1024 && (ch = fgetc(f)) != EOF; i++) {
        if (ch == 0) { 
            fclose(f);
            return 0; 
        }
    }
    fclose(f);
    return 1;
}

int archive_files(int file_count, char *files[], const char *output_file) {
    if (file_count == 0) {
        printf("Hata: Arşivlenecek dosya belirtilmedi.\n");
        return 1;
    }

    long total_size = 0;
    char header_info[8192] = ""; // Organizasyon (header) stringini tutacak tampon
    struct stat st;

    // 1. ADIM: DOSYALARI KONTROL ET VE HEADER METNİNİ OLUŞTUR
    for (int i = 0; i < file_count; i++) {
        // stat() ile dosyanın metadata bilgilerini çek
        if (stat(files[i], &st) != 0) {
            printf("Hata: %s dosyasi okunamadi veya bulunamadi.\n", files[i]);
            return 1;
        }

        // Metin dosyası mı kontrolü
        if (!is_text_file(files[i])) {
            printf("%s giriş dosyasının formatı uyumsuzdur!\n", files[i]);
            return 1;
        }

        total_size += st.st_size;
        // 200 MB Sınırı (200 * 1024 * 1024 bayt)
        if (total_size > 209715200) {
            printf("Hata: Dosyalarin toplam boyutu 200 MB'i gecemez.\n");
            return 1;
        }

        // Dosya izinlerini oktal formata çek (Örn: 0644)
        int permissions = st.st_mode & 0777;

        // Formata uygun string oluştur ve header_info tamponuna ekle
        char record[256];
        sprintf(record, "|%s,%04o,%ld|", files[i], permissions, st.st_size);
        strcat(header_info, record);
    }

    // 2. ADIM: .SAU DOSYASINA YAZMA İŞLEMİ
    FILE *out = fopen(output_file, "w");
    if (!out) {
        printf("Hata: %s cikti dosyasi olusturulamadi.\n", output_file);
        return 1;
    }

    // İlk 10 bayta organizasyon kısmının uzunluğunu yaz (Sıfır dolgulu, örn: 0000000125)
    int header_length = strlen(header_info);
    fprintf(out, "%010d", header_length);
    
    // Ardından | ile ayrılmış organizasyon bilgilerini yaz
    fprintf(out, "%s", header_info);

    // 3. ADIM: DOSYA İÇERİKLERİNİ ARŞİVE AKTAR
    for (int i = 0; i < file_count; i++) {
        FILE *in = fopen(files[i], "r");
        char buffer[1024];
        size_t bytes;
        // Dosyayı 1'er KB'lık parçalar halinde okuyup arşive aktar
        while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
            fwrite(buffer, 1, bytes, out);
        }
        fclose(in);
    }

    fclose(out);
    printf("Dosyalar birleştirildi. Arşiv oluşturuldu: %s\n", output_file);
    return 0;
}