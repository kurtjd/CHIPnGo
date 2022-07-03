# CHIPnGo
|Current Status|
|--------------|
|<img src = "/images/current_status.jpg?raw=true">|

Recently I've taken a serious interest in embedded development, so I decided for my first project I would attempt to port my [CHIP-8 emulator](https://github.com/kurtjd/jaxe) to a STM32 microcontroller in order to eventually build a CHIP-8 hand-held gaming console. Since this is being done as a learning experience, I also decided I would do this bare-metal with no libraries and write all drivers from scratch. While this is proving to be slow and extremely frustrating at times, it has also been very insightful and I've already learned a ton in such a short time.

## Progress
The PCB is fully functional! Will probably do some minor firmware tweaks in the future but other than that the console is complete!

[See it in action!](https://www.youtube.com/watch?v=ixTLp5M0RTQ)

## Features
- Fully supports all CHIP-8 and S-CHIP ROMs
- LCD graphical display
- Four directional buttons and two action buttons
- Piezo buzzer for simple tone generation
- SD card reader for quick loading of any CHIP-8 ROM
- Battery power
- [Desktop application to manage ROMs on an SD card (aka game cartridge)](tools/cartridge8/)

## ToDo
- Improve startup menu for user to select ROM via SD card (make a bit more pretty)
- Improve startup splash screen (want something cool and animated)
- Create simple case for PCB

## Limitations
Although my original emulator also has full support for XO-CHIP roms, I decided to focus on just CHIP-8 and S-CHIP for now for several reasons:
- XO-CHIP ROMs expect up to 64kb of memory, as opposed to 4kb expected by original CHIP-8 ROMs.
- XO-CHIP supports up to three colors, which would mean needing to move beyond a simple monochrome display.
- XO-CHIP supports variable frequencies/tones whereas CHIP-8 ROMs can only produce a single frequency, which makes handling sound simple.

CHIP-8 supports 16 keys for input, however for design purposes, the console only has 6 buttons.
However, most games only use a few buttons so this isn't too much of an issue.
