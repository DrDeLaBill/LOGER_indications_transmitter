#ifndef INC_SETTINGSMANAGER_H_
#define INC_SETTINGSMANAGER_H_


#include <stdint.h>

#include "main.h"

extern "C"
{
	#include "internal_storage.h"
}


#define SETTINGS_SD_PAYLOAD_MAGIC   ((uint32_t)(0xBADAC0DE))
#define SETTINGS_SD_PAYLOAD_VERSION (1)


#define SETTINGS_SD_MAX_PAYLOAD_SIZE 512


class SettingsManager {

private:
	typedef struct _payload_settings_t {
		uint32_t sens_read_period;
		uint32_t sens_transmit_period;
		uint8_t low_sens_status[LOW_MB_SENS_COUNT+1];
	} payload_settings_t;

	typedef union _settings_sd_payload_t {
		struct __attribute__((packed)) {
			struct _sd_payload_header_t header;
			uint8_t bits[SD_PAYLOAD_BITS_SIZE(SETTINGS_SD_MAX_PAYLOAD_SIZE)];
			uint16_t crc;
		};
		struct __attribute__((packed)) {
			struct _sd_payload_header_t header;
			payload_settings_t payload_settings;
		} v1;
	} settings_sd_payload_t;


	const char* MODULE_TAG = "STTNGS";

	const char* SETTINGS_FILENAME = "settings.bin";

	settings_sd_payload_t sd_sttngs;

	void update_public_settings();

public:
	typedef enum _settings_status_t {
		SETTINGS_OK = 0,
		SETTINGS_ERROR
	} settings_status_t;

	static payload_settings_t* sttngs;

	SettingsManager();
	settings_status_t load();
	settings_status_t save();
	settings_status_t reset();

};


#endif
