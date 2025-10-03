#include "ata.h"

void outw(short port, short data) {
    __asm__ __volatile__("outw %%ax, %%dx" : : "a"(data), "d"(port));
}

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outsw(unsigned short port, const unsigned short* addr, unsigned int count) {
    __asm__ __volatile__(
        "rep outsw"
        : "+S"(addr), "+c"(count)
        : "d"(port)
    );
}

static inline void insw(unsigned short port, unsigned short* addr, unsigned int count) {
    __asm__ __volatile__(
        "rep insw"
        : "+D"(addr), "+c"(count)
        : "d"(port)
        : "memory"
    );
}

static void ata_wait() {
    unsigned char status;
    do {
        status = inb(ATA_STATUS);
    } while (status & 0x80); // ждем пока занято (BSY)
    do {
        status = inb(ATA_STATUS);
    } while (!(status & 0x08)); // ждем пока DRQ
}

void ata_read_sector(unsigned int lba, void* buf) {
    unsigned char drive = 0xE0; // master
    outb(ATA_HDDEVSEL, drive | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT0, 1); // 1 сектор
    outb(ATA_LBA0, (unsigned char)(lba & 0xFF));
    outb(ATA_LBA1, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (unsigned char)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ);

    ata_wait();
    insw(ATA_DATA, (unsigned short*)buf, 256); // 256 слов = 512 байт
}

void ata_write_sector(unsigned int lba, const void* buf) {
    unsigned char drive = 0xE0; // master
    outb(ATA_HDDEVSEL, drive | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT0, 1); // 1 сектор
    outb(ATA_LBA0, (unsigned char)(lba & 0xFF));
    outb(ATA_LBA1, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (unsigned char)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    ata_wait();
    outsw(ATA_DATA, (const unsigned short*)buf, 256);
}
