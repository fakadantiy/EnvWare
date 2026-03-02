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

// 400нс задержка — читаем status 4 раза (каждое чтение ~100нс)
static void ata_delay400(void) {
    inb(ATA_ALT_STATUS);
    inb(ATA_ALT_STATUS);
    inb(ATA_ALT_STATUS);
    inb(ATA_ALT_STATUS);
}

// ждать BSY=0, возвращает 0 если таймаут
static int ata_wait_bsy(void) {
    unsigned int timeout = 0x100000;
    while (timeout--) {
        if (!(inb(ATA_STATUS) & ATA_SR_BSY))
            return 1;
    }
    return 0; // таймаут
}

// ждать DRQ=1 и BSY=0
static int ata_wait_drq(void) {
    unsigned int timeout = 0x100000;
    while (timeout--) {
        unsigned char s = inb(ATA_STATUS);
        if (s & ATA_SR_ERR) return 0;
        if (!(s & ATA_SR_BSY) && (s & ATA_SR_DRQ)) return 1;
    }
    return 0;
}

// проверить что диск вообще есть (возвращает 1 если есть)
static int ata_identify(void) {
    outb(ATA_HDDEVSEL, 0xA0); // master
    ata_delay400();
    // если все регистры 0xFF — шины нет
    if (inb(ATA_STATUS) == 0xFF) return 0;
    outb(ATA_SECCOUNT0, 0);
    outb(ATA_LBA0, 0);
    outb(ATA_LBA1, 0);
    outb(ATA_LBA2, 0);
    outb(ATA_COMMAND, 0xEC); // IDENTIFY
    unsigned char s = inb(ATA_STATUS);
    if (s == 0) return 0;
    if (!ata_wait_bsy()) return 0;
    // читаем 256 слов чтобы очистить буфер
    unsigned short tmp;
    int i;
    for (i = 0; i < 256; i++) {
        __asm__ __volatile__("inw %1, %0" : "=a"(tmp) : "Nd"((unsigned short)ATA_DATA));
    }
    return 1;
}

static int disk_ok = -1; // -1 = не проверяли, 0 = нет диска, 1 = есть

static int check_disk(void) {
    if (disk_ok == -1) disk_ok = ata_identify();
    return disk_ok;
}

void ata_read_sector(unsigned int lba, void* buf) {
    if (!check_disk()) return;

    if (!ata_wait_bsy()) return;

    outb(ATA_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    ata_delay400();
    outb(ATA_SECCOUNT0, 1);
    outb(ATA_LBA0, (unsigned char)(lba & 0xFF));
    outb(ATA_LBA1, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (unsigned char)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ);
    ata_delay400();

    if (!ata_wait_drq()) return;
    insw(ATA_DATA, (unsigned short*)buf, 256);
}

void ata_write_sector(unsigned int lba, const void* buf) {
    if (!check_disk()) return;

    if (!ata_wait_bsy()) return;

    outb(ATA_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    ata_delay400();
    outb(ATA_SECCOUNT0, 1);
    outb(ATA_LBA0, (unsigned char)(lba & 0xFF));
    outb(ATA_LBA1, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (unsigned char)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE);
    ata_delay400();

    if (!ata_wait_drq()) return;
    outsw(ATA_DATA, (const unsigned short*)buf, 256);

    // flush cache
    if (!ata_wait_bsy()) return;
    outb(ATA_COMMAND, 0xE7);
    ata_wait_bsy();
}
