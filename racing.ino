/*
  Addressable LED Strip Racing Game
  - Connect any number of buttons
  - Hold the button to accelerate
  - Release the button to decelerate
  - Accelerating in a turn zone limits the speed until the end of the turn
*/

#define LED_PIN 2     // LED strip pin
#define LED_NUM 120   // Number of LEDs
#define LED_BR 250    // LED brightness

// Button pins for each player
const byte pins[] = {3, 4};

#define MAX_SPEED 15  // Maximum speed
#define MIN_SPEED 4   // Maximum speed in turn zones
#define TURN_ZONES 2  // Number of turn zones
#define TURN_MIN 10   // Minimum turn length
#define TURN_MAX 30   // Maximum turn length
#define WIN_SCORE 50  // Winning score (laps)

#define BUZZER_PIN 6 // Pin connected to the buzzer

// =============================================
#include "FastLED.h"
CRGB leds[LED_NUM];
int turns[TURN_ZONES][2];
CRGB turnColor = CHSV(12, 255, 70);
const byte players = sizeof(pins);  // Number of players

// ============ SETUP =============
void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_NUM);
  FastLED.setBrightness(LED_BR);

  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer pin as output

  // Set up all buttons
  for (int p = 0; p < players; p++) pinMode(pins[p], INPUT_PULLUP);

  randomSeed(analogRead(0));              // Improve randomness
  int turnSize = LED_NUM / TURN_ZONES;    // Block length as number of LEDs / number of turns
  for (int t = 0; t < TURN_ZONES; t++) {  // For each turn zone
    // Find the start of the turn within each block
    turns[t][0] = random(t * turnSize, (t + 1) * turnSize - TURN_MAX);

    // Find the end of the turn by adding length to the start
    turns[t][1] = turns[t][0] + random(TURN_MIN, TURN_MAX);
  }
  newGame();    // Start a new game
}

// ========== VARIABLES ==========
int pos[players];       // Positions of cars
int spd[players];       // Speed of cars
int score[players];     // Scores
bool drag[players];     // Drag flags

// ============= LOOP =============
void loop() {
  static uint32_t tmr, tmr2;
  // Speed calculation
  // Timer for 200 ms
  if (millis() - tmr2 >= 200) {
    tmr2 = millis();
    for (int p = 0; p < players; p++) {         // For each player
      if (!digitalRead(pins[p])) spd[p] += 3;   // If button is pressed, accelerate
      else spd[p]--;                            // Otherwise, decelerate

      // Limit by max speed or speed in turn zone
      spd[p] = constrain(spd[p], 0, drag[p] ? MIN_SPEED : MAX_SPEED);
    }
  }

  // Car movement
  // Timer for 10 ms
  if (millis() - tmr >= 10) {
    tmr = millis();
    FastLED.clear();  // Clear the LED strip

    // Display turn zones
    for (int t = 0; t < TURN_ZONES; t++) {              // For each turn zone
      for (int s = turns[t][0]; s < turns[t][1]; s++) { // From start to end
        leds[s] = turnColor;    // Color the pixel
      }
    }

    // For each player
    for (int p = 0; p < players; p++) {
      pos[p] += spd[p];   // Move at current speed

      if (!digitalRead(pins[p])) {  // If button is pressed
        for (int t = 0; t < TURN_ZONES; t++) {    // Check turn zones
          // Car is inside a turn
          if (pos[p] / 10 >= turns[t][0] && pos[p] / 10 < turns[t][1]) {
            drag[p] = 1;            // Set drag flag
            spd[p] = MIN_SPEED;     // Immediately limit speed
          } else drag[p] = 0;       // Button not pressed, remove limitation
        }
      } else drag[p] = 0;       // Button not pressed, remove limitation

      // Check end of track
      if (pos[p] >= (LED_NUM * 10)) {   // Strip ends, completed a lap
        pos[p] -= LED_NUM * 10;         // Subtract a lap
        score[p]++;                     // Increase score

        // Reached winning score
        if (score[p] >= WIN_SCORE) {
          // Activate buzzer when a player wins
          digitalWrite(BUZZER_PIN, HIGH);
          delay(1000);  // Buzzer on for 1 second
          digitalWrite(BUZZER_PIN, LOW);

          // Gradually color the strip with the winner's color
          for (int led = 0; led < LED_NUM; led++) {
            leds[led].setHue(p * 255 / players);
            FastLED.show();
            delay(30);
          }
          newGame();
          return;   // Exit to next cycle
        }
      }

      // If not won, draw current point with player's color
      leds[pos[p] / 10].setHue(p * 255 / players);

    }   // End of for players
    FastLED.show();
  } // End of timer
}

void newGame() {
  // Reset scores and speeds of players
  for (int np = 0; np < players; np++) {
    pos[np] = spd[np] = score[np] = 0;
    drag[np] = 0;
  }
}
