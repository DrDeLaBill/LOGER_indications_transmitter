#include "SettingsManager.h"

#include <string.h>
#include <fatfs.h>

#include "main.h"
#include "SensorManager.h"
extern "C"
{
	#include "internal_storage.h"
	#include "utils.h"
}


typedef SettingsManager SM;


SM::payload_settings_t* SM::sttngs;


SM::SettingsManager() {
	if (this->load() == SETTINGS_OK) {
		return;
	}
	this->reset();
}

SM::settings_status_t SM::reset() {
	this->sd_sttngs.v1.payload_settings.sens_read_period = SENS_READ_PERIOD_MS;
	this->sd_sttngs.v1.payload_settings.sens_transmit_period = DATA_TRNS_PERIOD_MS;
	for (uint8_t i = 0x00; i < SensorManager::RESERVED_IDS_COUNT; i++) {
		this->sd_sttngs.v1.payload_settings.low_sens_status[i] = SensorManager::SENSOR_ID_RESERVED_S;
	}
	for (uint8_t i = SensorManager::RESERVED_IDS_COUNT; i < sizeof(this->sd_sttngs.v1.payload_settings.low_sens_status); i++) {
		this->sd_sttngs.v1.payload_settings.low_sens_status[i] = SensorManager::SENSOR_ID_FREE_S;
	}
	// TODO: реалитзовать проверку наличия всех датчиков с 1 по 127
	// SensorManager::check_sensors();
	return this->save();
}

SM::settings_status_t SM::load() {
	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, SETTINGS_FILENAME);

	UINT br;
	bool settings_load_ok = true;
	FRESULT res = intstor_read_file(filename, &(this->sd_sttngs), sizeof(this->sd_sttngs), &br);
	if(res != FR_OK) {
		settings_load_ok = false;
		LOG_DEBUG(MODULE_TAG, "read_file(%s) error=%i\n", filename, res);
	}

	if(this->sd_sttngs.header.magic != SETTINGS_SD_PAYLOAD_MAGIC) {
		settings_load_ok = false;
		LOG_DEBUG(MODULE_TAG, "bad settings magic %08lX!=%08lX\n", this->sd_sttngs.header.magic, SETTINGS_SD_PAYLOAD_MAGIC);
	}

	if(this->sd_sttngs.header.version != SETTINGS_SD_PAYLOAD_VERSION) {
		settings_load_ok = false;
		LOG_DEBUG(MODULE_TAG, "bad settings version %i!=%i\n", this->sd_sttngs.header.version, SETTINGS_SD_PAYLOAD_VERSION);
	}

	if(!settings_load_ok) {
		LOG_DEBUG(MODULE_TAG, "settings not loaded, using defaults\n");
		this->reset();
	}

	LOG_DEBUG(MODULE_TAG, "applying settings\n");

	if(!settings_load_ok) {
		return this->save();
	}

	this->update_public_settings();
	return SETTINGS_OK;
}

SM::settings_status_t SM::save() {
	this->sd_sttngs.header.magic = SETTINGS_SD_PAYLOAD_MAGIC;
	this->sd_sttngs.header.version = SETTINGS_SD_PAYLOAD_VERSION;

	WORD crc = 0;
	for(uint16_t i = 0; i < sizeof(this->sd_sttngs.bits); i++)
		DIO_SPI_CardCRC16(&crc, this->sd_sttngs.bits[i]);
	this->sd_sttngs.crc = crc;

	LOG_DEBUG(MODULE_TAG, "saving settings\n");
	Debug_HexDump(MODULE_TAG, (uint8_t*)&(this->sd_sttngs), sizeof(this->sd_sttngs));

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, SETTINGS_FILENAME);

	UINT br;
	FRESULT res = intstor_write_file(filename, &(this->sd_sttngs), sizeof(this->sd_sttngs), &br);
	if(res != FR_OK) {
		LOG_DEBUG(MODULE_TAG, "settings NOT saved\n");
		return SETTINGS_ERROR;
	}

	LOG_DEBUG(MODULE_TAG, "settings saved\n");
	this->update_public_settings();
	return SETTINGS_OK;
}

void SM::update_public_settings() {
	SM::sttngs = &(this->sd_sttngs.v1.payload_settings);
}
