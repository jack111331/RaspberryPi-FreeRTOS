# FreeRTOS ported to Raspberry Pi 3 Model B

Fork from Forty-Tw0's RPi 2 FreeRTOS build
https://github.com/Forty-Tw0/RaspberryPi-FreeRTOS

---

1. Clone this repository:

```git clone https://github.com/rooperl/RaspberryPi-FreeRTOS```

2. Run make in the root directory to build a kernel image file:

```
cd RaspberryPi-FreeRTOS
make clean
make
```

If you don't need to change any source files, you can use the pre-built
kernel7.img file in the root directory.

3. Format an SD card with the FAT32 filesystem.

4. Move bootcode.bin, config.txt, start.elf and kernel7.img to the SD card.

5. Refer to led_pins.txt on connecting the LEDs to Raspberry's GPIO pins.

Raspberry Pi 3 GPIO pin layout:
https://openclipart.org/image/2400px/svg_to_png/280972/gpiopinsv3withpi.png

6. Insert the SD card before powering the Raspberry.
You should see your LEDs continuosly blinking at a frequency
between 1 ... 5 seconds depending on the LEDs you have connected.

Optional: You can connect Raspberry to a monitor with HDMI to see some output.
