/*
 *
 * CUPSlaveManager
 * Custom UART Protocol manager - slave
 * Created for connection PC (or Main Device) with indictions_transmitter via UART or another interface
 *
 */
#ifndef INC_CUPSLAVEMANAGER_H_
#define INC_CUPSLAVEMANAGER_H_


#include <stdint.h>

#include "utils.h"


class CUPSlaveManager {

private:
	static const char* MODULE_TAG;

	typedef enum _CUP_command_typedef {
		CUP_CMD_DEVICE = 0x01,
		CUP_CMD_STTNGS,
		CUP_CMD_DATA,
	} CUP_command;

	typedef enum _CUP_error_typedef {
		CUP_ERROR_COMMAND = 0x01,
		CUP_ERROR_DATA_LEN,
		CUP_ERROR_DATA_OVERLOAD,
		CUP_ERROR_DATA_DOES_NOT_EXIST,
		CUP_ERROR_CRC,
		CUP_ERROR_TIMEOUT
	} CUP_error;

public:
	typedef struct _CUP_message {
		CUP_command command;
		uint8_t data_len;
		uint8_t* data;
		uint8_t crc8;
	} CUP_message;

private:

	const uint8_t CUP_WAIT_TIMEOUT = 100;


	uint8_t data_counter = 0;
	uint8_t* device_data;
	uint16_t  device_data_len = 0;
	uint8_t* settings_data;
	uint16_t  settings_data_len = 0;
	uint8_t* sensors_data;
	uint16_t  sensors_data_len = 0;

	void (CUPSlaveManager::*curr_status_action) (uint8_t msg);
	void status_wait(uint8_t msg);
	void status_status(uint8_t msg);
	void status_data_len(uint8_t msg);
	void status_data(uint8_t msg);
	void status_check_crc(uint8_t msg);

	void send_response();
	void send_error(CUP_error error_type);
	void reset_data();
	void save_request_data();
	uint8_t get_message_crc(CUP_message* message);
	uint8_t get_CRC8(uint8_t* buffer, uint8_t size);

public:
	CUP_message request;
	CUP_message response;

	CUPSlaveManager();

	void char_data_handler(uint8_t msg);
	void send_byte(uint8_t msg);
	void timeout();

	void update_device_handler(void);
	// Update sensors data function, implemented outside the class
	void update_sensors_handler(void);
	// Update settings data function, implemented outside the class
	void update_settings_handler(void);

	bool validate_settings_handler(uint8_t *data, uint8_t len);

};


#endif
