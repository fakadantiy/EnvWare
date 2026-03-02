#ifndef FS_H
#define FS_H

#define SECTOR_SIZE        512
#define FAT16_ROOT_ENTRIES 512
#define DIR_ENTRY_SIZE     32

#define FAT16_RESERVED_SECTORS    1
#define FAT16_FAT_COUNT           2
#define FAT16_SECTORS_PER_FAT    32
#define FAT16_ROOT_START          (FAT16_RESERVED_SECTORS + FAT16_FAT_COUNT * FAT16_SECTORS_PER_FAT)
#define FAT16_ROOT_SECTORS        (FAT16_ROOT_ENTRIES * DIR_ENTRY_SIZE / SECTOR_SIZE)
#define FAT16_DATA_START          (FAT16_ROOT_START + FAT16_ROOT_SECTORS)
#define FAT16_SECTORS_PER_CLUSTER  1

#define ATTR_READONLY 0x01
#define ATTR_FILE     0x20
#define ATTR_DIR      0x10
#define ATTR_LFN      0x0F

typedef struct {
    char           name[8];
    char           ext[3];
    unsigned char  attr;
    unsigned char  ntres;
    unsigned char  crt_time_tenth;
    unsigned short crt_time;
    unsigned short crt_date;
    unsigned short lst_acc_date;
    unsigned short fst_clus_hi;
    unsigned short wrt_time;
    unsigned short wrt_date;
    unsigned short startCluster;
    unsigned int   fileSize;
} __attribute__((packed)) FAT16_DirEntry;

typedef struct {
    unsigned char  order;
    unsigned short name1[5];
    unsigned char  attr;
    unsigned char  type;
    unsigned char  checksum;
    unsigned short name2[6];
    unsigned short cluster;
    unsigned short name3[2];
} __attribute__((packed)) FAT16_LFNEntry;

void fs_init(void);
void fs_ls(void);
int  fs_cd(const char *name);
int  fs_mkdir(const char *name);
int  fs_create(const char *name);
int  fs_delete(const char *name);
int  fs_rmdir(const char *name);
int  fs_write(const char *name, const char *data);
void fs_read(const char *name);

#endif
