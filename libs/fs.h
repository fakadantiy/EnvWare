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

#define ATTR_FILE  0x20
#define ATTR_DIR   0x10

typedef struct {
    char           name[8];
    char           ext[3];
    unsigned char  attr;
    char           reserved[10];
    unsigned short time;
    unsigned short date;
    unsigned short startCluster;
    unsigned int   fileSize;
} __attribute__((packed)) FAT16_DirEntry;

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
