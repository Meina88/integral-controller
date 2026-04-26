#pragma once

int sd_list_files(char files[][64], int max_files);
int sd_list_files_in_dir(const char *dir_path, char files[][64], int max_files);

int sd_read_file(const char *filename, char *buffer, int max_len);
int sd_read_file_path(const char *path, char *buffer, int max_len);

int sd_read_last_chunk(const char *filename, char *buffer, int max_len);