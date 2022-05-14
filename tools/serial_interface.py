#!/usr/bin/python3

import sys
import pygame
import serial


def get_pixels():
    return jaxe.read(1024)


def draw_pixels(pixels):
    x = 0
    y = 0

    for i, pixel in enumerate(pixels):
        bits = [(pixel >> bit) & 1 for bit in range(8 - 1, -1, -1)]

        for j, bit in enumerate(bits):
            screen.set_at((x + j, y), white if bit else black)

        x += 8
        if (i + 1) % 16 == 0:
            y += 1
            x = 0


def handle_input():
    for event in pygame.event.get():
        keyevt = False
        keyup = 0

        if event.type == pygame.QUIT:
            sys.exit()
        elif event.type == pygame.KEYDOWN:
            keyup = 0
            keyevt = True
        elif event.type == pygame.KEYUP:
            keyup = 1
            keyevt = True

        if keyevt:
            jaxe.write(bytearray([event.key, keyup]))
            pass


pygame.init()

size = width, height = 128, 64
black = 0, 0, 0
white = 255, 255, 255

screen = pygame.display.set_mode(size)

font = pygame.font.SysFont(None, 24)
img = font.render('Press any key', True, white)
screen.blit(img, (10, 20))
pygame.display.flip()

jaxe = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate=500000,
    timeout=0.1
)

while 1:
    handle_input()
    pixels = get_pixels()
    if len(pixels) == 1024:
        draw_pixels(pixels)
        pygame.display.flip()
