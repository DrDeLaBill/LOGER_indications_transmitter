#ifndef INC_SETTINGSMANAGER_H_
#define INC_SETTINGSMANAGER_H_


#include <stdint.h>

#include "main.h"

extern "C"
{
	#include "internal_storage.h"
}


class SettingsManager {

public:
	typedef struct __attribute__((packed)) _payload_settings_t  {
		uint32_t sens_read_period;
		uint32_t sens_transmit_period;
		uint8_t low_sens_status[LOW_MB_SENS_COUNT+1];
	} payload_settings_t;

private:
	typedef union _settings_sd_payload_t {
		struct __attribute__((packed)) {
			struct _sd_payload_header_t header;
			uint8_t bits[SD_PAYLOAD_BITS_SIZE(STORAGE_SD_MAX_PAYLOAD_SIZE)];
			uint16_t crc;
		};
		struct __attribute__((packed)) {
			struct _sd_payload_header_t header;
			payload_settings_t payload_settings;
		} v1;
	} settings_sd_payload_t;


	static const char* MODULE_TAG;

	static const char* SETTINGS_FILENAME;

	static settings_sd_payload_t sd_sttngs;

	static void update_public_settings();

public:
	typedef enum _settings_status_t {
		SETTINGS_OK = 0,
		SETTINGS_ERROR
	} settings_status_t;

	typedef enum _device_type_t {
		DEVICE_TUPE_UNKNOWN = 0x00,
		DEVICE_TYPE_LOGER,
		DEVICE_TYPE_MAIN
	} device_type_t;

	struct device_info_t {
		uint8_t device_type;
		uint8_t device_version;
		uint16_t id_base1;
		uint16_t id_base2;
		uint32_t id_base3;
		uint32_t id_base4;
	} device_info;

	static payload_settings_t* sttngs;

	SettingsManager();
	settings_status_t load();
	settings_status_t reset();
	static settings_status_t save();

};


#endif
