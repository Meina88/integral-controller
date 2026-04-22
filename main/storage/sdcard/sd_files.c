#include "sd_files.h"
#include "sdcard.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

// =========================
// LISTAR ARCHIVOS
// =========================
int sd_list_files(char files[][64], int max_files)
{
    if (!sdcard_is_ready())
        return -1;

    DIR *dir = opendir("/sdcard");
    if (!dir)
        return -1;

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max_files)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        strncpy(files[count], entry->d_name, 63);
        files[count][63] = '\0';
        count++;
    }

    closedir(dir);
    return count;
}

// =========================
// LEER ARCHIVO
// =========================
int sd_read_file(const char *filename, char *buffer, int max_len)
{
    char path[128];
    snprintf(path, sizeof(path), "/sdcard/%s", filename);

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    size_t read_bytes = fread(buffer, 1, max_len - 1, f);
    buffer[read_bytes] = '\0';

    fclose(f);
    return read_bytes;
}