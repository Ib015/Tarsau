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

int extract_archive(const char *archive_file, const char *target_dir) {
    FILE *in = fopen(archive_file, "r");
    if (!in) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        return 1;
    }

    // 1. İlk 10 baytı oku (Organizasyon uzunluğu)
    char header_len_str[11];
    if (fread(header_len_str, 1, 10, in) != 10) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(in);
        return 1;
    }
    header_len_str[10] = '\0';
    int header_length = atoi(header_len_str); 

    // 2. Organizasyon (Header) metnini oku
    char *header = malloc(header_length + 1);
    if (fread(header, 1, header_length, in) != header_length) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        free(header); 
        fclose(in); 
        return 1;
    }
    header[header_length] = '\0';

    // 3. Kullanıcı bir dizin belirttiyse onu oluştur (0777 standart klasör iznidir)
    if (target_dir) {
        mkdir(target_dir, 0777); 
    }

    // 4. Header'ı parçala ve dosyaları çıkart
    char *token = strtok(header, "|");
    
    while (token != NULL) {
        char filename[256];
        int perms;
        long size;
        
        // Kimlik kartını oku: İsim, İzin (Oktal), Boyut
        if (sscanf(token, "%[^,],%o,%ld", filename, &perms, &size) == 3) {
            
            char filepath[1024];
            if (target_dir) {
                sprintf(filepath, "%s/%s", target_dir, filename);
            } else {
                strcpy(filepath, filename);
            }

            FILE *out = fopen(filepath, "w");
            if (out) {
                char buffer[1024];
                long remaining = size;
                while (remaining > 0) {
                    size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
                    size_t bytes_read = fread(buffer, 1, to_read, in);
                    if (bytes_read == 0) break; 
                    
                    fwrite(buffer, 1, bytes_read, out);
                    remaining -= bytes_read;
                }
                fclose(out);
                
                // Orijinal dosya izinlerini (0664 vs.) geri yükle
                chmod(filepath, perms);
            }
        }
        token = strtok(NULL, "|");
    }

    free(header);
    fclose(in);
    
    if (target_dir) {
        printf("%s dizininde dosyalar açıldı.\n", target_dir);
    } else {
        printf("Geçerli dizinde dosyalar açıldı.\n");
    }
    return 0;
}