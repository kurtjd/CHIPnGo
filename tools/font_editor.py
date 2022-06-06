#!/usr/bin/python3

import sys
import pygame


def draw_grid():
    for i in range(1, 5):
        pygame.draw.line(screen, white, (i * 100, 0), (i * 100, 500))
        pygame.draw.line(screen, white, (0, i * 100), (500, i * 100))


def draw_bmp():
    for i, row in enumerate(bmp):
        for j, col in enumerate(row):
            pygame.draw.rect(
                screen,
                white if col else black,
                pygame.Rect(j * 100, i * 100, 100, 100),
            )


def flip_pixel(pos):
    col = pos[0] // 100
    row = pos[1] // 100

    bmp[row][col] = int(not bmp[row][col])


def print_hex():
    print("{", end="")

    for j in range(len(bmp[0])):
        bstr = "000"
        for i in reversed(range(len(bmp))):
            bstr += str(bmp[i][j])

        print(f"0x{int(bstr, 2):02X}", end="")

        if j < len(bmp[0]) - 1:
            print(", ", end="")

    print("},")


def handle_input():
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            sys.exit()
        elif event.type == pygame.MOUSEBUTTONUP:
            flip_pixel(pygame.mouse.get_pos())
        elif event.type == pygame.KEYUP and event.key == pygame.K_RETURN:
            print_hex()


pygame.init()

white = (255, 255, 255)
black = (0, 0, 0)
size = width, height = 500, 500
screen = pygame.display.set_mode(size)

bmp = [
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0],
]

print("\n\n\n")
while 1:
    handle_input()
    screen.fill(black)
    draw_grid()
    draw_bmp()
    pygame.display.flip()
