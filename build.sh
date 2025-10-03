Cc="gcc"
Asm="as"

echo "Thanks to locomiadev for build.sh"

tesl() {
  echo "EnvWare installer: " $1
}
if [ ! $Cc = "gcc" ]; then
    tesl "Warning: the CC is not GCC: can be unstable!"
fi
tesl "Compilating [0%]"
$Cc -m32 -ffreestanding -c kernel.c -o kernel.o
tesl "Compilating [20%]"
$Cc -m32 -ffreestanding -c libs/vga.c -o vga.o
$Cc -m32 -ffreestanding -c -fno-stack-protector libs/ata.c -o ata.o
tesl "Compilating [40%]"
$Cc -m32 -ffreestanding -c -fno-stack-protector libs/fs.c -o fs.o
tesl "Compilating [50%]"
$Cc -m32 -ffreestanding -c libs/syscall.c -o syscall.o
tesl "Compilating [55%]"
$Cc -m32 -ffreestanding -c libs/keyboard.c -o keyboard.o
tesl "Compilating [60%]"
$Cc -m32 -ffreestanding -c libs/string.c -o string.o
tesl "Compilating [80%]"
$Cc -m32 -ffreestanding -c libs/terminal.c -o terminal.o
tesl "Compilating [100%]"

$Asm --32 boot.s -o boot.o
tesl "Compilated bootloader"

tesl "Linking"
ld -m elf_i386 -T linker.ld -nostdlib -o kernel.bin boot.o kernel.o ata.o vga.o fs.o keyboard.o string.o terminal.o syscall.o
if [ ! -d isodir/boot/grub ]; then
  mkdir -p isodir/boot/grub
fi
tesl "Copying the kernel"
cp kernel.bin isodir/boot/kernel.bin

tesl "Making menuentry for EnvWare"
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "EnvWare" {
    multiboot /boot/kernel.bin
}
EOF
tesl "Making ISO"
grub-mkrescue -o envware.iso isodir
tesl "You sucessfully built EnvWare"
	
