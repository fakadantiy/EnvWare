#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_CREATE  1
#define SYS_DELETE  2
#define SYS_LS      3
#define SYS_MKDIR   4
#define SYS_RMDIR   5
#define SYS_CD      6
#define SYS_WRITE   7
#define SYS_READ    8

int syscall(int num, void* arg1, void* arg2);

static inline int sys_create(const char* name)              { return syscall(SYS_CREATE, (void*)name, 0);      }
static inline int sys_delete(const char* name)              { return syscall(SYS_DELETE, (void*)name, 0);      }
static inline void sys_ls(void)                             { syscall(SYS_LS, 0, 0);                           }
static inline int sys_mkdir(const char* name)               { return syscall(SYS_MKDIR, (void*)name, 0);       }
static inline int sys_rmdir(const char* name)               { return syscall(SYS_RMDIR, (void*)name, 0);       }
static inline int sys_cd(const char* name)                  { return syscall(SYS_CD, (void*)name, 0);          }
static inline int sys_write(const char* name, const char* d){ return syscall(SYS_WRITE, (void*)name, (void*)d);}
static inline void sys_read(const char* name)               { syscall(SYS_READ, (void*)name, 0);               }

#endif
