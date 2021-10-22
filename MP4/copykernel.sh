# Replace "/mnt/floppy" with the whatever directory is appropriate.
sudo mount -o loop dev_kernel_grub.img /mnt/floppy
sudo cp kernel.bin /mnt/floppy
sleep 1s
echo "copykernel is in place"
sudo umount /mnt/floppy
