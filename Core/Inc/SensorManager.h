#ifndef INC_SENSORMANAGER_H_
#define INC_SENSORMANAGER_H_


#include "DeviceStateBase.h"
#include "utils.h"
#include "modbus/mb-table.h"


#define MODBUS_DATA_OFFSET 16


class SensorManager: public DeviceStateBase {

private:
	// Read delay timer
	uint32_t read_time;
	dio_timer_t read_timer;
	// MODBUS data
    static uint8_t data[TABLE_Holding_Registers_Size + MODBUS_DATA_OFFSET];
    static uint8_t length;

    static void clear_modbus_data();
    static void modbus_data_handler(uint8_t * data, uint8_t len);

    // Sensors state actions
    void start_action();
    void wait_action();
    void record_action();
    void register_action();
    void sleep_action();

    void write_sensors_data();

protected:
	void (SensorManager::*current_action) (void);

public:
	SensorManager(uint32_t read_time);
	void proccess();
	void sleep();
	void awake();

};


#endif
