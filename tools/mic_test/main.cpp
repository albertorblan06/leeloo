// Temporary INMP441 bring-up: prints the peak level of each 100 ms block.
// Wiring: SCK GPIO15, WS GPIO16, SD GPIO17, L/R GND. See docs/hardware-checklists/mic-inmp441.md.
#include <Arduino.h>
#include <driver/i2s.h>

namespace {
constexpr i2s_port_t kPort = I2S_NUM_0;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kBlock = kSampleRate / 10;  // 100 ms
bool ready = false;
int32_t samples[kBlock];
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);  // native USB: give the monitor time to attach

  i2s_config_t cfg = {};
  cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = kSampleRate;
  // INMP441 sends 24 bits in a 32-bit slot, left channel when L/R = GND.
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = 15;
  pins.ws_io_num = 16;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = 17;
  if (i2s_driver_install(kPort, &cfg, 0, nullptr) != ESP_OK || i2s_set_pin(kPort, &pins) != ESP_OK) {
    Serial.println("FAIL: i2s install/set_pin");
    return;
  }
  ready = true;
  Serial.println("mic_test ready: speak or clap; peak should jump above the silence level.");
}

void loop() {
  if (!ready) { delay(1000); return; }
  size_t bytes = 0;
  if (i2s_read(kPort, samples, sizeof(samples), &bytes, 1000 / portTICK_PERIOD_MS) != ESP_OK || bytes == 0) {
    Serial.println("FAIL: no data from mic (check SD/WS/SCK wiring)");
    return;
  }
  int32_t peak = 0;
  for (size_t i = 0; i < bytes / sizeof(int32_t); ++i) {
    int32_t v = samples[i] >> 8;  // 24-bit signed value
    if (v < 0) v = -v;
    if (v > peak) peak = v;
  }
  // Bar scaled to 24-bit full range (~8.4M); silence is typically a few thousand.
  int bar = peak > 0 ? constrain((int)(log10f((float)peak) * 8), 0, 60) : 0;
  Serial.printf("peak %8ld |", (long)peak);
  for (int i = 0; i < bar; ++i) Serial.print('#');
  Serial.println();
}
