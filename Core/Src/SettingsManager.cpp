#include "SettingsManager.h"

#include <string.h>
#include <fatfs.h>

#include "main.h"
#include "SensorManager.h"
#include "internal_storage.h"
#include "utils.h"


typedef SettingsManager SM;


const char* SM::MODULE_TAG = "STTNGS";
const char* SM::SETTINGS_FILENAME = "settings.bin";
SM::payload_settings_t* SM::sttngs = NULL;
SM::settings_sd_payload_t SM::sd_sttngs = {0};


SM::SettingsManager() {
	if (this->load() != SETTINGS_OK) {
		this->reset();
	}
	SM::sttngs = &(SM::sd_sttngs.v1.payload_settings);
}

SM::settings_status_t SM::reset() {
	SM::sd_sttngs.v1.payload_settings.sens_record_period = SENS_READ_PERIOD_MS;
	SM::sd_sttngs.v1.payload_settings.sens_transmit_period = DATA_TRNS_PERIOD_MS;
	for (uint8_t i = 0x00; i < SensorManager::RESERVED_IDS_COUNT; i++) {
		SM::sd_sttngs.v1.payload_settings.low_sens_status[i] = SensorManager::SENSOR_RESERVED;
		SM::sd_sttngs.v1.payload_settings.low_sens_register[i] = 0x0000;
	}
	for (uint8_t i = SensorManager::RESERVED_IDS_COUNT; i < sizeof(SM::sd_sttngs.v1.payload_settings.low_sens_status); i++) {
		SM::sd_sttngs.v1.payload_settings.low_sens_status[i] = SensorManager::SENSOR_FREE;
		SM::sd_sttngs.v1.payload_settings.low_sens_register[i] = 0x0000;
	}
	// TODO: реалитзовать проверку наличия всех датчиков со 2 по 127
	// SensorManager::check_sensors();

	return SettingsManager::save();
}

SM::settings_status_t SM::load() {
	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, SETTINGS_FILENAME);

	UINT br;
	bool settings_load_ok = true;
	FRESULT res = intstor_read_file(filename, &(SM::sd_sttngs), sizeof(SM::sd_sttngs), &br);
	if(res != FR_OK) {
		settings_load_ok = false;
		LOG_DEBUG(MODULE_TAG, "read_file(%s) error=%i\n", filename, res);
	}

	if(SM::sd_sttngs.header.magic != STORAGE_SD_PAYLOAD_MAGIC) {
		settings_load_ok = false;
		LOG_DEBUG(MODULE_TAG, "bad settings magic %08lX!=%08lX\n", SM::sd_sttngs.header.magic, STORAGE_SD_PAYLOAD_MAGIC);
	}

	if(SM::sd_sttngs.header.version != STORAGE_SD_PAYLOAD_VERSION) {
		settings_load_ok = false;
		LOG_DEBUG(MODULE_TAG, "bad settings version %i!=%i\n", SM::sd_sttngs.header.version, STORAGE_SD_PAYLOAD_VERSION);
	}

	if(!settings_load_ok) {
		LOG_DEBUG(MODULE_TAG, "settings not loaded, using defaults\n");
		this->reset();
	}

	LOG_DEBUG(MODULE_TAG, "applying settings\n");

	if(!settings_load_ok) {
		return this->save();
	}

	// TODO: добавлено для проверки, убрать!
	SM::sd_sttngs.v1.payload_settings.low_sens_status[1] = SensorManager::SENSOR_TYPE_THERMAL;
	SM::sd_sttngs.v1.payload_settings.low_sens_register[1] = 0x0000;

	return SETTINGS_OK;
}

SM::settings_status_t SM::save() {
	SM::sd_sttngs.header.magic = STORAGE_SD_PAYLOAD_MAGIC;
	SM::sd_sttngs.header.version = STORAGE_SD_PAYLOAD_VERSION;

	WORD crc = 0;
	for(uint16_t i = 0; i < sizeof(SM::sd_sttngs.bits); i++)
		DIO_SPI_CardCRC16(&crc, SM::sd_sttngs.bits[i]);
	SM::sd_sttngs.crc = crc;

	LOG_DEBUG(SM::MODULE_TAG, "saving settings\n");
	Debug_HexDump(MODULE_TAG, (uint8_t*)&(SM::sd_sttngs), sizeof(SM::sd_sttngs));

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, SETTINGS_FILENAME);

	UINT br;
	FRESULT res = intstor_write_file(filename, &(SM::sd_sttngs), sizeof(SM::sd_sttngs), &br);
	if(res != FR_OK) {
		LOG_DEBUG(MODULE_TAG, "settings NOT saved\n");
		return SETTINGS_ERROR;
	}

	LOG_DEBUG(MODULE_TAG, "settings saved\n");
	return SETTINGS_OK;
}
