#include "SensorManager.h"

#include "DeviceStateBase.h"
#include "SettingsManager.h"
extern "C"
{
	#include "modbus/mb.h"
	#include "modbus/mb-table.h"
}


#define MODBUS_CHECK(condition, time) if (!wait_event(condition, time)) {return;}

#define MODBUS_RESPONSE_TIME          100 //ms


uint8_t SensorManager::mb_data[];
uint8_t SensorManager::mb_length = 0;


SensorManager::SensorManager() {
	DeviceStateBase();
	this->current_action = &SensorManager::start_action;
	this->read_time = SettingsManager::sttngs->sens_read_period;
	this->clear_modbus_data();
    mb_set_tx_handler(this->modbus_data_handler);
}

void SensorManager::proccess() {
	(this->*current_action)();
}

void SensorManager::sleep() {
	this->current_action = &SensorManager::sleep_action;
}

void SensorManager::awake() {
	this->current_action = &SensorManager::sleep_action;
}

void SensorManager::start_action() {
	this->current_action = &SensorManager::record_action;
}

void SensorManager::wait_action() {
	if (Util_TimerPending(&this->read_timer)) {
		return;
	}
	this->current_action = &SensorManager::record_action;
}

void SensorManager::record_action() {
	this->write_sensors_data();
	this->clear_modbus_data();

	this->current_action = &SensorManager::wait_action;
	Util_TimerStart(&this->read_timer, this->read_time);
}

void SensorManager::register_action() {}

void SensorManager::sleep_action() {}

void SensorManager::write_sensors_data() {}

void SensorManager::modbus_data_handler(uint8_t * data, uint8_t len)
{
    if (!len || len > sizeof(SensorManager::mb_data)) {
        return;
    }
    SensorManager::clear_modbus_data();
    SensorManager::mb_length = len;
    for (uint8_t i = 0; i < len; i++) {
    	SensorManager::mb_data[i] = data[i];
    }
}

void SensorManager::clear_modbus_data()
{
    for (uint8_t i = 0; i < sizeof(SensorManager::mb_data); i++) {
    	SensorManager::mb_data[i] = 0;
    }
    SensorManager::mb_length = 0;
//    SensorManager::start_time = HAL_GetTick();
}
