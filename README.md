# RaiderFPS

RaiderFPS is a small first-person-style maze shooter firmware project for an Arduino Nano. The game renders a simple 3D-ish view on a 128x64 OLED display and lets the player explore a maze, fight enemies, and reach the exit.

## Features

- Title, gameplay, win, and lose states
- Maze-based movement and turning
- Enemy AI with chase and melee behavior
- Simple shooting and hit feedback
- Buzzer sound effects for game events

## Hardware

This project is designed for:

- Arduino Nano (ATmega328P)
- SSD1306 128x64 OLED display over I2C
- Four input buttons
- Buzzer on digital pin 9

## Controls

- Left button: turn left
- Right button: turn right
- Forward button: move forward
- Fire button: shoot and start the game

## Build and Upload

This project uses PlatformIO.

1. Open the project folder in VS Code.
2. Ensure the PlatformIO extension is installed.
3. Build the firmware with:

   ```bash
   pio run
   ```

4. Upload it to the board:

   ```bash
   pio run -t upload
   ```

5. Optionally monitor the serial output:

   ```bash
   pio device monitor
   ```

## Project Structure

- src/main.cpp: main game logic, rendering, input handling, and enemy behavior
- platformio.ini: PlatformIO board and dependency configuration
- lib/: project libraries
- include/: project headers

## Dependencies

- U8g2 for OLED graphics
