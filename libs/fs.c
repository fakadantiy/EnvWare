#include "fs.h"
#include "vga.h"
#include "ata.h"
#include "string.h"

static unsigned short fat_table[FAT16_SECTORS_PER_FAT * SECTOR_SIZE / 2];

static FAT16_DirEntry cur_dir[FAT16_ROOT_ENTRIES];
static int            cur_dir_sector;   // LBA первого сектора текущей директории
static int            cur_cluster;      // 0 = root
static int            cur_is_root;

static void fmt83(const char *src, char out8[8], char out3[3]) {
    int i, j = 0;
    for (i = 0; i < 8; i++) out8[i] = ' ';
    for (i = 0; i < 3; i++) out3[i] = ' ';
    i = 0;
    while (src[i] && src[i] != '.' && j < 8) out8[j++] = src[i++];
    if (src[i] == '.') {
        i++;
        j = 0;
        while (src[i] && j < 3) out3[j++] = src[i++];
    }
}

static int entries_match(FAT16_DirEntry *e, const char *name) {
    char n8[8], n3[3];
    int j;
    fmt83(name, n8, n3);
    for (j = 0; j < 8; j++) if (e->name[j] != n8[j]) return 0;
    for (j = 0; j < 3; j++) if (e->ext[j]  != n3[j])  return 0;
    return 1;
}

static int cluster_to_lba(int cluster) {
    return FAT16_DATA_START + (cluster - 2) * FAT16_SECTORS_PER_CLUSTER;
}

static int fat_alloc(void) {
    int i;
    int total = FAT16_SECTORS_PER_FAT * SECTOR_SIZE / 2;
    for (i = 2; i < total; i++) {
        if (fat_table[i] == 0x0000) return i;
    }
    return -1;
}

static void fat_save(void) {
    int i;
    unsigned char *p = (unsigned char *)fat_table;
    for (i = 0; i < FAT16_SECTORS_PER_FAT; i++) {
        ata_write_sector(FAT16_RESERVED_SECTORS + i, p + i * SECTOR_SIZE);
        ata_write_sector(FAT16_RESERVED_SECTORS + FAT16_SECTORS_PER_FAT + i,
                         p + i * SECTOR_SIZE);
    }
}

static void dir_save(void) {
    int i;
    unsigned char *p = (unsigned char *)cur_dir;
    int sectors = cur_is_root ? FAT16_ROOT_SECTORS : FAT16_SECTORS_PER_CLUSTER;
    for (i = 0; i < sectors; i++) {
        ata_write_sector(cur_dir_sector + i, p + i * SECTOR_SIZE);
    }
}

static int dir_find_free(void) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i;
    for (i = 0; i < max; i++) {
        if (cur_dir[i].name[0] == 0x00 || (unsigned char)cur_dir[i].name[0] == 0xE5)
            return i;
    }
    return -1;
}

void fs_init(void) {
    int i;
    unsigned char *p = (unsigned char *)fat_table;
    for (i = 0; i < FAT16_SECTORS_PER_FAT; i++) {
        ata_read_sector(FAT16_RESERVED_SECTORS + i, p + i * SECTOR_SIZE);
    }
    cur_dir_sector = FAT16_ROOT_START;
    cur_cluster    = 0;
    cur_is_root    = 1;
    unsigned char *dp = (unsigned char *)cur_dir;
    for (i = 0; i < FAT16_ROOT_SECTORS; i++) {
        ata_read_sector(FAT16_ROOT_START + i, dp + i * SECTOR_SIZE);
    }
}

void fs_ls(void) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i, j;
    for (i = 0; i < max; i++) {
        if (cur_dir[i].name[0] == 0x00) break;
        if ((unsigned char)cur_dir[i].name[0] == 0xE5) continue;
        char out[13];
        int k = 0;
        for (j = 0; j < 8 && cur_dir[i].name[j] != ' '; j++) out[k++] = cur_dir[i].name[j];
        if (cur_dir[i].ext[0] != ' ') {
            out[k++] = '.';
            for (j = 0; j < 3 && cur_dir[i].ext[j] != ' '; j++) out[k++] = cur_dir[i].ext[j];
        }
        out[k] = 0;
        printstr("\n");
        if (cur_dir[i].attr & ATTR_DIR) {
            printclr(out, 0xB);
            printstr("  [DIR]");
        } else {
            printstr(out);
            printstr("  [FILE]");
        }
    }
}

int fs_cd(const char *name) {
    int i;
    int max = cur_is_root ? FAT16_ROOT_ENTRIES : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);

    if (name[0] == '.' && name[1] == '.' && name[2] == 0) {
        cur_dir_sector = FAT16_ROOT_START;
        cur_cluster    = 0;
        cur_is_root    = 1;
        unsigned char *dp = (unsigned char *)cur_dir;
        for (i = 0; i < FAT16_ROOT_SECTORS; i++)
            ata_read_sector(FAT16_ROOT_START + i, dp + i * SECTOR_SIZE);
        printstr("/ \n");
        return 1;
    }

    for (i = 0; i < max; i++) {
        if (cur_dir[i].name[0] == 0x00) break;
        if ((unsigned char)cur_dir[i].name[0] == 0xE5) continue;
        if (!(cur_dir[i].attr & ATTR_DIR)) continue;
        if (!entries_match(&cur_dir[i], name)) continue;

        int cl = cur_dir[i].startCluster;
        if (cl == 0) {
            // это . или ..
            cur_dir_sector = FAT16_ROOT_START;
            cur_cluster    = 0;
            cur_is_root    = 1;
            unsigned char *dp = (unsigned char *)cur_dir;
            for (i = 0; i < FAT16_ROOT_SECTORS; i++)
                ata_read_sector(FAT16_ROOT_START + i, dp + i * SECTOR_SIZE);
        } else {
            cur_cluster    = cl;
            cur_dir_sector = cluster_to_lba(cl);
            cur_is_root    = 0;
            unsigned char *dp = (unsigned char *)cur_dir;
            ata_read_sector(cur_dir_sector, dp);
        }
	printstr("\n");
        printstr(name);
        return 1;
    }
    printclr("\nerror: no such directory", 0x4);
    return 0;
}

int fs_mkdir(const char *name) {
    int idx = dir_find_free();
    if (idx < 0) { printclr("no space\n", 0x4); return 0; }

    int cl = fat_alloc();
    if (cl < 0) { printclr("no clusters\n", 0x4); return 0; }

    fat_table[cl] = 0xFFFF;
    fat_save();

    unsigned char zero[SECTOR_SIZE];
    int z;
    for (z = 0; z < SECTOR_SIZE; z++) zero[z] = 0;
    int s;
    for (s = 0; s < FAT16_SECTORS_PER_CLUSTER; s++)
        ata_write_sector(cluster_to_lba(cl) + s, zero);

    FAT16_DirEntry *e = &cur_dir[idx];
    int k;
    for (k = 0; k < DIR_ENTRY_SIZE; k++) ((char*)e)[k] = 0;
    fmt83(name, e->name, e->ext);
    e->attr         = ATTR_DIR;
    e->startCluster = (unsigned short)cl;
    e->fileSize     = 0;
    dir_save();
    printstr("\nDirectory created.");
    return 1;
}

int fs_create(const char *name) {
    int idx = dir_find_free();
    if (idx < 0) { printclr("\n error: no space", 0x4); return 0; }

    FAT16_DirEntry *e = &cur_dir[idx];
    int k;
    for (k = 0; k < DIR_ENTRY_SIZE; k++) ((char*)e)[k] = 0;
    fmt83(name, e->name, e->ext);
    e->attr         = ATTR_FILE;
    e->startCluster = 0;
    e->fileSize     = 0;
    dir_save();
    printstr("\n created file");
    return 1;
}

int fs_delete(const char *name) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i;
    for (i = 0; i < max; i++) {
        if (cur_dir[i].name[0] == 0x00) break;
        if ((unsigned char)cur_dir[i].name[0] == 0xE5) continue;
        if (cur_dir[i].attr & ATTR_DIR) continue;
        if (!entries_match(&cur_dir[i], name)) continue;

        unsigned short cl = cur_dir[i].startCluster;
        while (cl >= 2 && cl < 0xFFF8) {
            unsigned short next = fat_table[cl];
            fat_table[cl] = 0;
            cl = next;
        }
        fat_save();
        cur_dir[i].name[0] = (char)0xE5;
        dir_save();
        printstr("\ndeleted");
        return 1;
    }
    printclr("\nerror: not found", 0x4);
    return 0;
}

int fs_rmdir(const char *name) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i;
    for (i = 0; i < max; i++) {
        if (cur_dir[i].name[0] == 0x00) break;
        if ((unsigned char)cur_dir[i].name[0] == 0xE5) continue;
        if (!(cur_dir[i].attr & ATTR_DIR)) continue;
        if (!entries_match(&cur_dir[i], name)) continue;

        unsigned short cl = cur_dir[i].startCluster;
        if (cl >= 2) {
            fat_table[cl] = 0;
            fat_save();
        }
        cur_dir[i].name[0] = (char)0xE5;
        dir_save();
        printstr("rmdir ok\n");
        return 1;
    }
    printclr("not found\n", 0x4);
    return 0;
}

int fs_write(const char *name, const char *data) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i;

    unsigned int dlen = 0;
    while (data[dlen]) dlen++;

    for (i = 0; i < max; i++) {
        if (cur_dir[i].name[0] == 0x00) break;
        if ((unsigned char)cur_dir[i].name[0] == 0xE5) continue;
        if (cur_dir[i].attr & ATTR_DIR) continue;
        if (!entries_match(&cur_dir[i], name)) continue;

        unsigned short cl = cur_dir[i].startCluster;
        while (cl >= 2 && cl < 0xFFF8) {
            unsigned short next = fat_table[cl];
            fat_table[cl] = 0;
            cl = next;
        }

        int ncl = fat_alloc();
        if (ncl < 0) { printclr("no clusters\n", 0x4); return 0; }
        fat_table[ncl] = 0xFFFF;
        fat_save();

        unsigned char buf[SECTOR_SIZE];
        int j;
        for (j = 0; j < SECTOR_SIZE; j++) buf[j] = 0;
        unsigned int copy = dlen < SECTOR_SIZE ? dlen : SECTOR_SIZE - 1;
        for (j = 0; j < (int)copy; j++) buf[j] = (unsigned char)data[j];
        ata_write_sector(cluster_to_lba(ncl), buf);

        cur_dir[i].startCluster = (unsigned short)ncl;
        cur_dir[i].fileSize     = dlen;
        dir_save();
        printstr("written\n");
        return 1;
    }
    printclr("file not found\n", 0x4);
    return 0;
}

void fs_read(const char *name) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i;
    for (i = 0; i < max; i++) {
        if (cur_dir[i].name[0] == 0x00) break;
        if ((unsigned char)cur_dir[i].name[0] == 0xE5) continue;
        if (cur_dir[i].attr & ATTR_DIR) continue;
        if (!entries_match(&cur_dir[i], name)) continue;

        unsigned short cl  = cur_dir[i].startCluster;
        unsigned int   rem = cur_dir[i].fileSize;
        if (cl < 2 || rem == 0) { printstr("(empty)\n"); return; }

        unsigned char buf[SECTOR_SIZE];
        while (cl >= 2 && cl < 0xFFF8 && rem > 0) {
            ata_read_sector(cluster_to_lba(cl), buf);
            unsigned int n = rem < SECTOR_SIZE ? rem : SECTOR_SIZE;
            unsigned int j;
            for (j = 0; j < n; j++) {
                char ch[2]; ch[0] = buf[j]; ch[1] = 0;
                printstr(ch);
            }
            rem -= n;
            cl = fat_table[cl];
        }
        printstr("\n");
        return;
    }
    printclr("not found\n", 0x4);
}
