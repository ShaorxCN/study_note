build:

nasm boot.asm -o boot.bin
dd if=boot.bin of=../../boot.img bs=512 count=1 conv=notrunc

run:
bochs -f xxxxx


loader:
mount boot.img /media/ -t vfat -o loop
cp loader.bin /media/
sync
unmount /media/