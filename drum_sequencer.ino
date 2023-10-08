
#include <math.h>
#include <PCM.h>
#define DEBUG false

short potPin0 = A0; // Potentiometer output connected to analog pin 0 (A0)
short potPin1 = A4; // Potentiometer output connected to analog pin 4 (A4)
short ledPins[] = {8, 9, 13, 12, 10}; // led 0, led 1, led 2, led 3, mode led
short buttonPins[] = {2, 3, 4, 5, 6, 7}; // BTN 0, BTN 1, BTN 2, BTN 3, FN BTN 0, FN BTN 1
boolean buttonStates[] = {LOW, LOW, LOW, LOW, LOW, LOW}; // whether a button is pressed or not

boolean sequence[4][8][16] = {};

// For editing mode (select the bar which is to be edited)
short barIndex = 0; // -1 < barIndex < 4
short instrumentIndex = 0; // -1 < instrumentIndex < 8
short patternIndex = 0; // -1 < patternIndex < 4

// For playing
unsigned short bpm = 80; // 50 < bpm < 254
short currentBeatIndex = -1; // index of a beat in a measure (-1 < currentBeatIndex < 16)
unsigned short delayBetweenBeats = (60000 / bpm) / 4; // time in-between every 1/4th beat
long lastMillis = 0;
short queuedPatternIndex = -1; // index of the pattern that is queued to play after the current measure (-1 means no pattern queued)
short currentPatternIndex = 0; // the current pattern that is being played

// Modes
const char EDIT_MODE = 0; // editing a selected bar [DEFAULT]
const char SELECTION_MODE = 1; // selecting a bar
const char FUNCTION_MODE = 2; // other actions
const char PLAY_MODE = 3; // for playing
char currentMode = EDIT_MODE; // setting default mode

// Sub-Modes of Function Mode
const char DEFAULT_FUNCTION_MODE = 0; // blank display
const char METRONOME_FUNCTION_MODE = 1; // for metronome display and adjustment [a FUNCTION_MODE sub-mode]
char currentFunctionMode = DEFAULT_FUNCTION_MODE; // setting default mode

// Animations
const boolean EDIT_MODE_ANIMATION[6][4] = {
  {0, 0, 0, 1}, {0, 0, 1, 1}, {0, 1, 1, 1}, {1, 1, 1, 0},
  {1, 1, 0, 0}, {1, 0, 0, 0},
};
const boolean SELECTION_MODE_ANIMATION[6][4] = {
  {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 1, 0}, {0, 1, 1, 1},
  {0, 0, 1, 1}, {0, 0, 0, 1},
};
const boolean BAR_ACTION_MODE_ANIMATION[6][4] = {
  {0, 1, 0, 0}, {0, 1, 1, 0}, {1, 1, 1, 1}, {1, 1, 1, 1},
  {0, 1, 1, 0}, {0, 1, 0, 0},
};
const boolean WELCOME_ANIMATION[49][4] = {
  // first stage
  {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, // move
  {0, 0, 0, 1}, {0, 0, 0, 1}, // wait
  {0, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}, // move
  {1, 0, 0, 0}, {1, 0, 0, 0}, // wait
  {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, // move
  {0, 0, 0, 1}, {0, 0, 0, 1}, // wait
  {0, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, // move
  {0, 0, 0, 0}, {0, 0, 0, 0}, // wait

  // second stage
  {0, 1, 1, 0}, {0, 1, 1, 0},
  {1, 0, 0, 1}, {1, 0, 0, 1},
  {0, 1, 1, 0}, {0, 1, 1, 0},
  {1, 0, 0, 1}, {1, 0, 0, 1},
  {0, 1, 1, 0}, {0, 1, 1, 0},
  {1, 0, 0, 1}, {1, 0, 0, 1},
  {0, 0, 0, 0}, {0, 0, 0, 0},

  // third stage
  {1, 1, 1, 1}, {1, 1, 1, 1},
  {0, 0, 0, 0}, {0, 0, 0, 0},
  {1, 1, 1, 1}, {1, 1, 1, 1},
  {0, 0, 0, 0}, {0, 0, 0, 0},
  {1, 1, 1, 1}, {1, 1, 1, 1},

  // forth stage
  {0, 1, 1, 0}, {0, 1, 1, 0},
  {0, 1, 0, 0},
};

const unsigned short MODE_TRANSITION_ANIMATION_FRAME_DELAY = 40;

/**
 * This array holds value of 1 to 8
 * Use this to show indexes on the LED display
 */
const boolean INDEX_INDICATOR_ANIMATIONS[8][3][4] = {
  {{0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 1}},
  {{0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},

  {{0, 0, 1, 1}, {0, 0, 0, 0}, {0, 0, 1, 1}},
  {{0, 1, 0, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}},

  {{0, 1, 0, 1}, {0, 0, 0, 0}, {0, 1, 0, 1}},
  {{0, 1, 1, 0}, {0, 0, 0, 0}, {0, 1, 1, 0}},
  
  {{0, 1, 1, 1}, {0, 0, 0, 0}, {0, 1, 1, 1}},
  {{1, 0, 0, 0}, {0, 0, 0, 0}, {1, 0, 0, 0}},
};

// Sound Samples
const unsigned char sampleKick[] PROGMEM = {};
const unsigned char sampleSnare[] PROGMEM = {};
const unsigned char sampleHiHat[] PROGMEM = {};

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB
  Serial.println("start");

  for(size_t i = 0; i < sizeof(buttonPins) / sizeof(ledPins[0]); i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  for(size_t i = 0; i < sizeof(buttonPins) / sizeof(buttonPins[0]); i++) {
    pinMode(buttonPins[i], INPUT);
  }

  for(size_t i = 0; i < 4; i++) { // 4 patterns
    for(size_t j = 0; j < 8; j++) { // 8 instruments (kick, snare etc.)
      for(size_t k = 0; k < 16; k++) { // 16 beats
        sequence[i][j][k] = LOW;
      }
    }
  }

  animate(WELCOME_ANIMATION, 49, 50); // welcome animation

  pinMode(11, OUTPUT); // speaker pin
}

void loop() {
  buttonHandler();
  analogInputHandler();

  logicHandler();
  displayHandler();
}

/**
 * This function will run on every frame. It will call onButtonClickEnter and onButtonClickExit with
 * the corresponding button index.
 */
void buttonHandler() {
  for(size_t i = 0; i < sizeof(buttonPins) / sizeof(buttonPins[0]); i++) {
    boolean currentButtonState = digitalRead(buttonPins[i]);

    if (util_debounceButton(currentButtonState, buttonPins[i]) == HIGH && buttonStates[i] == LOW) {
      buttonStates[i] = HIGH;
      onButtonClickEnter(i);
    }
    else if (util_debounceButton(currentButtonState, buttonPins[i]) == LOW && buttonStates[i] == HIGH) {
      buttonStates[i] = LOW;
      onButtonClickExit(i);
    }
  }
}

void onButtonClickEnter(int buttonIndex) {}

void onButtonClickExit(int buttonIndex) {
  if(buttonIndex == 4) { // FN BTN 0
    // Switch between modes. EDIT_MODE -> SELECTION_MODE -> FUNCTION_MODE -> 🔁

    if (currentMode == EDIT_MODE) {
      animate(SELECTION_MODE_ANIMATION, 6, MODE_TRANSITION_ANIMATION_FRAME_DELAY);
      currentMode = SELECTION_MODE;
    }
    else if (currentMode == SELECTION_MODE) {
      animate(BAR_ACTION_MODE_ANIMATION, 6, MODE_TRANSITION_ANIMATION_FRAME_DELAY);
      currentMode = FUNCTION_MODE;

      // setting default sub-mode for FUNCTION_MODE
      currentFunctionMode = DEFAULT_FUNCTION_MODE;
    }
    else if (currentMode == FUNCTION_MODE) {
      // animate(EDIT_MODE_ANIMATION, 6, MODE_TRANSITION_ANIMATION_FRAME_DELAY);
      currentMode = EDIT_MODE;
    }
  }
  else if(buttonIndex == 5) { // FN BTN 1 (Play Button)
    if (currentMode == PLAY_MODE) currentMode = EDIT_MODE;
    else currentMode = PLAY_MODE;

    resetCurrentBeatIndex(); // reset beat index
    queuedPatternIndex = -1; // reset queued pattern
    currentPatternIndex = 0; // current pattern index set to the 0 (first pattern)
  }
  else { // BTN 0, BTN 1, BTN 2, BTN 3
    if (currentMode == EDIT_MODE) {
      short i = barIndex * 4 + buttonIndex;
      sequence[patternIndex][instrumentIndex][i] = sequence[patternIndex][instrumentIndex][i] == LOW ? HIGH : LOW;

      Serial.print("edit ");
      Serial.print(patternIndex);
      Serial.print(" ");
      Serial.print(instrumentIndex);
      Serial.print(" ");
      Serial.print(i);
      Serial.print(" ");
      Serial.println(sequence[patternIndex][instrumentIndex][i] ? "ON" : "OFF");

      #if DEBUG
      util_sequenceLog();
      #endif
    }
    else if (currentMode == SELECTION_MODE) {
      // if (buttonIndex == 0) changeBarIndex();
      // else if (buttonIndex == 1) changeInstrumentIndex();
      if (buttonIndex == 1) changeInstrumentIndex();
      else if (buttonIndex == 2) changePatternIndex();
      else if (buttonIndex == 3) displayIndices();
    }
    else if (currentMode == FUNCTION_MODE) {
      if (buttonIndex == 0) { // Toggle METRONOME_FUNCTION_MODE
        if (currentFunctionMode == METRONOME_FUNCTION_MODE) currentFunctionMode = DEFAULT_FUNCTION_MODE;
        else currentFunctionMode = METRONOME_FUNCTION_MODE;

        resetCurrentBeatIndex(); // reset beat index
      }
      else if (buttonIndex == 1) ;
      else if (buttonIndex == 2) ;
      else if (buttonIndex == 3) ;
    }
    else if (currentMode == PLAY_MODE) {
      if (buttonIndex == 0) queuedPatternIndex = queuedPatternIndex == 0 ? -1 : 0;
      else if (buttonIndex == 1) queuedPatternIndex = queuedPatternIndex == 1 ? -1 : 1;
      else if (buttonIndex == 2) queuedPatternIndex = queuedPatternIndex == 2 ? -1 : 2;
      else if (buttonIndex == 3) queuedPatternIndex = queuedPatternIndex == 3 ? -1 : 3;

      // Fill-in button
      // if (buttonIndex == 1) currentPatternIndex = 1;
      // else if (buttonIndex == 3) currentPatternIndex = 3;
    }
  }
}

void analogInputHandler() {
  int potVal0 = analogRead(potPin0);
  int potVal1 = analogRead(potPin1);

  if (currentMode == FUNCTION_MODE) {
    if (currentFunctionMode == METRONOME_FUNCTION_MODE) {
      bpm = (potVal0 / 5) + 50;
      delayBetweenBeats = (60000 / bpm) / 4;
    }
  }
  else if (currentMode == EDIT_MODE) {
    barIndex = potVal1 / 256;
  }
}

/**
 * This function will run on every frame. It will take care of the "business logic" that is required (scheduled)
 * to be executed after a delay, or without any user action (i.e. button press etc.).
 */
void logicHandler() {
  if (currentMode == FUNCTION_MODE) {
    if (currentFunctionMode == METRONOME_FUNCTION_MODE) { // Metronome display feature of FUNCTION_MODE
      long currentMillis = millis();
      if (currentMillis - lastMillis > delayBetweenBeats) {
        lastMillis = currentMillis;
        currentBeatIndex = currentBeatIndex == 15 ? 0 : currentBeatIndex + 1;

        if (currentBeatIndex % 4 == 0) tone(11, 80, 20);
      }
    }
  }
  else if (currentMode == PLAY_MODE) {
    long currentMillis = millis();
    if (currentMillis - lastMillis > delayBetweenBeats) {
      lastMillis = currentMillis;
      currentBeatIndex = currentBeatIndex == 15 ? 0 : currentBeatIndex + 1;

      if (sequence[currentPatternIndex][0][currentBeatIndex]) {
        startPlayback(sampleKick, sizeof(sampleKick));
        // tone(11, 70, 20);
        Serial.println("drum kick");
      }
      if (sequence[currentPatternIndex][1][currentBeatIndex]) {
        startPlayback(sampleSnare, sizeof(sampleSnare));
        // tone(11, 185, 20);
        Serial.println("drum snare");
      }
      if (sequence[currentPatternIndex][2][currentBeatIndex]) {
        startPlayback(sampleHiHat, sizeof(sampleHiHat));
        // tone(11, 1800, 20);
        Serial.println("drum hi_hat");
      }
      if (sequence[currentPatternIndex][3][currentBeatIndex]) {
        Serial.println("drum clap");
      }

      // switch to queued pattern on last beat (reset queued pattern)
      if (currentBeatIndex == 15 && queuedPatternIndex != -1) {
        currentPatternIndex = queuedPatternIndex;
        queuedPatternIndex = -1;
      }
    }
  }
}

/**
 * This function will run on every frame. It will update the LED display to show the current Bar (musical)
 * and other information.
 */
void displayHandler() {
  // LED 0, LED 1, LED 2, LED 3
  if (currentMode == EDIT_MODE) {
    for(size_t i = 0; i < 4; i++) {
      digitalWrite(ledPins[i], sequence[patternIndex][instrumentIndex][barIndex * 4 + i]);
    }
  }
  else if (currentMode == SELECTION_MODE) {
    for(size_t i = 0; i < 4; i++) digitalWrite(ledPins[i], LOW);
  }
  else if (currentMode == FUNCTION_MODE) {
    if (currentFunctionMode == METRONOME_FUNCTION_MODE) { // METRONOME_FUNCTION_MODE of FUNCTION_MODE
      for(size_t i = 0; i < 4; i++) digitalWrite(ledPins[i], LOW);
      digitalWrite(ledPins[((currentBeatIndex%4))], HIGH);
    }
    else {
      for(size_t i = 0; i < 4; i++) digitalWrite(ledPins[i], LOW);
    }
  }
  else if (currentMode == PLAY_MODE) {
    for(size_t i = 0; i < 4; i++) digitalWrite(ledPins[i], LOW);
    digitalWrite(ledPins[((currentBeatIndex / 4))], HIGH);
  }

  // LED 4 (mode led)
  if (currentMode == EDIT_MODE) analogWrite(ledPins[4], 0);
  else if (currentMode == SELECTION_MODE) analogWrite(ledPins[4], 50);
  else if (currentMode == FUNCTION_MODE) analogWrite(ledPins[4], 255);
  else analogWrite(ledPins[4], 0);

  if (currentMode == PLAY_MODE && queuedPatternIndex != -1) analogWrite(ledPins[4], 255);
}

/**
 * Change the current instrument that your editing
 */
void changeInstrumentIndex() {
  instrumentIndex = instrumentIndex == 7 ? 0 : instrumentIndex + 1;

  animate(INDEX_INDICATOR_ANIMATIONS[instrumentIndex], 3, 50);
}

/**
 * Change the current pattern that your editing
 *
 * Side Effect: Selects the first instrument when called
 */
void changePatternIndex() {
  patternIndex = patternIndex == 3 ? 0 : patternIndex + 1;
  instrumentIndex = 0;

  animate(INDEX_INDICATOR_ANIMATIONS[patternIndex], 3, 50);
}

/**
 * Show the currentPattern, and currentInstrument (in the same order) using the LED display
 * inside SELECTION_MODE
 */
void displayIndices() {
  // Pattern
  animate(INDEX_INDICATOR_ANIMATIONS[patternIndex], 3, 50);
  delay(500);

  // Instrument
  animate(INDEX_INDICATOR_ANIMATIONS[instrumentIndex], 3, 50);
  delay(500);
}

/**
 * Resets currentBeatIndex to -1
 */
void resetCurrentBeatIndex() {
  currentBeatIndex = -1;
}

/**
 * Animates using 0th -> 3rd LED
 */
void animate(boolean animationFrames[][4], int frameCount, int delayAmount) {
  Serial.println("ANIMATION STARTED");

  for(size_t i = 0; i < frameCount; i++) { // Frame by frame
    for(size_t j = 0; j < 4; j++) {
      digitalWrite(ledPins[j], animationFrames[i][j]);
    }
    delay(delayAmount);
  }

  Serial.println("ANIMATION ENDED");
}

/**
 * Debouces button
 * https://docs.arduino.cc/built-in-examples/digital/Debounce
 */
boolean util_debounceButton(boolean state, int buttonPin) {
  boolean stateNow = digitalRead(buttonPin);
  if (state != stateNow) {
    delay(10);
    stateNow = digitalRead(buttonPin);
  }
  return stateNow;
}


void util_sequenceLog() {
  Serial.println("=====================\n");
  for(size_t i = 0; i < 4; i++) {
    for(size_t j = 0; j < 8; j++) {
      Serial.print("|");
      for(size_t k = 0; k < 16; k++) {
        Serial.print(sequence[i][j][k]);
        if((k+1)%4 == 0) Serial.print("|");
      }
      Serial.println();
    }
    Serial.println("---------------------");
  }
}

