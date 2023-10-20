# Importing Libraries

import serial
import pygame

pygame.init()

win = pygame.display.set_mode((720, 480))
pygame.display.set_caption("Drum Machine")

arduino = serial.Serial(port='COM11', baudrate=9600, timeout=.1)

metronome = pygame.mixer.Sound("./audio/metronome.mp3")

sounds = [
    pygame.mixer.Sound("./audio/kick.mp3"),
    pygame.mixer.Sound("./audio/snare.mp3"),
    pygame.mixer.Sound("./audio/hi-hat.mp3")
]

p = [[False for i in range(16)] for j in range(8)]

pattern_0_beat_metrix = [[False for i in range(16)] for j in range(8)]
pattern_1_beat_metrix = [[False for i in range(16)] for j in range(8)]
pattern_2_beat_metrix = [[False for i in range(16)] for j in range(8)]
pattern_3_beat_metrix = [[False for i in range(16)] for j in range(8)]

run = True

while run:

    pygame.time.delay(int(1000 / 60))

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            run = False

    try:
        if arduino != None:
            line = str(arduino.readline().decode("utf-8")).strip()

            if line != "":
                print(line)

            if line.split(" ")[0] == 'e': # Edit mode
                patternIndex = int(line.split(" ")[1])
                instrumentIndex = int(line.split(" ")[2])
                beatIndex = int(line.split(" ")[3])
                val = line.split(" ")[4] == 'ON'
                if patternIndex == 0: pattern_0_beat_metrix[instrumentIndex][beatIndex] = val
                if patternIndex == 1: pattern_1_beat_metrix[instrumentIndex][beatIndex] = val
                if patternIndex == 2: pattern_2_beat_metrix[instrumentIndex][beatIndex] = val
                if patternIndex == 3: pattern_3_beat_metrix[instrumentIndex][beatIndex] = val

                print(pattern_0_beat_metrix)

            if line.split(" ")[0] == 'p': # Play mode
                instrument_index = int(line.split(" ")[1])
                sounds[instrument_index].play()

        else:
            if arduino.isOpen():
                arduino.close()
                print("Disconnected.")
            else:
                arduino.open()
                print("Connected.")

    except serial.SerialException:
        print("An exception occurred (serial communication)")
        arduino.open() # try to reopen

    win.fill((0, 0, 0))

    for i in range(8): # instrument
        for j in range(16): # beat
            pygame.draw.rect(win,
                               (32*i, 16*j, 255),
                               (24 * j + 16, 16 * i + 12, 16, 12),
                               6 if pattern_0_beat_metrix[i][j] else 1,
                             2)

    pygame.display.update()

pygame.quit()


def print_pat0():
    for row in pattern_0_beat_metrix:
        for element in row:
            print(1 if element else 0, end=" ")
        print()
