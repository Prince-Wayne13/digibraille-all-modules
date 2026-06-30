// ============================================================
//  SAM TTS -> I2S amplifier output
// ============================================================
#include "sam_tts.h"
#include "globals.h"
#include "AudioTools.h"
#include "sam_arduino.h"
#include "esp_log.h"
#include "driver/i2s.h"

static const i2s_port_t SAM_I2S_PORT = I2S_NUM_0;

class DirectI2S : public AudioOutput {
public:
  bool begin(AudioInfo info) override {
    _info = info;
    configureI2S();
    setAudioInfo(info);
    return true;
  }

  size_t write(const uint8_t* buf, size_t size) override {
    for (size_t i = 0; i + 1 < size; i += 2) {
      int16_t sample = (int16_t)(buf[i] | (buf[i + 1] << 8));
      int16_t stereo[2] = {sample, sample};
      size_t written = 0;
      i2s_write(SAM_I2S_PORT, stereo, sizeof(stereo), &written, portMAX_DELAY);
    }
    return size;
  }

private:
  void configureI2S() {
    i2s_driver_uninstall(SAM_I2S_PORT);

    i2s_config_t config = {};
    config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = _info.sample_rate > 0 ? _info.sample_rate : 22050;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = 8;
    config.dma_buf_len = 128;
    config.use_apll = false;
    config.tx_desc_auto_clear = true;
    config.fixed_mclk = 0;

    i2s_pin_config_t pins = {};
    pins.bck_io_num = AMP_BCLK_PIN;
    pins.ws_io_num = AMP_LRC_PIN;
    pins.data_out_num = AMP_DIN_PIN;
    pins.data_in_num = I2S_PIN_NO_CHANGE;

    i2s_driver_install(SAM_I2S_PORT, &config, 0, nullptr);
    i2s_set_pin(SAM_I2S_PORT, &pins);
    i2s_zero_dma_buffer(SAM_I2S_PORT);
  }

  AudioInfo _info;
};

static DirectI2S _samOut;
static SAM       _sam(_samOut);

void samSetup() {
  esp_log_level_set("i2s", ESP_LOG_NONE);

  _sam.setVoice(SAM::Elf);
  _sam.setPitch(64);
  _sam.setThroat(128);
  _sam.setSpeed(48);
  _sam.setMouth(150);

  AudioInfo info;
  info.sample_rate     = 22050;
  info.channels        = 1;
  info.bits_per_sample = 16;
  _samOut.begin(info);
  logTs("AUDIO", "SAM I2S configured");
}

void samSay(const char* text) {
  String s = String(text);
  char last = s.length() > 0 ? s.charAt(s.length() - 1) : 0;
  if (last != '.' && last != '!' && last != '?') s += '.';
  logTsValue("AUDIO", "SAM START ", s);
  logTestEvent(3, "audio-start", String("SAM ") + s);
  unsigned long t0 = millis();
  _sam.say(s.c_str());
  i2s_zero_dma_buffer(SAM_I2S_PORT);
  Serial.print('['); Serial.print(millis()); Serial.print(F(" ms] AUDIO SAM DONE duration="));
  Serial.print(millis() - t0); Serial.println(F("ms"));
  logTestEvent(3, "audio-complete", String("duration_ms=") + String(millis() - t0));
}

void samSayString(String t) { samSay(t.c_str()); }

void samSayChunked(String text) {
  text.trim();
  while (text.length() > 0) {
    String chunk = ""; int words = 0, pos = 0;
    while (pos < (int)text.length() && words < 10) {
      int sp = text.indexOf(' ', pos);
      if (sp < 0) { chunk += text.substring(pos); pos = text.length(); words++; }
      else        { chunk += text.substring(pos, sp + 1); pos = sp + 1; words++; }
    }
    chunk.trim();
    if (chunk.length() > 0) samSayString(chunk);
    text = (pos < (int)text.length()) ? text.substring(pos) : "";
    text.trim();
  }
}

void samSayChunkedInterruptible(String text) {
  text.trim();
  while (text.length() > 0) {
    if (buttonPressed(BTN_SELECT) || buttonPressed(BTN_BACK) ||
        buttonPressed(BTN_DOWN)   || buttonPressed(BTN_REREAD) ||
        buttonPressed(BTN_AISAVE) || buttonPressed(BTN_DELETE)) {
      logTs("BUTTON", "SAM interrupted by button");
      i2s_zero_dma_buffer(SAM_I2S_PORT);
      return;
    }
    String chunk = ""; int words = 0, pos = 0;
    while (pos < (int)text.length() && words < 8) {
      int sp = text.indexOf(' ', pos);
      if (sp < 0) { chunk += text.substring(pos); pos = text.length(); words++; }
      else        { chunk += text.substring(pos, sp + 1); pos = sp + 1; words++; }
    }
    chunk.trim();
    if (chunk.length() > 0) samSayString(chunk);
    text = (pos < (int)text.length()) ? text.substring(pos) : "";
    text.trim();
  }
}
