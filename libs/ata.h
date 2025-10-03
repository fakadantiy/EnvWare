#ifndef ATA_H
#define ATA_H

#define ATA_PRIMARY_IO   0x1F0
#define ATA_DATA         0x1F0
#define ATA_ERROR        0x1F1
#define ATA_SECCOUNT0    0x1F2
#define ATA_LBA0         0x1F3
#define ATA_LBA1         0x1F4
#define ATA_LBA2         0x1F5
#define ATA_HDDEVSEL     0x1F6
#define ATA_COMMAND      0x1F7
#define ATA_STATUS       0x1F7

#define ATA_CMD_READ     0x20
#define ATA_CMD_WRITE    0x30

void outw(short port, short data);
void ata_read_sector(unsigned int lba, void* buf);
void ata_write_sector(unsigned int lba, const void* buf);

#endif
