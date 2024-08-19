#include <Arduino.h>

#include "keyboard.h"

#define BLUE_LED_PIN 25
#define PREV_BUTTON_PIN 16
#define NEXT_BUTTON_PIN 17

#define KEY_UP_ARROW 0x52
#define KEY_DOWN_ARROW 0x51

BLEKeyboard keyboard("Cupping Clicker", "0x08.in");

void setup() {
  Serial.begin(115200);

  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(PREV_BUTTON_PIN, INPUT_PULLUP);
  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(BLUE_LED_PIN, 0);

  keyboard.Start();
}

bool old_prev_state = false;
bool old_next_state = false;
uint64_t last_prev_state_change = 0;
uint64_t last_next_state_change = 0;

void loop() {
  digitalWrite(BLUE_LED_PIN, keyboard.IsConnected() ? HIGH : LOW);

  bool current_prev_state = !digitalRead(PREV_BUTTON_PIN);
  if ((millis() - last_prev_state_change) > 100 && current_prev_state != old_prev_state) {
    old_prev_state = current_prev_state;
    last_prev_state_change = millis();
    if (current_prev_state) {
      keyboard.Press(KEY_UP_ARROW);
      delay(20);
      keyboard.Release(KEY_UP_ARROW);
    }
  }

  bool current_next_state = !digitalRead(NEXT_BUTTON_PIN);
  if ((millis() - last_next_state_change) > 100 && current_next_state != old_next_state) {
    old_next_state = current_next_state;
    last_next_state_change = millis();
    if (current_next_state) {
      keyboard.Press(KEY_DOWN_ARROW);
      delay(20);
      keyboard.Release(KEY_DOWN_ARROW);
    }
  }

  delay(10);
}