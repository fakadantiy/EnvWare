#ifndef FS_H
#define FS_H

#define SECTOR_SIZE 512
#define FAT12_ENTRY_SIZE 32
#define FAT12_ROOT_ENTRIES 224

// FAT12 запись директории (8.3 формат)
typedef struct {
    char name[11];         // Имя (8) + расширение (3)
    unsigned char attr;    // Атрибуты (0x10 = DIR, 0x20 = FILE)
    char reserved[10];     // Зарезервировано
    unsigned short time;   // Время
    unsigned short date;   // Дата
    unsigned short startCluster;
    unsigned int fileSize;
} __attribute__((packed)) FAT12_DirEntry;

void fs_init();
int fs_create(const char* path);
int fs_delete(const char* path);
int fs_rmdir(const char* path);
int fs_mkdir(const char* path);
int fs_cd(const char* name);
void fs_ls();

#endif
