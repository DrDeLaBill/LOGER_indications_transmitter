#ifndef INC_SETTINGSMANAGER_H_
#define INC_SETTINGSMANAGER_H_


#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "storage_data_manager.h"


#define SETTINGS_BEDUG                 (true)

#define SETTINGS_CONFIG_STRUCT_VERSION ((uint8_t)1)
#define SETTINGS_LOW_MODBUS_COUNT      (LOW_MB_SENS_COUNT + 1)


class SettingsManager {

private:
	static const char* MODULE_TAG;

public:
	typedef struct __attribute__((packed)) _settings_t  {
		uint8_t  config_struct_version;
		uint32_t sens_record_period;
		uint32_t sens_transmit_period;
		uint8_t  low_sens_status[LOW_MB_ARR_SIZE];
		uint16_t low_sens_val_register[LOW_MB_ARR_SIZE];
		uint16_t low_sens_id_register[LOW_MB_ARR_SIZE];
	} settings_t;

	typedef enum _settings_status_t {
		SETTINGS_OK = 0,
		SETTINGS_ERROR
	} settings_status_t;

	typedef enum _device_type_t {
		DEVICE_TUPE_UNKNOWN = 0x00,
		DEVICE_TYPE_LOGER,
		DEVICE_TYPE_MAIN
	} device_type_t;

	static settings_t sttngs;

	SettingsManager();
	static settings_status_t load();
	static settings_status_t reset();
	static settings_status_t save();
};


#endif
