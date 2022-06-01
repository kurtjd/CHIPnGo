# dpg.add_text("Hello, world")
# dpg.add_input_text(label="string", default_value="Quick brown fox")

import struct
import dearpygui.dearpygui as dpg


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
    sd.seek(n * (512 * 8))
    data = sd.read(512)
    start_byte = data[0]

    if (start_byte == 0xC8):
        title = data[1:12].decode()
        config = struct.unpack('>IBBBHHHHHH', data[12:31])
        user_flags = list(data[31:47])

        return Game(n, title, *config, user_flags)

    return Game(n)


sd = open('/dev/sda', 'rb+')
games = [load_game(sd, i) for i in range(25)]
sd.close()

dpg.create_context()
dpg.create_viewport(title='CHIPnGo Cartridge Hacker', width=547, height=532)

with dpg.window(tag='Primary Window'):
    for i in range(0, 5):
        with dpg.group(horizontal=True):
            for j in range(0, 5):
                dpg.add_button(label=games[(i * 5) + j].title,
                               width=100, height=100)

dpg.setup_dearpygui()
dpg.show_viewport()
dpg.set_primary_window('Primary Window', True)
dpg.start_dearpygui()
dpg.destroy_context()
