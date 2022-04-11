# CHIPnGo
|Current Status|
|--------------|
|<img src = "/images/current_status.jpg?raw=true">|

Recently I've taken a serious interest in embedded development, so I decided for my first project I would attempt to port my [CHIP-8 emulator](https://github.com/kurtjd/jaxe) to a STM32 microcontroller in order to eventually build a CHIP-8 hand-held gaming console. Since this is being done as a learning experience, I also decided I would do this bare-metal with no libraries and write all drivers from scratch. While this is proving to be slow and extremely frustrating at times, it has also been very insightful and I've already learned a ton in such a short time.

## Progress
Currently, I have basic drivers written for interfacing with peripherals such as the system clock, GPIO, and UART. I have also begun making a basic prototype on a breadboard. Right now input via buttons works, however the display is handled by communicating with a Python program via UART. This Python program then uses Pygame to draw the received graphical data in real-time. This is allowing me to test other functionality while I wait for my LCD display to arrive and for me to write a driver for it.

## Planned Features
- Fully support all CHIP-8 and S-CHIP ROMs
- LCD graphical display
- Four directional buttons and two action buttons
- Piezo buzzer for simple tone generation
- SD card reader for quick loading of any CHIP-8 ROM
- Battery power

## ToDo
- Wire LCD and write LCD driver
- Wire SD card reader and write SD driver
- Wire piezo buzzer and generate appropriate frequency via code
- Add startup menu for user to select ROM via SD card, as well as configure options such as button mapping
- Add ROM config file parser, in order to save configurations for certain ROMs to SD
- Add functionality for S-CHIP user flags to be saved to flash memory
- Determine battery needs and get prototype running off battery
- Add simple switch to turn console on/off
- Perform extensive testing on breadboard prototype
- Learn PCB design and create custom PCB
- Create simple case for PCB

## Limitations
Although my original emulator also has full support for XO-CHIP roms, I decided to focus on just CHIP-8 and S-CHIP for now for several reasons:
- XO-CHIP ROMs expect up to 64kb of memory, as opposed to 4kb expected by original CHIP-8 ROMs.
- XO-CHIP supports up to three colors, which would mean needing to move beyond a simple monochrome display.
- XO-CHIP supports variable frequencies/tones whereas CHIP-8 ROMs can only produce a single frequency, which makes handling sound simple.
