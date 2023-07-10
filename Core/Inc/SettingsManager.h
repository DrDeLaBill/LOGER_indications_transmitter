#ifndef INC_SETTINGSMANAGER_H_
#define INC_SETTINGSMANAGER_H_


#include <stdint.h>

#include "main.h"

#include "internal_storage.h"


#define LOW_MODBUS_COUNT LOW_MB_SENS_COUNT + 1


class SettingsManager {

private:
	static const char* MODULE_TAG;

	static const char* SETTINGS_FILENAME;

public:
	typedef struct __attribute__((packed)) _payload_settings_t  {
		uint32_t sens_record_period;
		uint32_t sens_transmit_period;
		uint8_t low_sens_status[LOW_MB_ARR_SIZE];
		uint16_t low_sens_register[LOW_MB_ARR_SIZE];
	} payload_settings_t;

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

	typedef enum _settings_status_t {
		SETTINGS_OK = 0,
		SETTINGS_ERROR
	} settings_status_t;

	typedef enum _device_type_t {
		DEVICE_TUPE_UNKNOWN = 0x00,
		DEVICE_TYPE_LOGER,
		DEVICE_TYPE_MAIN
	} device_type_t;

	static settings_sd_payload_t sd_sttngs;

	SettingsManager();
	settings_status_t load();
	static settings_status_t reset();
	static settings_status_t save();

};


#endif
