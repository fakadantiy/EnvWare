#include "fs.h"
#include "vga.h"
#include "ata.h"

static unsigned char fat[9 * SECTOR_SIZE];
static FAT12_DirEntry current[FAT12_ROOT_ENTRIES];
static int current_sector = 19;
static int current_cluster = 0;  

static int find_free_dir_entry(FAT12_DirEntry* dir, int maxEntries) {
    int i;
    for (i = 0; i < maxEntries; i++) {
        if (dir[i].name[0] == 0x00 || dir[i].name[0] == (char)0xE5)
            return i;
    }
    return -1;
}

static void format_filename(const char* name, char* out) {
    int i = 0, j = 0;
    for (j = 0; j < 11; j++) out[j] = ' ';

    j = 0;
    while (name[i] && j < 11) {
        if (name[i] == '.') { j = 8; i++; continue; }
        out[j++] = name[i++];
    }
}

void fs_init() {
    int i;
    for (i = 0; i < 9; i++) {
        ata_read_sector(1 + i, fat + i * SECTOR_SIZE);
    }
    ata_read_sector(current_sector, current);
}

void fs_ls() {
    int i;
    for (i = 0; i < FAT12_ROOT_ENTRIES; i++) {
        if (current[i].name[0] == 0x00) break;
        if (current[i].name[0] == (char)0xE5) continue;

        char out[12];
        int j;
        for (j = 0; j < 11; j++) out[j] = current[i].name[j];
        out[11] = 0;

	printstr("\n");
        if (current[i].attr & 0x10) {
            printstr(out);
            printstr("   [DIR]\n");
        } else {
            printstr(out);
            printstr("   [FILE]\n");
        }
    }
}

int cluster_to_sector(int cluster) {
    return 33 + (cluster - 2); // DATA_START = 33
}

int fs_cd(const char* name) {
    char formatted[11];
    int i, j;
    format_filename(name, formatted);

    for (i = 0; i < FAT12_ROOT_ENTRIES; i++) {
        if (current[i].name[0] == 0x00) break;
        if (current[i].name[0] == (char)0xE5) continue;

        int eq = 1;
        for (j = 0; j < 11; j++) {
            if (current[i].name[j] != formatted[j]) { eq = 0; break; }
        }

        if (eq && (current[i].attr & 0x10)) {
            if (current[i].startCluster == 0) {
                // это root
                current_sector = 19;
                current_cluster = 0;
                ata_read_sector(current_sector, current);
            } else {
                current_cluster = current[i].startCluster;
                current_sector = cluster_to_sector(current_cluster);
                ata_read_sector(current_sector, current);
            }

            printstr("Changed directory to: ");
            printstr(name);
            printstr("\n");
            return 1;
        }
    }

    printclr("Directory not found\n", 0x4);
    return 0;
}


int fs_create(const char* name) {
    int idx = find_free_dir_entry(current, FAT12_ROOT_ENTRIES);
    if (idx < 0) {
        printclr("\nNo free memory\n", 0x4);
        return 0;
    }

    FAT12_DirEntry* e = &current[idx];
    int k;
    for (k = 0; k < sizeof(FAT12_DirEntry); k++) ((char*)e)[k] = 0;

    format_filename(name, e->name);
    e->attr = 0x20;
    e->startCluster = 0;
    e->fileSize = 0;

    ata_write_sector(current_sector, current);
    printstr("File created: ");
    printstr(name);
    printstr("\n");
    return 1;
}

int fs_mkdir(const char* name) {
    int idx = find_free_dir_entry(current, FAT12_ROOT_ENTRIES);
    if (idx < 0) {
        printclr("\nNo free memory\n", 0x4);
        return 0;
    }

    FAT12_DirEntry* e = &current[idx];
    int k;
    for (k = 0; k < sizeof(FAT12_DirEntry); k++) ((char*)e)[k] = 0;

    format_filename(name, e->name);
    e->attr = 0x10;
    e->startCluster = 0;
    e->fileSize = 0;

    ata_write_sector(current_sector, current);
    printstr("Directory created: ");
    printstr(name);
    printstr("\n");
    return 1;
}

int fs_delete(const char* name) {
    char formatted[11];
    int i, j;
    format_filename(name, formatted);

    for (i = 0; i < FAT12_ROOT_ENTRIES; i++) {
        if (current[i].name[0] == 0x00) break;
        if (current[i].name[0] == (char)0xE5) continue;

        int eq = 1;
        for (j = 0; j < 11; j++) {
            if (current[i].name[j] != formatted[j]) { eq = 0; break; }
        }

        if (eq) {
            if (current[i].attr & 0x10) {
                printclr("Use rmdir for directories\n", 0x4);
                return 0;
            }

            current[i].name[0] = (char)0xE5;
            ata_write_sector(current_sector, current);
            printstr("Deleted file: ");
            printstr(name);
            printstr("\n");
            return 1;
        }
    }

    printclr("File not found\n", 0x4);
    return 0;
}

int fs_rmdir(const char* name) {
    char formatted[11];
    int i, j;
    format_filename(name, formatted);

    for (i = 0; i < FAT12_ROOT_ENTRIES; i++) {
        if (current[i].name[0] == 0x00) break;
        if (current[i].name[0] == (char)0xE5) continue;

        int eq = 1;
        for (j = 0; j < 11; j++) {
            if (current[i].name[j] != formatted[j]) { eq = 0; break; }
        }

        if (eq) {
            if (!(current[i].attr & 0x10)) {
                printclr("This is not a directory\n", 0x4);
                return 0;
            }

            current[i].name[0] = (char)0xE5;
            ata_write_sector(current_sector, current);
            printstr("Deleted directory: ");
            printstr(name);
            printstr("\n");
            return 1;
        }
    }

    printclr("Directory not found\n", 0x4);
    return 0;
}
