#include "syscall.h"
#include "fs.h"

#define SYS_CREATE 1
#define SYS_DELETE 2
#define SYS_LS 3

int syscall(int num, void* arg1, void* arg2) {
        switch (num) {
                case SYS_CREATE: return fs_create((const char*)arg1);
                case SYS_DELETE: return fs_delete((const char*)arg1);
                case SYS_LS: fs_ls(); return 0;
                default: return -1;
        }
}	
