#pragma once

int sd_list_files(char files[][64], int max_files);
int sd_read_file(const char *filename, char *buffer, int max_len);