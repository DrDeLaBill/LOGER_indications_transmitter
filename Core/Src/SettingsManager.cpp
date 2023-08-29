#include "SettingsManager.h"

#include <string.h>

#include "main.h"
#include "utils.h"
#include "SensorManager.h"
#include "storage_data_manager.h"


typedef SettingsManager SM;


const char* SM::MODULE_TAG = "STNG";

SM::settings_t SM::sttngs  = { 0 };


SM::SettingsManager() {
	if (SM::load() != SETTINGS_OK) {
		SM::reset();
	}
}

SM::settings_status_t SM::reset() {
	SM::sttngs.config_struct_version = SETTINGS_CONFIG_STRUCT_VERSION;
	SM::sttngs.sens_record_period    = SENS_READ_PERIOD_MS;
	SM::sttngs.sens_transmit_period  = DATA_TRNS_PERIOD_MS;
	for (uint8_t i = 0; i < SensorManager::RESERVED_IDS_COUNT; i++) {
		SM::sttngs.low_sens_status[i]       = SensorManager::SENSOR_RESERVED;
		SM::sttngs.low_sens_val_register[i] = 0;
		SM::sttngs.low_sens_id_register[i]  = 0;
	}
	for (uint8_t i = SensorManager::RESERVED_IDS_COUNT; i < sizeof(SM::sttngs.low_sens_status); i++) {
		SM::sttngs.low_sens_status[i]       = SensorManager::SENSOR_FREE;
		SM::sttngs.low_sens_val_register[i] = 0;
		SM::sttngs.low_sens_id_register[i]  = 0;
	}
	// TODO: реалитзовать проверку наличия всех датчиков со 2 по 127
	// SensorManager::check_sensors();

	return SM::save();
}

SM::settings_status_t SM::load() {
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SM::MODULE_TAG, "load settings: begin");
#endif

	uint32_t sttngs_addr    = 0;
	storage_status_t status = storage_get_first_available_addr(&sttngs_addr);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SM::MODULE_TAG, "load settings: get settings address=%lu error=%02x", sttngs_addr, status);
#endif
		return SETTINGS_ERROR;
	}

	status = storage_load(sttngs_addr, (uint8_t*)&SM::sttngs, sizeof((uint8_t*)&SM::sttngs));
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(SM::MODULE_TAG, "load settings: load settings error=%02x", status);
#endif
		return SETTINGS_ERROR;
	}

	// TODO: добавлено для проверки, убрать!
	SM::sttngs.low_sens_status[1]       = SensorManager::SENSOR_TYPE_THERMAL;
	SM::sttngs.low_sens_val_register[1] = 0x0000;
	SM::sttngs.low_sens_id_register[1]  = 0x0000;


#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SM::MODULE_TAG, "load settings: OK");
#endif

	return SETTINGS_OK;
}

SM::settings_status_t SM::save() {
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SM::MODULE_TAG, "save settings: begin");
#endif

	uint32_t sttngs_addr    = 0;
	storage_status_t status = storage_get_first_available_addr(&sttngs_addr);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SM::MODULE_TAG, "save settings: get settings address=%lu error=%02x", sttngs_addr, status);
#endif
		return SETTINGS_ERROR;
	}

	status = storage_save(sttngs_addr, (uint8_t*)&SM::sttngs, sizeof((uint8_t*)&SM::sttngs));
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(SM::MODULE_TAG, "save settings: load settings error=%02x", status);
#endif
		return SETTINGS_ERROR;
	}

#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SM::MODULE_TAG, "save settings: OK");
#endif

	return SETTINGS_OK;
}
