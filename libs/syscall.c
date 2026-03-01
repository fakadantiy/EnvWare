#include "syscall.h"
#include "fs.h"
#include "vga.h"

int syscall(int num, void* arg1, void* arg2) {
    switch (num) {
        case SYS_CREATE: return fs_create((const char*)arg1);
        case SYS_DELETE: return fs_delete((const char*)arg1);
        case SYS_LS:     fs_ls(); return 0;
        case SYS_MKDIR:  return fs_mkdir((const char*)arg1);
        case SYS_RMDIR:  return fs_rmdir((const char*)arg1);
        case SYS_CD:     return fs_cd((const char*)arg1);
        case SYS_WRITE:  return fs_write((const char*)arg1, (const char*)arg2);
        case SYS_READ:   fs_read((const char*)arg1); return 0;
        default:
            printclr("\nsyscall: unknown", 0x4);
            return -1;
    }
}
