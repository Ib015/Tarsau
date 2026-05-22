#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "archive.h"

int archive_files(int file_count, char *files[], const char *output_file) {
    long total_size = 0;

    // ASCII ve 200 MB Sınırı
    for (int i = 0; i < file_count; i++) {
        FILE *f = fopen(files[i], "rb");
        if (!f) {
            printf("Hata: %s dosyası açılamadı!\n", files[i]);
            return 1;
        }

        int c;
        long f_size = 0;
        while ((c = fgetc(f)) != EOF) {
            f_size++;
            // Karakter başına 1 bayt ve ASCII (0-127) kontrolü
            if (c < 0 || c > 127) {
                printf("%s giriş dosyasının formatı uyumsuzdur!\n", files[i]);
                fclose(f);
                return 1;
            }
        }
        fclose(f);
        total_size += f_size;
    }

    // Toplam boyut 200 MB kontrolü
    if (total_size > 209715200) {
        printf("Hata: Giriş dosyalarının toplam boyutu 200 MB'ı geçemez!\n");
        return 1;
    }

    // 2. ARŞİVLEME İŞLEMİ
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Hata: Çıktı dosyası oluşturulamadı!\n");
        return 1;
    }

    // Organizasyon (Header) metnini oluşturma
    char header[4096] = "";
    for (int i = 0; i < file_count; i++) {
        struct stat st;
        if (stat(files[i], &st) == 0) {
            char entry[256];
            // Dosya adı, izinler (oktal) ve boyut yazımı
            sprintf(entry, "%s,%o,%ld|", files[i], st.st_mode & 0777, st.st_size);
            strcat(header, entry);
        }
    }

    int header_length = strlen(header);
    // İlk 10 bayta organizasyon bölümünün boyutunu yazıyoruz
    fprintf(out, "%010d", header_length);
    fwrite(header, 1, header_length, out);

    // Dosya içeriklerini ardı ardına ekleme
    for (int i = 0; i < file_count; i++) {
        FILE *f = fopen(files[i], "rb");
        if (f) {
            char buffer[1024];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
                fwrite(buffer, 1, bytes_read, out);
            }
            fclose(f);
        }
    }

    fclose(out);
    printf("Dosyalar birleştirildi.\n");
    return 0;
}

int extract_archive(const char *archive_file, const char *target_dir) {
    FILE *in = fopen(archive_file, "rb");
    if (!in) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        return 1;
    }

    // İlk 10 baytı oku (Organizasyon uzunluğu)
    char header_len_str[11];
    if (fread(header_len_str, 1, 10, in) != 10) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(in);
        return 1;
    }
    header_len_str[10] = '\0';
    int header_length = atoi(header_len_str); 

    // Organizasyon (Header) metnini oku
    char *header = malloc(header_length + 1);
    if (fread(header, 1, header_length, in) != (size_t)header_length) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        free(header); 
        fclose(in); 
        return 1;
    }
    header[header_length] = '\0';

    // Dizin oluşturma
    if (target_dir) {
        #ifdef _WIN32
            mkdir(target_dir);
        #else
            mkdir(target_dir, 0777); 
        #endif
    }

    // Header'ı parçala ve dosyaları çıkart
    char *token = strtok(header, "|");
    while (token != NULL) {
        char filename[256];
        int perms;
        long size;
        
        if (sscanf(token, "%[^,],%o,%ld", filename, &perms, &size) == 3) {
            char filepath[1024];
            if (target_dir) {
                sprintf(filepath, "%s/%s", target_dir, filename);
            } else {
                strcpy(filepath, filename);
            }

            FILE *out = fopen(filepath, "wb");
            if (out) {
                char buffer[1024];
                long remaining = size;
                while (remaining > 0) {
                    size_t to_read = ((size_t)remaining < sizeof(buffer)) ? (size_t)remaining : sizeof(buffer);
                    size_t bytes_read = fread(buffer, 1, to_read, in);
                    if (bytes_read == 0) break; 
                    
                    fwrite(buffer, 1, bytes_read, out);
                    remaining -= bytes_read;
                }
                fclose(out);
                
                #ifndef _WIN32
                    chmod(filepath, perms);
                #endif
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