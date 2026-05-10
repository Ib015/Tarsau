#ifndef ARCHIVE_H
#define ARCHIVE_H

// -b komutu için arşivleme
int archive_files(int file_count, char *files[], const char *output_file);

// -a komutu için arşivden çıkarma
int extract_archive(const char *archive_file, const char *target_dir);

#endif