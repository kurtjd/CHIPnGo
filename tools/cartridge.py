# Octopeg
rom = open('/home/kurtjd/Documents/jaxe/roms/chip8archive/octopeg.ch8', 'rb')
rom_data = rom.read()

start_byte = 0xC8
title = 'Octopeg'
cpu_freq = [0, 0, 0, 0]
timer_freq = 60
refresh_freq = 30
quirks = 0
left_map = [0x00, 0x80]
right_map = [0x02, 0x00]
up_map = [0x00, 0x20]
down_map = [0x01, 0x00]
a_map = [0x00, 0x40]
b_map = [0x00, 0x40]

metadata = bytes([start_byte])
metadata += title.encode()
metadata += bytes([0] * (11 - len(title)))
metadata += bytes(cpu_freq)
metadata += bytes([timer_freq])
metadata += bytes([refresh_freq])
metadata += bytes([quirks])
metadata += bytes(left_map)
metadata += bytes(right_map)
metadata += bytes(up_map)
metadata += bytes(down_map)
metadata += bytes(a_map)
metadata += bytes(b_map)

sd = open('/dev/sda', 'rb+')
sd.write(metadata)

sd.seek(0)
data = sd.read(512)

bs = [hex(b) for b in data]
for b in bs:
    print(f'{b} ', end='')
print()

sd.seek(512)
sd.write(rom_data)
