import struct
import dearpygui.dearpygui as dpg


WIDTH = 547
HEIGHT = 632
SD_PATH = '/dev/sda'
SD_BLOCK_SIZE = 512


class Game:
    def __init__(
        self,
        sector_num,
        title='Empty',
        cpu_freq=1000,
        timer_freq=60,
        display_freq=30,
        quirks=0,
        left_btn_map=0x0080,
        right_btn_map=0x0200,
        up_btn_map=0x0020,
        down_btn_map=0x0100,
        a_btn_map=0x0040,
        b_btn_map=0x0040,
        user_flags=0x0040
    ) -> None:
        self.sector_num = sector_num
        self.title = title
        self.cpu_freq = cpu_freq
        self.timer_freq = timer_freq
        self.display_freq = display_freq
        self.quirks = quirks
        self.left_btn_map = left_btn_map
        self.right_btn_map = right_btn_map
        self.up_btn_map = up_btn_map
        self.down_btn_map = down_btn_map
        self.a_btn_map = a_btn_map
        self.b_btn_map = b_btn_map
        self.user_flags = user_flags


def load_game(sd, n):
    sd.seek(n * (SD_BLOCK_SIZE * 8))
    data = sd.read(SD_BLOCK_SIZE)
    start_byte = data[0]

    if (start_byte == 0xC8):
        title = data[1:12].decode()
        config = struct.unpack('>IBBBHHHHHH', data[12:31])
        user_flags = list(data[31:47])

        return Game(n, title, *config, user_flags)

    return Game(n)


def save_game(sender, data, input):
    rom_num = input[0]
    fields = input[1]
    quirk_inpt = input[2]
    btn_maps = input[3]

    start_byte = struct.pack('>B', 0xC8)
    title = dpg.get_value(fields['title'])
    cpu_freq = struct.pack('>I', dpg.get_value(fields['cpu_freq']))
    timer_freq = struct.pack('>B', dpg.get_value(fields['timer_freq']))
    refresh_freq = struct.pack('>B', dpg.get_value(fields['display_freq']))

    quirks = ''
    for q in quirk_inpt:
        quirks += str(int(dpg.get_value(q)))
    quirks = struct.pack('>B', int(quirks[::-1], 2))

    left_map = ''
    for m in btn_maps['Left']:
        left_map += str(int(dpg.get_value(m)))
    left_map = struct.pack('>H', int(left_map[::-1], 2))

    right_map = ''
    for m in btn_maps['Right']:
        right_map += str(int(dpg.get_value(m)))
    right_map = struct.pack('>H', int(right_map[::-1], 2))

    up_map = ''
    for m in btn_maps['Up']:
        up_map += str(int(dpg.get_value(m)))
    up_map = struct.pack('>H', int(up_map[::-1], 2))

    down_map = ''
    for m in btn_maps['Down']:
        down_map += str(int(dpg.get_value(m)))
    down_map = struct.pack('>H', int(down_map[::-1], 2))

    a_map = ''
    for m in btn_maps['A']:
        a_map += str(int(dpg.get_value(m)))
    a_map = struct.pack('>H', int(a_map[::-1], 2))

    b_map = ''
    for m in btn_maps['B']:
        b_map += str(int(dpg.get_value(m)))
    b_map = struct.pack('>H', int(b_map[::-1], 2))

    metadata = start_byte
    metadata += title.encode()
    metadata += bytes([0] * (11 - len(title)))
    metadata += cpu_freq
    metadata += timer_freq
    metadata += refresh_freq
    metadata += quirks
    metadata += left_map
    metadata += right_map
    metadata += up_map
    metadata += down_map
    metadata += a_map
    metadata += b_map

    sd = open(SD_PATH, 'rb+')
    sd.seek(rom_num * (SD_BLOCK_SIZE * 8))
    sd.write(metadata)
    sd.close()

    rom_path = dpg.get_value(fields['file_path'])
    try:
        print(rom_path)
        rom = open(rom_path, 'rb')
    except IOError:
        print('Unable to open ROM file.')
        return

    rom_data = rom.read()
    rom.close()

    sd = open(SD_PATH, 'rb+')
    sd.seek((rom_num * (SD_BLOCK_SIZE * 8)) + SD_BLOCK_SIZE)
    sd.write(rom_data)
    sd.close()

    print('ROM Saved.')


def open_rom(sender, data, path):
    dpg.set_value(path, data['file_path_name'])


def browse_roms(sender, data, user_data):
    name = 'f{user_data[1]}_open'

    # I HATE GUIs! >:(
    if dpg.does_item_exist(name):
        dpg.delete_item(name)

    dpg.add_file_dialog(tag=name, show=True, modal=True, height=300,
                        callback=open_rom, user_data=user_data[0])
    dpg.add_file_extension(extension='.ch8', parent=name)
    dpg.add_file_extension(extension='.c8', parent=name)


def show_fields(game, n):
    with dpg.group(horizontal=True):
        dpg.add_text('Rom: ')
        file_path = dpg.add_input_text(label='', width=300)
        dpg.add_button(label='Open...', callback=browse_roms,
                       user_data=(file_path, n))

    with dpg.group(horizontal=True):
        dpg.add_text('Title: ')
        title = dpg.add_input_text(label='', default_value=game.title,
                                   width=100, uppercase=True)

    with dpg.group(horizontal=True):
        dpg.add_text('CPU Freq: ')
        cpu_freq = dpg.add_input_int(label='', default_value=game.cpu_freq,
                                     width=100, min_value=0, min_clamped=True)

    with dpg.group(horizontal=True):
        dpg.add_text('Timer Freq: ')
        timer_freq = dpg.add_input_int(label='', default_value=game.timer_freq,
                                       width=100, min_value=0,
                                       min_clamped=True)

    with dpg.group(horizontal=True):
        dpg.add_text('Display Freq: ')
        display_freq = dpg.add_input_int(label='',
                                         default_value=game.display_freq,
                                         width=100, min_value=0,
                                         min_clamped=True)

    return {'file_path': file_path, 'title': title, 'cpu_freq': cpu_freq,
            'timer_freq': timer_freq, 'display_freq': display_freq}


def show_quirks(quirks):
    dpg.add_text('Quirks: ')
    q0 = dpg.add_checkbox(label='Enable 8xy6/8xye Bug',
                          default_value=quirks[0])
    q1 = dpg.add_checkbox(label='Enable Fx55/Fx65 Bug',
                          default_value=quirks[1])
    q2 = dpg.add_checkbox(label='Enable Bnnn Bug', default_value=quirks[2])
    q3 = dpg.add_checkbox(label='Don\'t allow big sprites to be drawn in '
                          'LO-RES mode', default_value=quirks[3])
    q4 = dpg.add_checkbox(label='Don\'t clear display when 00FE/00FF execute',
                          default_value=quirks[4])
    q5 = dpg.add_checkbox(label='Don\'t allow sprite wrapping',
                          default_value=quirks[5])
    q6 = dpg.add_checkbox(label='Enable collision enumeration',
                          default_value=quirks[6])
    q7 = dpg.add_checkbox(label='Enable collision check with bottom of screen',
                          default_value=quirks[7])

    return q0, q1, q2, q3, q4, q5, q6, q7


def show_btn_maps(left_map, right_map, up_map, down_map, a_map, b_map):
    btn_maps = {}

    for r in range(2):
        with dpg.group(horizontal=True, horizontal_spacing=50):
            for i in range(3):
                if (r == 0):
                    if (i == 0):
                        tag = 'Left'
                        mapping = left_map
                    elif (i == 1):
                        tag = 'Right'
                        mapping = right_map
                    else:
                        tag = 'A'
                        mapping = a_map
                else:
                    if (i == 0):
                        tag = 'Up'
                        mapping = up_map
                    elif (i == 1):
                        tag = 'Down'
                        mapping = down_map
                    else:
                        tag = 'B'
                        mapping = b_map

                with dpg.group():
                    dpg.add_text(f'{tag} Button Map: ')
                    with dpg.group(horizontal=True):
                        m1 = dpg.add_checkbox(label='1',
                                              default_value=mapping[0x1])
                        m2 = dpg.add_checkbox(label='2',
                                              default_value=mapping[0x2])
                        m3 = dpg.add_checkbox(label='3',
                                              default_value=mapping[0x3])
                        mC = dpg.add_checkbox(label='C',
                                              default_value=mapping[0xC])

                    with dpg.group(horizontal=True):
                        m4 = dpg.add_checkbox(label='4',
                                              default_value=mapping[0x4])
                        m5 = dpg.add_checkbox(label='5',
                                              default_value=mapping[0x5])
                        m6 = dpg.add_checkbox(label='6',
                                              default_value=mapping[0x6])
                        mD = dpg.add_checkbox(label='D',
                                              default_value=mapping[0xD])

                    with dpg.group(horizontal=True):
                        m7 = dpg.add_checkbox(label='7',
                                              default_value=mapping[0x7])
                        m8 = dpg.add_checkbox(label='8',
                                              default_value=mapping[0x8])
                        m9 = dpg.add_checkbox(label='9',
                                              default_value=mapping[0x9])
                        mE = dpg.add_checkbox(label='E',
                                              default_value=mapping[0xE])

                    with dpg.group(horizontal=True):
                        mA = dpg.add_checkbox(label='A',
                                              default_value=mapping[0xA])
                        m0 = dpg.add_checkbox(label='0',
                                              default_value=mapping[0x0])
                        mB = dpg.add_checkbox(label='B',
                                              default_value=mapping[0xB])
                        mF = dpg.add_checkbox(label='F',
                                              default_value=mapping[0xF])

                    btn_maps[tag] = (m0, m1, m2, m3, m4, m5, m6, m7, m8, m9,
                                     mA, mB, mC, mD, mE, mF)

    return btn_maps


def edit_rom(sender, data, n):
    game = games[n]
    quirks = [bool(int(c)) for c in format(game.quirks, '016b')[::-1]]
    left_map = [bool(int(c)) for c in format(game.left_btn_map, '016b')[::-1]]
    right_map = [bool(int(c))
                 for c in format(game.right_btn_map, '016b')[::-1]]
    up_map = [bool(int(c)) for c in format(game.up_btn_map, '016b')[::-1]]
    down_map = [bool(int(c)) for c in format(game.down_btn_map, '016b')[::-1]]
    a_map = [bool(int(c)) for c in format(game.a_btn_map, '016b')[::-1]]
    b_map = [bool(int(c)) for c in format(game.b_btn_map, '016b')[::-1]]

    with dpg.window(width=WIDTH, height=HEIGHT,
                    on_close=dpg.delete_item):
        fields = show_fields(game, n)
        quirk_inpt = show_quirks(quirks)
        maps = show_btn_maps(left_map, right_map, up_map,
                             down_map, a_map, b_map)

        with dpg.group(horizontal=True, indent=(WIDTH / 2) - 50):
            dpg.add_button(label='Save', callback=save_game,
                           user_data=(n, fields, quirk_inpt, maps))
            dpg.add_button(label='Erase')


def rom_select():
    with dpg.window(tag='Primary Window'):
        for i in range(0, 5):
            with dpg.group(horizontal=True):
                for j in range(0, 5):
                    rom_num = (i * 5) + j
                    dpg.add_button(label=games[rom_num].title,
                                   width=100, height=100,
                                   callback=edit_rom, user_data=rom_num)


sd = open(SD_PATH, 'rb')
games = [load_game(sd, i) for i in range(25)]
sd.close()

dpg.create_context()
dpg.create_viewport(title='CHIPnGo Cartridge Hacker',
                    width=WIDTH, height=HEIGHT)

rom_select()

dpg.setup_dearpygui()
dpg.show_viewport()
dpg.set_primary_window('Primary Window', True)
dpg.start_dearpygui()
dpg.destroy_context()
