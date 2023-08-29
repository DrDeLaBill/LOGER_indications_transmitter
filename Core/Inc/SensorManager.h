#ifndef INC_SENSORMANAGER_H_
#define INC_SENSORMANAGER_H_


#include <stdint.h>

#include "DeviceStateBase.h"
#include "RecordManager.h"
#include "utils.h"
#include "storage_data_manager.h"
#include "modbus_rtu_master.h"


#define MODBUS_DATA_OFFSET 16
#define MODBUS_SIZE_MAX    10


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

private:
	static const char* MODULE_TAG;

    // Modbus
	static uint8_t current_slave_id;
	dio_timer_t wait_record_timer;
	dio_timer_t wait_modbus_timer;
	uint8_t error_count;

	static uint8_t mb_send_data_buffer[MODBUS_SIZE_MAX];
	static uint8_t mb_send_data_length;

    static void send_request(uint8_t* data,uint8_t Len);
    static void response_packet_handler(modbus_response_t* packet);

    void update_modbus_slave_id();
    void registrate_modbus_error();
    bool write_sensors_data();

    // Sensors state actions
    void start_action();
    void wait_record_action();
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
