#include "keyboard.h"

#include <Arduino.h>
#include <BLE2902.h>

static uint8_t kReportMap[] = {
    USAGE_PAGE(1), 0x01,  // USAGE_PAGE (Generic Desktop Ctrls)
    USAGE(1), 0x06,       // USAGE (Keyboard)
    COLLECTION(1), 0x01,  // COLLECTION (Application)
    // ------------------------------------------------- Keyboard
    REPORT_ID(1), 0x01,        //   REPORT_ID (1)
    USAGE_PAGE(1), 0x07,       //   USAGE_PAGE (Kbrd/Keypad)
    USAGE_MINIMUM(1), 0xE0,    //   USAGE_MINIMUM (0xE0)
    USAGE_MAXIMUM(1), 0xE7,    //   USAGE_MAXIMUM (0xE7)
    LOGICAL_MINIMUM(1), 0x00,  //   LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1), 0x01,  //   Logical Maximum (1)
    REPORT_SIZE(1), 0x01,      //   REPORT_SIZE (1)
    REPORT_COUNT(1), 0x08,     //   REPORT_COUNT (8)
    HIDINPUT(1),
    0x02,  //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    REPORT_COUNT(1), 0x01,  //   REPORT_COUNT (1) ; 1 byte (Reserved)
    REPORT_SIZE(1), 0x08,   //   REPORT_SIZE (8)
    HIDINPUT(1),
    0x01,  //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    REPORT_COUNT(1),
    0x05,  //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
    REPORT_SIZE(1), 0x01,    //   REPORT_SIZE (1)
    USAGE_PAGE(1), 0x08,     //   USAGE_PAGE (LEDs)
    USAGE_MINIMUM(1), 0x01,  //   USAGE_MINIMUM (0x01) ; Num Lock
    USAGE_MAXIMUM(1), 0x05,  //   USAGE_MAXIMUM (0x05) ; Kana
    HIDOUTPUT(1),
    0x02,  //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    REPORT_COUNT(1), 0x01,  //   REPORT_COUNT (1) ; 3 bits (Padding)
    REPORT_SIZE(1), 0x03,   //   REPORT_SIZE (3)
    HIDOUTPUT(1),
    0x01,  //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    REPORT_COUNT(1), 0x06,     //   REPORT_COUNT (6) ; 6 bytes (Keys)
    REPORT_SIZE(1), 0x08,      //   REPORT_SIZE(8)
    LOGICAL_MINIMUM(1), 0x00,  //   LOGICAL_MINIMUM(0)
    LOGICAL_MAXIMUM(1), 0x65,  //   LOGICAL_MAXIMUM(0x65) ; 101 keys
    USAGE_PAGE(1), 0x07,       //   USAGE_PAGE (Kbrd/Keypad)
    USAGE_MINIMUM(1), 0x00,    //   USAGE_MINIMUM (0)
    USAGE_MAXIMUM(1), 0x65,    //   USAGE_MAXIMUM (0x65)
    HIDINPUT(1),
    0x00,  //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    END_COLLECTION(0),  // END_COLLECTION
};

void BLEKeyboardCallbacks::onConnect(BLEServer* server) {
  this->keyboard_.OnConnect();
}

void BLEKeyboardCallbacks::onDisconnect(BLEServer* server) {
  this->keyboard_.OnDisconnect();
}

void BLEKeyboard::Start() {
  BLEDevice::init(this->name_.c_str());
  this->server_ = BLEDevice::createServer();
  this->server_->setCallbacks(new BLEKeyboardCallbacks(*this));

  this->hid_device_ = new BLEHIDDevice(this->server_);
  this->input_characteristic_ = this->hid_device_->inputReport(1);
  this->output_characteristic_ = this->hid_device_->outputReport(1);

  this->hid_device_->manufacturer()->setValue(this->manufacturer_);
  this->hid_device_->pnp(0x02, 0xE502, 0xA111, 0x0210);
  this->hid_device_->hidInfo(0, 1);

  this->security_ = new BLESecurity();
  this->security_->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  this->hid_device_->reportMap(kReportMap, sizeof(kReportMap));
  this->hid_device_->startServices();

  this->hid_device_->setBatteryLevel(100);

  this->advertising_ = this->server_->getAdvertising();
  this->advertising_->setAppearance(HID_KEYBOARD);
  this->advertising_->addServiceUUID(
      this->hid_device_->hidService()->getUUID());
  this->advertising_->addServiceUUID(
      this->hid_device_->deviceInfo()->getUUID());
  this->advertising_->addServiceUUID(
      this->hid_device_->batteryService()->getUUID());
  this->advertising_->start();
}

void BLEKeyboard::OnConnect() {
  const auto ble_2902_descriptor =
      static_cast<BLE2902*>(this->input_characteristic_->getDescriptorByUUID(
          BLEUUID(static_cast<uint16_t>(0x2902))));
  ble_2902_descriptor->setNotifications(true);
  this->connected_ = true;
}

void BLEKeyboard::OnDisconnect() {
  this->connected_ = false;
  const auto ble_2902_descriptor =
      static_cast<BLE2902*>(this->input_characteristic_->getDescriptorByUUID(
          BLEUUID(static_cast<uint16_t>(0x2902))));
  ble_2902_descriptor->setNotifications(false);
  this->advertising_->start();
}

void BLEKeyboard::Press(uint8_t key) {
  this->key_states_[key] = true;
  this->SendHIDReport();
}

void BLEKeyboard::Release(uint8_t key) {
  this->key_states_[key] = false;
  this->SendHIDReport();
}

struct __attribute__((__packed__)) InputReport {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t pressed_keys[6];
};

void BLEKeyboard::SendHIDReport() {
  InputReport report = {};

  uint8_t pressed_key_count = 0;
  for (size_t i = 0; i < 256 && pressed_key_count < 6; i++) {
    if (this->key_states_[i]) {
      report.pressed_keys[pressed_key_count] = i;
      pressed_key_count++;
    }
  }

  this->input_characteristic_->setValue(reinterpret_cast<uint8_t*>(&report),
                                        sizeof(report));
  this->input_characteristic_->notify();

  delay(5);
}

bool BLEKeyboard::IsConnected() const {
  return this->connected_;
}