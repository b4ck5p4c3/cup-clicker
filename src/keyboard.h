#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <string>

#include <BLEDevice.h>
#include <BLEHIDDevice.h>

class BLEKeyboard;

class BLEKeyboardCallbacks : public BLEServerCallbacks {
 public:
  BLEKeyboardCallbacks(BLEKeyboard& keyboard) : keyboard_(keyboard) {}

  void onConnect(BLEServer* server);
  void onDisconnect(BLEServer* server);

 private:
  BLEKeyboard& keyboard_;
};

class BLEKeyboard {
 public:
  friend BLEKeyboardCallbacks;

  BLEKeyboard(const std::string& name, const std::string& manufacturer)
      : name_(name), manufacturer_(manufacturer) {};

  void Start();
  void Press(uint8_t key);
  void Release(uint8_t key);
  bool IsConnected() const;

 private:
  void OnConnect();
  void OnDisconnect();
  void SendHIDReport();

  bool key_states_[256];

  std::string name_;
  std::string manufacturer_;

  BLEServer* server_;
  BLEHIDDevice* hid_device_;
  BLECharacteristic* input_characteristic_;
  BLECharacteristic* output_characteristic_;
  BLESecurity* security_;
  BLEAdvertising* advertising_;

  bool connected_;
};

#endif