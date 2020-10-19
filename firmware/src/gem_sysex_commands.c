#include "gem_sysex_commands.h"
#include "gem_adc.h"
#include "gem_config.h"
#include "gem_led_animation.h"
#include "gem_mcp4728.h"
#include "gem_midi_core.h"
#include "gem_pulseout.h"
#include "gem_settings.h"
#include "gem_usb.h"
#include "printf.h"
#include <string.h>

/* Static variables. */

static uint8_t _settings_buf[64];
static uint8_t _encoding_buf[128];

/* Forward declarations. */

void _cmd_0x01_hello(uint8_t* data, size_t len);
void _cmd_0x02_write_adc_gain(uint8_t* data, size_t len);
void _cmd_0x03_write_adc_offset(uint8_t* data, size_t len);
void _cmd_0x04_read_adc(uint8_t* data, size_t len);
void _cmd_0x05_set_dac(uint8_t* data, size_t len);
void _cmd_0x06_set_period(uint8_t* data, size_t len);
void _cmd_0x07_erase_settings(uint8_t* data, size_t len);
void _cmd_0x08_read_settings(uint8_t* data, size_t len);
void _cmd_0x09_write_settings(uint8_t* data, size_t len);

/* Public functions. */

void gem_register_sysex_commands() {
    gem_midi_register_sysex_command(0x01, _cmd_0x01_hello);
    gem_midi_register_sysex_command(0x02, _cmd_0x02_write_adc_gain);
    gem_midi_register_sysex_command(0x03, _cmd_0x03_write_adc_offset);
    gem_midi_register_sysex_command(0x04, _cmd_0x04_read_adc);
    gem_midi_register_sysex_command(0x05, _cmd_0x05_set_dac);
    gem_midi_register_sysex_command(0x06, _cmd_0x06_set_period);
    gem_midi_register_sysex_command(0x07, _cmd_0x07_erase_settings);
    gem_midi_register_sysex_command(0x08, _cmd_0x08_read_settings);
    gem_midi_register_sysex_command(0x09, _cmd_0x09_write_settings);
};

void _cmd_0x01_hello(uint8_t* data, size_t len) {
    (void)(data);
    (void)(len);

    gem_adc_stop_scanning();
    gem_led_animation_set_mode(GEM_LED_MODE_CALIBRATION);

    gem_usb_midi_send((uint8_t[4]){MIDI_SYSEX_START_OR_CONTINUE, MIDI_SYSEX_START_BYTE, GEM_MIDI_SYSEX_MARKER, 0x01});
    gem_usb_midi_send((uint8_t[4]){MIDI_SYSEX_END_TWO_BYTE, GEM_FIRMWARE_VERSION, MIDI_SYSEX_END_BYTE, 0x00});
}

void _cmd_0x02_write_adc_gain(uint8_t* data, size_t len) {
    (void)(len);

    struct gem_settings settings;
    gem_settings_load(&settings);
    settings.adc_gain_corr = data[2] << 12 | data[3] << 8 | data[4] << 4 | data[5];
    gem_settings_save(&settings);
}

void _cmd_0x03_write_adc_offset(uint8_t* data, size_t len) {
    (void)(len);

    struct gem_settings settings;
    gem_settings_load(&settings);
    settings.adc_offset_corr = data[2] << 12 | data[3] << 8 | data[4] << 4 | data[5];
    gem_settings_save(&settings);
}

void _cmd_0x04_read_adc(uint8_t* data, size_t len) {
    (void)(len);

    uint16_t result = gem_adc_read_sync(&gem_adc_inputs[data[2]]);
    gem_usb_midi_send((uint8_t[4]){MIDI_SYSEX_START_OR_CONTINUE, MIDI_SYSEX_START_BYTE, GEM_MIDI_SYSEX_MARKER, 0x02});
    gem_usb_midi_send(
        (uint8_t[4]){MIDI_SYSEX_START_OR_CONTINUE, (result >> 12) & 0xF, (result >> 8) & 0xF, (result >> 4) & 0xF});
    gem_usb_midi_send((uint8_t[4]){MIDI_SYSEX_END_TWO_BYTE, result & 0xF, MIDI_SYSEX_END_BYTE, 0x00});
}

void _cmd_0x05_set_dac(uint8_t* data, size_t len) {
    (void)(len);

    struct gem_mcp4728_channel dac_settings = {};
    dac_settings.vref = data[3];
    dac_settings.value = data[4] << 12 | data[5] << 8 | data[6] << 4 | data[7];
    gem_mcp_4728_write_channel(data[2], dac_settings);
}

void _cmd_0x06_set_period(uint8_t* data, size_t len) {
    (void)(len);
    gem_pulseout_set_period(data[2], data[3] << 12 | data[4] << 8 | data[5] << 4 | data[6]);
}

void _cmd_0x07_erase_settings(uint8_t* data, size_t len) {
    (void)(data);
    (void)(len);

    gem_settings_erase();
}

void _cmd_0x08_read_settings(uint8_t* data, size_t len) {
    (void)(data);
    (void)(len);

    struct gem_settings settings;
    gem_settings_serialize(&settings, _settings_buf);
    gem_midi_encode(_settings_buf, _encoding_buf, 64);
    gem_usb_midi_send((uint8_t[4]){MIDI_SYSEX_START_OR_CONTINUE, MIDI_SYSEX_START_BYTE, GEM_MIDI_SYSEX_MARKER, 0x06});
    /* Settings are sent in 16 byte chunks to avoid overflowing midi buffers. */
    if (data[2] < 8) {
        gem_midi_send_sysex(_encoding_buf + (16 * data[2]), 16);
    }
}

void _cmd_0x09_write_settings(uint8_t* data, size_t len) {
    (void)(len);

    /* Settings are sent in 16 byte chunks to avoid overflowing midi buffers. */
    if (data[2] == 0) {
        memset(_encoding_buf, 0xFF, 128);
    }
    if (data[2] < 8) {
        memcpy(_encoding_buf + (16 * data[2]), data + 3, 16);
        /* Ack the data. */
        gem_usb_midi_send(
            (uint8_t[4]){MIDI_SYSEX_START_OR_CONTINUE, MIDI_SYSEX_START_BYTE, GEM_MIDI_SYSEX_MARKER, 0x07});
        gem_usb_midi_send((uint8_t[4]){MIDI_SYSEX_END_ONE_BYTE, MIDI_SYSEX_END_BYTE, 0x00, 0x00});
    }
    if (data[2] == 7) {
        /* All data recieved, save the settings. */
        struct gem_settings settings;
        gem_midi_decode(_encoding_buf, _settings_buf, 64);
        if (gem_settings_deserialize(&settings, _settings_buf)) {
            gem_settings_save(&settings);
        }
    }
}