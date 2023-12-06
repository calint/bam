#!/bin/python3
from PIL import Image
import sys

def print_sprites_as_game_resource(filename):
    try:
        with Image.open(filename) as img:
            if img.mode != "P":
                print("Error: The PNG is not paletted.")
                return

            width, height = img.size
            ix = 0
            for row in range(0, height, 16):
                for column in range(0, width, 16):
                    print("{ //", ix)
                    ix += 1
                    for y in range(row, row + 16):
                        for x in range(column, column + 16):
                            pixel = img.getpixel((x, y))
                            print(f"0x{pixel:02X},", end="")
                        print()
                    print("},")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: read-sprites <filename>")
        sys.exit(1)
    print_sprites_as_game_resource(sys.argv[1])
