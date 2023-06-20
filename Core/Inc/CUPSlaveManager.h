/*
 *
 * CUPSlaveManager
 * Custom UART Protocol manager - slave
 * Created for connection PC (or Main Device) with indictions_transmitter via UART
 *
 */
#ifndef INC_CUPSLAVEMANAGER_H_
#define INC_CUPSLAVEMANAGER_H_


#include <stdint.h>

extern "C"
{
	#include "utils.h"
}


class CUPSlaveManager {

private:
	static const char* MODULE_TAG;

	typedef enum _CUP_command_typedef {
		CUP_CMD_STATUS = 0x01,
		CUP_CMD_STTNGS,
		CUP_CMD_SENSRS,
	} CUP_command;


	const uint8_t CUP_WAIT_TIMEOUT = 100;


	uint8_t data_counter = 0;
	uint8_t* settings_data;
	uint8_t* sensors_data;

	void (CUPSlaveManager::*curr_status_action) (uint8_t msg);
	void status_wait(uint8_t msg);
	void status_status(uint8_t msg);
	void status_data_len(uint8_t msg);
	void status_data(uint8_t msg);
	void status_check_crc(uint8_t msg);

	void send_response();
	void send_error();
	void save_request_data();
	void send_byte(uint8_t msg);
	uint8_t get_CRC8(uint8_t* buffer, uint8_t size);

public:
	typedef struct _CUP_message {
		CUP_command command;
		uint8_t data_len;
		uint8_t* data;
		uint8_t crc8;
	} CUP_message;

	CUP_message request;
	CUP_message response;

	CUPSlaveManager();

	void char_data_handler(uint8_t msg);
	void reset_data();

	void set_settings_data(uint8_t* data);
	void set_sensors_data(uint8_t* data);

	// Update sensors data function, implemented outside the class
	void update_sensors_handler(void);
	// Update settings data function, implemented outside the class
	void update_settings_handler(void);

};


#endif
