#ifndef INC_SENSORMANAGER_H_
#define INC_SENSORMANAGER_H_


#include <stdint.h>

#include "DeviceStateBase.h"

extern "C"
{
	#include "utils.h"
	#include "internal_storage.h"
	#include "modbus/mb-table.h"
}


#define MODBUS_DATA_OFFSET 16


class SensorManager: public DeviceStateBase {

public:
	static const uint8_t RESERVED_IDS_COUNT = 2;

	typedef enum _sensor_id_status_t {
		SENSOR_FREE = 0,
		SENSOR_RESERVED,
		SENSOR_ERROR,
		SENSOR_TYPE_THERMAL,
		SENSOR_TYPE_HUMIDITY,
		SENSOR_TYPE_OTHER
	} sensor_id_status_t;

	SensorManager();
	void proccess();
	void sleep();
	void awake();

	uint8_t* get_sensors_data();

private:
	static const char* MODULE_TAG;

    // Modbus
	static uint8_t current_slave_id;
	uint32_t read_time;
	dio_timer_t wait_timer;
	uint8_t error_count;

    static void send_request(uint8_t* data,uint8_t Len);
    static void master_process(mb_packet_s* packet);
    void update_modbus_slave_id();
    void registrate_modbus_error();
    void write_sensors_data();

    // Sensors state actions
    void start_action();
    void wait_action();
    void request_action();
    void wait_response_action();
    void success_response_action();
    void error_response_action();
    void register_action();
    void sleep_action();

protected:
    typedef enum _sensor_read_status_typedef {
    	SENS_WAIT = 0,
		SENS_SUCCESS,
		SENS_ERROR
    } sensor_read_status;

	void (SensorManager::*current_action) (void);
	static sensor_read_status sens_status;

};


#endif
