/*
 * Mock de sd_files — retorna lista vacía y falla en lecturas.
 * El simulador no tiene acceso a la SD card.
 */
#include "storage/sdcard/sd_files.h"
#include <string.h>

int sd_list_files(char files[][64], int max_files)
    { (void)files; (void)max_files; return 0; }

int sd_list_files_in_dir(const char *dir, char files[][64], int max_files)
    { (void)dir; (void)files; (void)max_files; return 0; }

int sd_read_file(const char *filename, char *buffer, int max_len)
    { (void)filename; (void)buffer; (void)max_len; return -1; }

int sd_read_file_path(const char *path, char *buffer, int max_len)
    { (void)path; (void)buffer; (void)max_len; return -1; }

int sd_read_last_chunk(const char *filename, char *buffer, int max_len)
    { (void)filename; (void)buffer; (void)max_len; return -1; }
