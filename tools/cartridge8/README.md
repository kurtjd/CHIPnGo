# Cartridge8
|ROM Selection|ROM Edit|
|-------------|--------|
|<img src = "/tools/cartridge8/screenshots/rom_selection.png?raw=true">|<img src = "/tools/cartridge8/screenshots/rom_edit.png?raw=true">|

A tool for adding and configuring CHIP-8 ROMs to an SD card (aka game cartridge)
 for use by the CHIPnGo game console.
 
 ## Requirements
 - Python 3+
 - All requirements in requirements.txt
 
 ## Installation
 ```pip install -r requirements.txt```
 
 ## Run
 ```python cartridge8.py <path-to-SD-card>```
 
 ## Instructions
 - Insert an SD card (dedicated to CHIPnGo, see Warning) before running
 - After starting script, you will be presented with a ROM selection window
 - Select an existing ROM to modify, or an Empty slot to add a new ROM
 - Click Save to write your changes to SD, or Erase to erase the ROM from the SD
 
 ## WARNING
 This tool performs raw writes to your SD card and disregards any kind of file system already on there. 
 Thus, existing files are likely to be erased or corrupted after using this tool. ONLY use an SD card dedicated to 
 CHIPnGo to avoid data loss!
