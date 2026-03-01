#!/bin/bash

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

ISO="envware.iso"
DISK="disk.img"

check_deps() {
    local missing=0
    for cmd in qemu-system-i386 gcc grub-mkrescue xorriso; do
        if ! command -v "$cmd" &>/dev/null; then
            echo "[!] Не найден: $cmd"
            missing=1
        fi
    done
    if [ $missing -ne 0 ]; then
        echo ""
        echo "Установи зависимости (Ubuntu/Debian):"
        echo "  sudo apt-get install qemu-system-x86 gcc gcc-multilib grub-pc-bin xorriso"
        exit 1
    fi
}

check_deps

if [ "$1" = "-f" ] || [ ! -f "$DISK" ]; then
    echo "[*] Создаём диск..."
    bash create_disk.sh
fi

if [ "$1" = "rebuild" ] || [ "$1" = "fresh" ] || [ ! -f "$ISO" ]; then
    echo "[*] Сборка ядра..."
    bash build.sh
fi

echo "[*] Запуск QEMU..."
echo "    Диск: $DISK  (сохраняется между запусками)"
echo "    ISO:  $ISO"
echo ""

qemu-system-i386 \
    -drive if=ide,bus=0,unit=0,file="$DISK",format=raw \
    -drive if=ide,bus=1,unit=0,file="$ISO",format=raw,media=cdrom \
    -boot d \
    -m 32M \
    -vga std \
    -no-reboot \
    "$@"
