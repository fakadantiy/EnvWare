#!/bin/bash

DISK="disk.img"
SIZE_MB=16

echo "[*] Создаём $DISK ($SIZE_MB MB)..."
dd if=/dev/zero of="$DISK" bs=1M count=$SIZE_MB status=none

# Пишем FAT16 Boot Sector (BPB)
python3 - "$DISK" <<'PYEOF'
import struct, sys

disk = sys.argv[1]

with open(disk, 'r+b') as f:
    bpb = bytearray(512)

    bpb[0]  = 0xEB
    bpb[1]  = 0x58
    bpb[2]  = 0x90
    bpb[3:11] = b'ENVWARE '

    struct.pack_into('<H', bpb, 11, 512)
    bpb[13] = 1
    # Reserved sectors (boot sector)
    struct.pack_into('<H', bpb, 14, 1)
    # Number of FATs
    bpb[16] = 2
    # Root entry count
    struct.pack_into('<H', bpb, 17, 512)
    # Total sectors 16 (0 = use 32-bit field)
    struct.pack_into('<H', bpb, 19, 0)
    # Media type
    bpb[21] = 0xF8
    # FAT size in sectors
    struct.pack_into('<H', bpb, 22, 32)
    # Sectors per track
    struct.pack_into('<H', bpb, 24, 63)
    # Number of heads
    struct.pack_into('<H', bpb, 26, 16)
    # Hidden sectors
    struct.pack_into('<I', bpb, 28, 0)
    # Total sectors 32
    total = 16 * 1024 * 1024 // 512
    struct.pack_into('<I', bpb, 32, total)
    # Drive number
    bpb[36] = 0x80
    # Boot sig
    bpb[38] = 0x29
    # Volume ID
    struct.pack_into('<I', bpb, 39, 0xDEADBEEF)
    # Volume label
    bpb[43:54] = b'ENVWARE    '
    # FS type
    bpb[54:62] = b'FAT16   '
    # Signature
    bpb[510] = 0x55
    bpb[511] = 0xAA

    f.seek(0)
    f.write(bpb)

    # FAT1 и FAT2 — записать медиабайт
    fat = bytearray(32 * 512)
    fat[0] = 0xF8
    fat[1] = 0xFF
    fat[2] = 0xFF
    fat[3] = 0xFF
    f.seek(512)       # LBA 1
    f.write(fat)
    f.seek(512 + 32*512)  # LBA 33
    f.write(fat)

print("  BPB и FAT записаны")
PYEOF

echo "[+] Диск создан: $DISK"
