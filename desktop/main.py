# Importing Libraries

import serial
import pygame

pygame.init()

win = pygame.display.set_mode((276, 480))
pygame.display.set_caption("Drum Machine")
comm_port = "COM11"

arduino = None
try:
    arduino = serial.Serial(port=comm_port, baudrate=9600, timeout=.1)
except serial.SerialException:
    print("cannot connect to arduino")

pygame.mixer.init()
metronome = pygame.mixer.Sound("./audio/metronome.mp3")

sounds = [
    pygame.mixer.Sound("./audio/kick.mp3"),
    pygame.mixer.Sound("./audio/snare.mp3"),
    pygame.mixer.Sound("./audio/hi-hat.mp3")
]

logo = pygame.image.load("./logo.png").convert()

pattern_0_beat_metrix = [[False for i in range(16)] for j in range(8)]
pattern_1_beat_metrix = [[False for i in range(16)] for j in range(8)]
pattern_2_beat_metrix = [[False for i in range(16)] for j in range(8)]
pattern_3_beat_metrix = [[False for i in range(16)] for j in range(8)]

run = True

color_index = 100
color_dif = -10

lastCommLine = ""
currentPatternIndex = 0

font = pygame.font.SysFont('consolas', 16)
smallFont = pygame.font.SysFont('consolas', 12)
colorGrey = (150, 150, 150)

disconnectionError = False

while run:

    # resetting matrices when there is an error
    if disconnectionError:
        pattern_0_beat_metrix = [[False for i in range(16)] for j in range(8)]
        pattern_1_beat_metrix = [[False for i in range(16)] for j in range(8)]
        pattern_2_beat_metrix = [[False for i in range(16)] for j in range(8)]
        pattern_3_beat_metrix = [[False for i in range(16)] for j in range(8)]

    pygame.time.delay(int(1000 / 100))

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            run = False
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_a:
                currentPatternIndex = currentPatternIndex - 1 if currentPatternIndex > 0 else 0
            if event.key == pygame.K_s:
                currentPatternIndex = currentPatternIndex + 1 if currentPatternIndex < 3 else 3

    try:
        if arduino is not None:
            disconnectionError = False
            line = str(arduino.readline().decode("utf-8")).strip()

            if line != "":
                print(line)
                lastCommLine = line

            if line.split(" ")[0] == 'e': # Edit mode
                patternIndex = int(line.split(" ")[1])
                instrumentIndex = int(line.split(" ")[2])
                beatIndex = int(line.split(" ")[3])
                val = line.split(" ")[4] == 'ON'
                if patternIndex == 0: pattern_0_beat_metrix[instrumentIndex][beatIndex] = val
                if patternIndex == 1: pattern_1_beat_metrix[instrumentIndex][beatIndex] = val
                if patternIndex == 2: pattern_2_beat_metrix[instrumentIndex][beatIndex] = val
                if patternIndex == 3: pattern_3_beat_metrix[instrumentIndex][beatIndex] = val

            if line.split(" ")[0] == 'p': # Play mode
                instrument_index = int(line.split(" ")[1])
                sounds[instrument_index].play()

        else:
            # try re-establishing connection
            try:
                disconnectionError = True
                arduino = serial.Serial(port=comm_port, baudrate=9600, timeout=.1)
            except serial.SerialException: pass

    except serial.SerialException:
        # try re-establishing connection
        try:
            disconnectionError = True
            arduino = serial.Serial(port=comm_port, baudrate=9600, timeout=.1)
        except serial.SerialException: pass

    # reset screen
    win.fill((0, 0, 0))

    # draw logo
    win.blit(logo, (10, 0))

    # text
    patternIndexDisplayText = (font.render('Pattern Index: ' + str(currentPatternIndex), True, colorGrey))
    win.blit(patternIndexDisplayText, (12, 300))
    lastCommLineText = (font.render('Comm: ' + lastCommLine, True, colorGrey))
    win.blit(lastCommLineText, (12, 324))

    # prev button
    pygame.draw.rect(win, colorGrey, (10, 360, 36, 36), 1, 4)
    prevPatternHelpLabel = (font.render('A', True, colorGrey))
    win.blit(prevPatternHelpLabel, (18, 372))
    prevPatternHelpLabel2 = (smallFont.render('Prev', True, colorGrey))
    win.blit(prevPatternHelpLabel2, (12, 400))

    # next button
    pygame.draw.rect(win, colorGrey, (56, 360, 36, 36), 1, 4)
    nextPatternHelpLabel = (font.render('S', True, colorGrey))
    win.blit(nextPatternHelpLabel, (64, 372))
    nextPatternHelpLabel2 = (smallFont.render('Next', True, colorGrey))
    win.blit(nextPatternHelpLabel2, (58, 400))

    # errors
    errorLabel = (font.render('NOT CONNECTED' if disconnectionError else '', True, (255, 0, 0)))
    win.blit(errorLabel, (12, 428))

    # credit
    credit = (smallFont.render('Check out github.com/captainayan', True, colorGrey))
    win.blit(credit, (12, 458))

    # select beat matrix to draw
    beat_matrix = []
    if currentPatternIndex == 0: beat_matrix = pattern_0_beat_metrix
    elif currentPatternIndex == 1: beat_matrix = pattern_1_beat_metrix
    elif currentPatternIndex == 2: beat_matrix = pattern_2_beat_metrix
    elif currentPatternIndex == 3: beat_matrix = pattern_3_beat_metrix

    # draw matrix
    for i in range(8): # instrument
        for j in range(16): # beat
            pygame.draw.rect(win,
                               (32*i, 16*j, color_index),
                               (16 * j + 12, 16 * i + 160, 12, 12),
                               6 if beat_matrix[i][j] else 1,
                             2)

    # color change matrix
    if color_index > 240: color_dif = -10
    elif color_index < 150: color_dif = 10
    color_index += color_dif

    pygame.display.update()

pygame.quit()

