#!/bin/bash
CC="gcc"
ASM="as"

echo "[*] Компиляция EnvWare..."

$CC -m32 -ffreestanding -fno-stack-protector -c kernel.c      -o kernel.o   || exit 1
$CC -m32 -ffreestanding -fno-stack-protector -c libs/vga.c    -o vga.o      || exit 1
$CC -m32 -ffreestanding -fno-stack-protector -c libs/ata.c    -o ata.o      || exit 1
$CC -m32 -ffreestanding -fno-stack-protector -c libs/fs.c     -o fs.o       || exit 1
$CC -m32 -ffreestanding -fno-stack-protector -c libs/syscall.c -o syscall.o || exit 1
$CC -m32 -ffreestanding -fno-stack-protector -c libs/keyboard.c -o keyboard.o || exit 1
$CC -m32 -ffreestanding -fno-stack-protector -c libs/string.c -o string.o   || exit 1
$CC -m32 -ffreestanding -fno-stack-protector -c libs/terminal.c -o terminal.o || exit 1

$ASM --32 boot.s -o boot.o || exit 1
echo "[*] Линковка..."

ld -m elf_i386 -T linker.ld -nostdlib -o kernel.bin \
   boot.o kernel.o ata.o vga.o fs.o keyboard.o string.o terminal.o syscall.o || exit 1

mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/kernel.bin

cat > isodir/boot/grub/grub.cfg << 'EOF'
menuentry "EnvWare" {
    multiboot /boot/kernel.bin
}
EOF

echo "[*] Создание ISO..."
grub-mkrescue -o envware.iso isodir || exit 1

echo "[+] Готово: envware.iso"
