#include "fs.h"
#include "vga.h"
#include "ata.h"
#include "string.h"

static unsigned short fat_table[FAT16_SECTORS_PER_FAT * SECTOR_SIZE / 2];
static FAT16_DirEntry cur_dir[FAT16_ROOT_ENTRIES];
static int cur_dir_sector;
static int cur_cluster;
static int cur_is_root;

static int fs_strlen(const char *s) {
    int i = 0; while (s[i]) i++; return i;
}

static int fs_streq_nocase(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a; char cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == *b;
}

static char to_upper(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

static unsigned char lfn_checksum(const char n11[11]) {
    unsigned char s = 0;
    int i;
    for (i = 0; i < 11; i++)
        s = (unsigned char)(((s & 1) << 7) | (s >> 1)) + (unsigned char)n11[i];
    return s;
}

static char lfn_get_char(unsigned short v) {
    if (v == 0xFFFF || v == 0x0000) return 0;
    return (char)(v & 0xFF);
}

static unsigned short lfn_put_char(char c) {
    if (c == 0) return 0x0000;
    return (unsigned short)(unsigned char)c;
}

static void lfn_collect(int lfn_start, int count, char *out) {
    int pos = 0;
    int seg, i;
    for (seg = count - 1; seg >= 0; seg--) {
        FAT16_LFNEntry *e = (FAT16_LFNEntry *)&cur_dir[lfn_start + seg];
        char c;
        for (i = 0; i < 5; i++) { c = lfn_get_char(e->name1[i]); if (!c) goto done; out[pos++] = c; }
        for (i = 0; i < 6; i++) { c = lfn_get_char(e->name2[i]); if (!c) goto done; out[pos++] = c; }
        for (i = 0; i < 2; i++) { c = lfn_get_char(e->name3[i]); if (!c) goto done; out[pos++] = c; }
    }
done:
    out[pos] = 0;
}

static void gen_83(const char *name, char out11[11], int uniq) {
    int i, j = 0;
    for (i = 0; i < 11; i++) out11[i] = ' ';
    for (i = 0; name[i] && name[i] != '.' && j < 6; i++)
        out11[j++] = to_upper(name[i]);
    out11[j++] = '~';
    out11[j]   = '0' + (char)(uniq % 10);
    const char *dot = 0;
    for (i = 0; name[i]; i++) if (name[i] == '.') dot = &name[i + 1];
    if (dot) {
        for (i = 0; i < 3 && dot[i]; i++)
            out11[8 + i] = to_upper(dot[i]);
    }
}

static int needs_lfn(const char *name) {
    int base = 0, ext = 0, in_ext = 0, i;
    for (i = 0; name[i]; i++) {
        if (name[i] == '.') { in_ext = 1; continue; }
        if (!in_ext) base++; else ext++;
        if (name[i] >= 'a' && name[i] <= 'z') return 1;
        if (name[i] == ' ') return 1;
    }
    return base > 8 || ext > 3;
}

static int lfn_count_needed(int name_len) {
    return (name_len + 12) / 13;
}

static void write_lfn_entries(int idx, const char *name, const char n11[11],
                               unsigned char attr) {
    int name_len = fs_strlen(name);
    int n = lfn_count_needed(name_len);
    unsigned char csum = lfn_checksum(n11);
    int seg, i;

    for (seg = n; seg >= 1; seg--) {
        FAT16_LFNEntry *lfn = (FAT16_LFNEntry *)&cur_dir[idx + (n - seg)];
        int base = (seg - 1) * 13;
        lfn->order    = (seg == n) ? (0x40 | (unsigned char)n) : (unsigned char)seg;
        lfn->attr     = ATTR_LFN;
        lfn->type     = 0;
        lfn->checksum = csum;
        lfn->cluster  = 0;
        for (i = 0; i < 5; i++) {
            int ci = base + i;
            lfn->name1[i] = (ci < name_len) ? lfn_put_char(name[ci])
                          : (ci == name_len) ? 0x0000 : 0xFFFF;
        }
        for (i = 0; i < 6; i++) {
            int ci = base + 5 + i;
            lfn->name2[i] = (ci < name_len) ? lfn_put_char(name[ci])
                          : (ci == name_len) ? 0x0000 : 0xFFFF;
        }
        for (i = 0; i < 2; i++) {
            int ci = base + 11 + i;
            lfn->name3[i] = (ci < name_len) ? lfn_put_char(name[ci])
                          : (ci == name_len) ? 0x0000 : 0xFFFF;
        }
    }

    FAT16_DirEntry *e = &cur_dir[idx + n];
    int k;
    for (k = 0; k < DIR_ENTRY_SIZE; k++) ((char *)e)[k] = 0;
    for (i = 0; i < 8; i++) e->name[i] = n11[i];
    for (i = 0; i < 3; i++) e->ext[i]  = n11[8 + i];
    e->attr = attr;
}

static int find_entry(const char *name, int want_dir) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES
                          : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i;
    char lfn_buf[256];
    int lfn_start = -1, lfn_cnt = 0;

    for (i = 0; i < max; i++) {
        unsigned char first = (unsigned char)cur_dir[i].name[0];
        if (first == 0x00) break;
        if (first == 0xE5) { lfn_start = -1; lfn_cnt = 0; continue; }

        if (cur_dir[i].attr == ATTR_LFN) {
            FAT16_LFNEntry *lfn = (FAT16_LFNEntry *)&cur_dir[i];
            if (lfn->order & 0x40) {
                lfn_start = i;
                lfn_cnt   = 1;
            } else {
                lfn_cnt++;
            }
            continue;
        }

        int is_dir = (cur_dir[i].attr & ATTR_DIR) != 0;
        if (want_dir == 1 && !is_dir) { lfn_start = -1; lfn_cnt = 0; continue; }
        if (want_dir == 0 &&  is_dir) { lfn_start = -1; lfn_cnt = 0; continue; }

        if (lfn_start >= 0 && lfn_cnt > 0) {
            lfn_collect(lfn_start, lfn_cnt, lfn_buf);
            if (fs_streq_nocase(lfn_buf, name)) return i;
            lfn_start = -1; lfn_cnt = 0;
            continue;
        }

        char short_name[13];
        int k = 0, j;
        for (j = 0; j < 8 && cur_dir[i].name[j] != ' '; j++)
            short_name[k++] = cur_dir[i].name[j];
        if (cur_dir[i].ext[0] != ' ') {
            short_name[k++] = '.';
            for (j = 0; j < 3 && cur_dir[i].ext[j] != ' '; j++)
                short_name[k++] = cur_dir[i].ext[j];
        }
        short_name[k] = 0;
        if (fs_streq_nocase(short_name, name)) return i;
        lfn_start = -1; lfn_cnt = 0;
    }
    return -1;
}

static int find_free_n(int n) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES
                          : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i, run = 0, run_start = 0;
    for (i = 0; i < max; i++) {
        unsigned char f = (unsigned char)cur_dir[i].name[0];
        if (f == 0x00 || f == 0xE5) {
            if (run == 0) run_start = i;
            if (++run >= n) return run_start;
        } else {
            run = 0;
        }
    }
    return -1;
}

static void delete_entry(int idx83) {
    cur_dir[idx83].name[0] = (char)0xE5;
    int i = idx83 - 1;
    while (i >= 0 && cur_dir[i].attr == ATTR_LFN) {
        cur_dir[i].name[0] = (char)0xE5;
        i--;
    }
}

static int cluster_to_lba(int cl) {
    return FAT16_DATA_START + (cl - 2) * FAT16_SECTORS_PER_CLUSTER;
}

static int fat_alloc(void) {
    int total = FAT16_SECTORS_PER_FAT * SECTOR_SIZE / 2;
    int i;
    for (i = 2; i < total; i++)
        if (fat_table[i] == 0x0000) return i;
    return -1;
}

static void fat_save(void) {
    unsigned char *p = (unsigned char *)fat_table;
    int i;
    for (i = 0; i < FAT16_SECTORS_PER_FAT; i++) {
        ata_write_sector(FAT16_RESERVED_SECTORS + i,                         p + i * SECTOR_SIZE);
        ata_write_sector(FAT16_RESERVED_SECTORS + FAT16_SECTORS_PER_FAT + i, p + i * SECTOR_SIZE);
    }
}

static void dir_save(void) {
    unsigned char *p = (unsigned char *)cur_dir;
    int sectors = cur_is_root ? FAT16_ROOT_SECTORS : FAT16_SECTORS_PER_CLUSTER;
    int i;
    for (i = 0; i < sectors; i++)
        ata_write_sector(cur_dir_sector + i, p + i * SECTOR_SIZE);
}

void fs_init(void) {
    unsigned char *p = (unsigned char *)fat_table;
    int i;
    for (i = 0; i < FAT16_SECTORS_PER_FAT; i++)
        ata_read_sector(FAT16_RESERVED_SECTORS + i, p + i * SECTOR_SIZE);
    cur_dir_sector = FAT16_ROOT_START;
    cur_cluster    = 0;
    cur_is_root    = 1;
    unsigned char *dp = (unsigned char *)cur_dir;
    for (i = 0; i < FAT16_ROOT_SECTORS; i++)
        ata_read_sector(FAT16_ROOT_START + i, dp + i * SECTOR_SIZE);
}

void fs_ls(void) {
    int max = cur_is_root ? FAT16_ROOT_ENTRIES
                          : (FAT16_SECTORS_PER_CLUSTER * SECTOR_SIZE / DIR_ENTRY_SIZE);
    int i;
    char lfn_buf[256];
    int lfn_start = -1, lfn_cnt = 0;

    for (i = 0; i < max; i++) {
        unsigned char first = (unsigned char)cur_dir[i].name[0];
        if (first == 0x00) break;
        if (first == 0xE5) { lfn_start = -1; lfn_cnt = 0; continue; }

        if (cur_dir[i].attr == ATTR_LFN) {
            FAT16_LFNEntry *lfn = (FAT16_LFNEntry *)&cur_dir[i];
            if (lfn->order & 0x40) { lfn_start = i; lfn_cnt = 1; }
            else lfn_cnt++;
            continue;
        }

        if (cur_dir[i].name[0] == '.' ) { lfn_start = -1; lfn_cnt = 0; continue; }

        char display[256];
        if (lfn_start >= 0 && lfn_cnt > 0) {
            lfn_collect(lfn_start, lfn_cnt, display);
        } else {
            int k = 0, j;
            for (j = 0; j < 8 && cur_dir[i].name[j] != ' '; j++)
                display[k++] = cur_dir[i].name[j];
            if (cur_dir[i].ext[0] != ' ') {
                display[k++] = '.';
                for (j = 0; j < 3 && cur_dir[i].ext[j] != ' '; j++)
                    display[k++] = cur_dir[i].ext[j];
            }
            display[k] = 0;
        }
        lfn_start = -1; lfn_cnt = 0;

        printstr("\n");
        if (cur_dir[i].attr & ATTR_DIR) {
            printclr(display, 0xB);
            printstr("  [DIR]");
        } else {
            printstr(display);
            printstr("  [FILE]");
        }
    }
}

int fs_cd(const char *name) {
    int i;
    if (name[0] == '.' && name[1] == '.' && name[2] == 0) {
        cur_dir_sector = FAT16_ROOT_START;
        cur_cluster    = 0;
        cur_is_root    = 1;
        unsigned char *dp = (unsigned char *)cur_dir;
        for (i = 0; i < FAT16_ROOT_SECTORS; i++)
            ata_read_sector(FAT16_ROOT_START + i, dp + i * SECTOR_SIZE);
        printstr("\n/");
        return 1;
    }

    int idx = find_entry(name, 1);
    if (idx < 0) { printclr("\nno such directory", 0x4); return 0; }

    int cl = cur_dir[idx].startCluster;
    if (cl < 2) {
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
        for (i = 0; i < FAT16_SECTORS_PER_CLUSTER; i++)
            ata_read_sector(cur_dir_sector + i, dp + i * SECTOR_SIZE);
    }
    printstr("\n");
    printstr(name);
    return 1;
}

int fs_mkdir(const char *name) {
    int name_len = fs_strlen(name);
    int use_lfn  = needs_lfn(name);
    int lfn_n    = use_lfn ? lfn_count_needed(name_len) : 0;
    int slots    = lfn_n + 1;

    int idx = find_free_n(slots);
    if (idx < 0) { printclr("\nno space", 0x4); return 0; }

    int cl = fat_alloc();
    if (cl < 0) { printclr("\nno clusters", 0x4); return 0; }
    fat_table[cl] = 0xFFFF;
    fat_save();

    unsigned char sector[SECTOR_SIZE];
    int z, q;
    for (z = 0; z < SECTOR_SIZE; z++) sector[z] = 0;

    FAT16_DirEntry *dot    = (FAT16_DirEntry *)(sector);
    FAT16_DirEntry *dotdot = (FAT16_DirEntry *)(sector + DIR_ENTRY_SIZE);

    for (q = 0; q < 8; q++) dot->name[q] = ' ';
    for (q = 0; q < 3; q++) dot->ext[q]  = ' ';
    dot->name[0]      = '.';
    dot->attr         = ATTR_DIR;
    dot->startCluster = (unsigned short)cl;

    for (q = 0; q < 8; q++) dotdot->name[q] = ' ';
    for (q = 0; q < 3; q++) dotdot->ext[q]  = ' ';
    dotdot->name[0]      = '.';
    dotdot->name[1]      = '.';
    dotdot->attr         = ATTR_DIR;
    dotdot->startCluster = (unsigned short)cur_cluster;

    ata_write_sector(cluster_to_lba(cl), sector);
    int s;
    for (s = 1; s < FAT16_SECTORS_PER_CLUSTER; s++) {
        unsigned char z2[SECTOR_SIZE];
        for (q = 0; q < SECTOR_SIZE; q++) z2[q] = 0;
        ata_write_sector(cluster_to_lba(cl) + s, z2);
    }

    char n11[11];
    gen_83(name, n11, 1);

    if (use_lfn) {
        write_lfn_entries(idx, name, n11, ATTR_DIR);
        cur_dir[idx + lfn_n].startCluster = (unsigned short)cl;
        cur_dir[idx + lfn_n].fileSize     = 0;
    } else {
        FAT16_DirEntry *e = &cur_dir[idx];
        for (q = 0; q < DIR_ENTRY_SIZE; q++) ((char *)e)[q] = 0;
        for (q = 0; q < 8; q++) e->name[q] = n11[q];
        for (q = 0; q < 3; q++) e->ext[q]  = n11[8 + q];
        e->attr         = ATTR_DIR;
        e->startCluster = (unsigned short)cl;
        e->fileSize     = 0;
    }

    dir_save();
    printstr("\nmkdir ok");
    return 1;
}

int fs_create(const char *name) {
    int name_len = fs_strlen(name);
    int use_lfn  = needs_lfn(name);
    int lfn_n    = use_lfn ? lfn_count_needed(name_len) : 0;
    int slots    = lfn_n + 1;

    int idx = find_free_n(slots);
    if (idx < 0) { printclr("\nno space", 0x4); return 0; }

    char n11[11];
    gen_83(name, n11, 1);

    int q;
    if (use_lfn) {
        write_lfn_entries(idx, name, n11, ATTR_FILE);
        cur_dir[idx + lfn_n].startCluster = 0;
        cur_dir[idx + lfn_n].fileSize     = 0;
    } else {
        FAT16_DirEntry *e = &cur_dir[idx];
        for (q = 0; q < DIR_ENTRY_SIZE; q++) ((char *)e)[q] = 0;
        for (q = 0; q < 8; q++) e->name[q] = n11[q];
        for (q = 0; q < 3; q++) e->ext[q]  = n11[8 + q];
        e->attr         = ATTR_FILE;
        e->startCluster = 0;
        e->fileSize     = 0;
    }

    dir_save();
    printstr("\ncreated");
    return 1;
}

int fs_delete(const char *name) {
    int idx = find_entry(name, 0);
    if (idx < 0) { printclr("\nnot found", 0x4); return 0; }

    unsigned short cl = cur_dir[idx].startCluster;
    while (cl >= 2 && cl < 0xFFF8) {
        unsigned short next = fat_table[cl];
        fat_table[cl] = 0;
        cl = next;
    }
    fat_save();
    delete_entry(idx);
    dir_save();
    printstr("\ndeleted");
    return 1;
}

int fs_rmdir(const char *name) {
    int idx = find_entry(name, 1);
    if (idx < 0) { printclr("\nnot found", 0x4); return 0; }

    unsigned short cl = cur_dir[idx].startCluster;
    if (cl >= 2) { fat_table[cl] = 0; fat_save(); }
    delete_entry(idx);
    dir_save();
    printstr("\nrmdir ok");
    return 1;
}

int fs_write(const char *name, const char *data) {
    int idx = find_entry(name, 0);
    if (idx < 0) { printclr("\nfile not found", 0x4); return 0; }

    unsigned int dlen = 0;
    while (data[dlen]) dlen++;

    unsigned short cl = cur_dir[idx].startCluster;
    while (cl >= 2 && cl < 0xFFF8) {
        unsigned short next = fat_table[cl];
        fat_table[cl] = 0;
        cl = next;
    }

    int ncl = fat_alloc();
    if (ncl < 0) { printclr("\nno clusters", 0x4); return 0; }
    fat_table[ncl] = 0xFFFF;
    fat_save();

    unsigned char buf[SECTOR_SIZE];
    int j;
    for (j = 0; j < SECTOR_SIZE; j++) buf[j] = 0;
    unsigned int copy = dlen < SECTOR_SIZE ? dlen : SECTOR_SIZE - 1;
    for (j = 0; j < (int)copy; j++) buf[j] = (unsigned char)data[j];
    ata_write_sector(cluster_to_lba(ncl), buf);

    cur_dir[idx].startCluster = (unsigned short)ncl;
    cur_dir[idx].fileSize     = dlen;
    dir_save();
    printstr("\nwritten");
    return 1;
}

void fs_read(const char *name) {
    int idx = find_entry(name, 0);
    if (idx < 0) { printclr("\nnot found", 0x4); return; }

    unsigned short cl  = cur_dir[idx].startCluster;
    unsigned int   rem = cur_dir[idx].fileSize;
    if (cl < 2 || rem == 0) { printstr("\n(empty)"); return; }

    printstr("\n");
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
}
