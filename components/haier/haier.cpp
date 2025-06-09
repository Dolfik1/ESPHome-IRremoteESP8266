#include "haier.h"
#include "esphome/core/log.h"

namespace esphome {
namespace haier {

static const char *const TAG = "haier.climate";

// Constants from the IR protocol analysis
const uint16_t HAIER_HEADER_MARK = 3000;
const uint16_t HAIER_HEADER_SPACE = 4300;
const uint16_t HAIER_BIT_MARK = 520;
const uint16_t HAIER_ONE_SPACE = 1650;
const uint16_t HAIER_ZERO_SPACE = 650;
const uint32_t HAIER_MIN_GAP = 150000;

// Temperature constants
const uint8_t HAIER_TEMP_MIN = 16;
const uint8_t HAIER_TEMP_MAX = 30;
const uint8_t HAIER_TEMP_OFFSET = 16;

// Mode constants
const uint8_t HAIER_MODE_AUTO = 0b000;
const uint8_t HAIER_MODE_COOL = 0b001;
const uint8_t HAIER_MODE_DRY = 0b010;
const uint8_t HAIER_MODE_HEAT = 0b100;
const uint8_t HAIER_MODE_FAN = 0b110;

// Fan constants
const uint8_t HAIER_FAN_HIGH = 0b001;
const uint8_t HAIER_FAN_MED = 0b010;
const uint8_t HAIER_FAN_LOW = 0b011;
const uint8_t HAIER_FAN_AUTO = 0b101;

// Swing constants
const uint8_t HAIER_SWING_OFF = 0x0;
const uint8_t HAIER_SWING_VERTICAL = 0xC;
const uint8_t HAIER_SWING_HORIZONTAL = 0x7;
const uint8_t HAIER_SWING_BOTH = 0xF;

// Button/Command constants
const uint8_t HAIER_BUTTON_POWER = 0x05;
const uint8_t HAIER_BUTTON_MODE = 0x06;
const uint8_t HAIER_BUTTON_FAN = 0x04;
const uint8_t HAIER_BUTTON_TEMP_UP = 0x00;
const uint8_t HAIER_BUTTON_TEMP_DOWN = 0x01;
const uint8_t HAIER_BUTTON_SWING_V = 0x02;
const uint8_t HAIER_BUTTON_SWING_H = 0x03;

void HaierClimate::setup() {
  climate_ir::ClimateIR::setup();
  
  // Initialize with a known state
  remote_state_[0] = 0xA6;  // Model identifier for YRW02
  remote_state_[1] = 0x00;  // SwingV and Temp
  remote_state_[2] = 0x00;  // SwingH
  remote_state_[3] = 0x00;  // Health and Timer
  remote_state_[4] = 0x40;  // Power on
  remote_state_[5] = 0x60;  // Fan settings
  remote_state_[6] = 0x00;  // Turbo/Quiet
  remote_state_[7] = 0x20;  // Mode
  remote_state_[8] = 0x00;  // Sleep
  remote_state_[9] = 0x00;
  remote_state_[10] = 0x00;
  remote_state_[11] = 0x00;
  remote_state_[12] = 0x00; // Button
  
  this->apply_state_();
}

void HaierClimate::transmit_state() {
  this->encode_state_();
  
  ESP_LOGD(TAG, "Sending Haier code: %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
           remote_state_[0], remote_state_[1], remote_state_[2], remote_state_[3],
           remote_state_[4], remote_state_[5], remote_state_[6], remote_state_[7],
           remote_state_[8], remote_state_[9], remote_state_[10], remote_state_[11],
           remote_state_[12]);
  
  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  
  data->set_carrier_frequency(38000);
  
  // Header
  data->mark(HAIER_HEADER_MARK);
  data->space(HAIER_HEADER_MARK);
  data->mark(HAIER_HEADER_MARK);
  data->space(HAIER_HEADER_SPACE);
  
  // Data
  for (uint8_t i = 0; i < 13; i++) {
    for (uint8_t bit = 0; bit < 8; bit++) {
      data->mark(HAIER_BIT_MARK);
      if ((remote_state_[i] >> bit) & 1) {
        data->space(HAIER_ONE_SPACE);
      } else {
        data->space(HAIER_ZERO_SPACE);
      }
    }
  }
  
  // Footer
  data->mark(HAIER_BIT_MARK);
  data->space(HAIER_MIN_GAP);
  
  transmit.perform();
}

bool HaierClimate::on_receive(remote_base::RemoteReceiveData data) {
  // Check header
  if (!data.expect_item(HAIER_HEADER_MARK, HAIER_HEADER_MARK) ||
      !data.expect_item(HAIER_HEADER_MARK, HAIER_HEADER_SPACE)) {
    return false;
  }
  
  return this->decode_state_(data);
}

void HaierClimate::encode_state_() {
  // Reset state while keeping model identifier
  remote_state_[1] = 0x00;
  remote_state_[2] = 0x00;
  remote_state_[3] = 0x00;
  remote_state_[4] = 0x00;
  remote_state_[5] = 0x00;
  remote_state_[6] = 0x00;
  remote_state_[7] = 0x00;
  remote_state_[8] = 0x00;
  remote_state_[12] = 0x00;
  
  // Power state
  if (this->mode != climate::CLIMATE_MODE_OFF) {
    remote_state_[4] |= 0x40;  // Power on bit
    remote_state_[12] = HAIER_BUTTON_POWER;
    
    // Temperature (stored in upper 4 bits of byte 1)
    uint8_t temp = (uint8_t) roundf(clamp(this->target_temperature, HAIER_TEMP_MIN, HAIER_TEMP_MAX));
    remote_state_[1] |= ((temp - HAIER_TEMP_OFFSET) << 4);
    
    // Operating mode (bits 5-7 of byte 7)
    switch (this->mode) {
      case climate::CLIMATE_MODE_HEAT_COOL:
        remote_state_[7] |= (HAIER_MODE_AUTO << 5);
        break;
      case climate::CLIMATE_MODE_COOL:
        remote_state_[7] |= (HAIER_MODE_COOL << 5);
        break;
      case climate::CLIMATE_MODE_HEAT:
        remote_state_[7] |= (HAIER_MODE_HEAT << 5);
        break;
      case climate::CLIMATE_MODE_DRY:
        remote_state_[7] |= (HAIER_MODE_DRY << 5);
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        remote_state_[7] |= (HAIER_MODE_FAN << 5);
        break;
      default:
        remote_state_[7] |= (HAIER_MODE_AUTO << 5);
    }
    
    // Fan speed (bits 5-7 of byte 5)
    if (this->fan_mode.has_value()) {
      switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_LOW:
          remote_state_[5] |= (HAIER_FAN_LOW << 5);
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          remote_state_[5] |= (HAIER_FAN_MED << 5);
          break;
        case climate::CLIMATE_FAN_HIGH:
          remote_state_[5] |= (HAIER_FAN_HIGH << 5);
          break;
        case climate::CLIMATE_FAN_AUTO:
        default:
          remote_state_[5] |= (HAIER_FAN_AUTO << 5);
          break;
      }
    } else {
      remote_state_[5] |= (HAIER_FAN_AUTO << 5);
    }
    
    // Swing mode
    uint8_t swing_v = HAIER_SWING_OFF;
    uint8_t swing_h = 0;
    
    switch (this->swing_mode) {
      case climate::CLIMATE_SWING_VERTICAL:
        swing_v = HAIER_SWING_VERTICAL;
        remote_state_[12] = HAIER_BUTTON_SWING_V;
        break;
      case climate::CLIMATE_SWING_HORIZONTAL:
        swing_h = HAIER_SWING_HORIZONTAL;
        remote_state_[12] = HAIER_BUTTON_SWING_H;
        break;
      case climate::CLIMATE_SWING_BOTH:
        swing_v = HAIER_SWING_VERTICAL;
        swing_h = HAIER_SWING_HORIZONTAL;
        remote_state_[12] = HAIER_BUTTON_SWING_V;
        break;
      case climate::CLIMATE_SWING_OFF:
      default:
        break;
    }
    
    // Set swing values
    remote_state_[1] |= (swing_v & 0x0F);  // Lower 4 bits of byte 1
    remote_state_[2] |= (swing_h << 5);    // Bits 5-7 of byte 2
    
    // Health/Filter always on for now
    remote_state_[3] |= 0x02;
  } else {
    // Power off
    remote_state_[12] = HAIER_BUTTON_POWER;
  }
  
  // Calculate and set checksum
  remote_state_[13] = this->calculate_checksum_();
}

bool HaierClimate::decode_state_(remote_base::RemoteReceiveData data) {
  uint8_t state[13] = {0};
  
  // Decode the data
  for (uint8_t i = 0; i < 13; i++) {
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(HAIER_BIT_MARK, HAIER_ONE_SPACE)) {
        state[i] |= (1 << bit);
      } else if (data.expect_item(HAIER_BIT_MARK, HAIER_ZERO_SPACE)) {
        // Bit is already 0
      } else {
        return false;
      }
    }
  }
  
  // Verify model byte
  if (state[0] != 0xA6) {
    ESP_LOGD(TAG, "Invalid model byte: 0x%02X", state[0]);
    return false;
  }
  
  // Copy state
  memcpy(remote_state_, state, 13);
  
  // Decode power state
  if ((state[4] & 0x40) == 0) {
    this->mode = climate::CLIMATE_MODE_OFF;
  } else {
    // Decode mode
    uint8_t mode = (state[7] >> 5) & 0x07;
    switch (mode) {
      case HAIER_MODE_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      case HAIER_MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case HAIER_MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case HAIER_MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case HAIER_MODE_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      default:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
    }
    
    // Decode temperature
    uint8_t temp = (state[1] >> 4) & 0x0F;
    this->target_temperature = temp + HAIER_TEMP_OFFSET;
    
    // Decode fan mode
    uint8_t fan = (state[5] >> 5) & 0x07;
    switch (fan) {
      case HAIER_FAN_LOW:
        this->fan_mode = climate::CLIMATE_FAN_LOW;
        break;
      case HAIER_FAN_MED:
        this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
        break;
      case HAIER_FAN_HIGH:
        this->fan_mode = climate::CLIMATE_FAN_HIGH;
        break;
      case HAIER_FAN_AUTO:
      default:
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
        break;
    }
    
    // Decode swing mode
    uint8_t swing_v = state[1] & 0x0F;
    uint8_t swing_h = (state[2] >> 5) & 0x07;
    
    if (swing_v == HAIER_SWING_VERTICAL && swing_h == HAIER_SWING_HORIZONTAL) {
      this->swing_mode = climate::CLIMATE_SWING_BOTH;
    } else if (swing_v == HAIER_SWING_VERTICAL) {
      this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
    } else if (swing_h == HAIER_SWING_HORIZONTAL) {
      this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
    } else {
      this->swing_mode = climate::CLIMATE_SWING_OFF;
    }
  }
  
  this->publish_state();
  ESP_LOGD(TAG, "Decoded Haier state");
  return true;
}

uint8_t HaierClimate::calculate_checksum_() {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 13; i++) {
    sum += remote_state_[i];
  }
  return sum;
}

}  // namespace haier
}  // namespace esphome