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
#include "internal_storage.h"


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
		uint8_t command;
		uint16_t data_len;
		uint8_t data[SD_PAYLOAD_BITS_SIZE(STORAGE_SD_MAX_PAYLOAD_SIZE)];
		uint8_t crc8;
	} CUP_message;

private:
	const uint8_t CUP_WAIT_TIMEOUT = 50;

	uint8_t data_counter = 0;

	void (CUPSlaveManager::*curr_status_action) (uint8_t msg);

	template <typename T>
	bool fill_param(T* param, uint8_t msg) {
		*param <<= 8;
		*param += (msg & 0xFF);
		this->data_counter++;

		if (this->data_counter < sizeof(*param)) {
			return false;
		}

		this->data_counter = 0;
		return true;
	}

	void status_wait(uint8_t msg);
	void status_status(uint8_t msg);
	void status_data_len(uint8_t msg);
	void status_data(uint8_t msg);
	void status_check_crc(uint8_t msg);

	void send_response();
	void send_error(CUP_error error_type);
	void send_buffer(uint8_t* buffer, uint16_t len);
	void reset_data();
	void save_settings_data();
	uint8_t get_message_crc(CUP_message* message);
	uint8_t get_CRC8(uint8_t* buffer, uint16_t size);

	uint16_t get_data_len(CUP_command command);
	uint16_t get_data_len_handler(CUP_command command);
	void set_request_data_handler(uint8_t* buffer, CUP_command command);
	void load_settings_data_handler(uint8_t* buffer);

public:
	struct device_info_t {
		uint8_t device_type;
		uint8_t device_version;
		uint32_t id_base1;
		uint32_t id_base2;
		uint32_t id_base3;
		uint32_t id_base4;
	} device_info;

	CUP_message request;
	CUP_message response;

	CUPSlaveManager();

	void char_data_handler(uint8_t msg);
	void send_byte(uint8_t msg);
	void timeout();

	bool validate_settings_handler(uint8_t *data, uint8_t len);

	template <typename T>
	static uint16_t deserialize(uint8_t* buffer, T* variable) {
		for (uint8_t* ptr = buffer + sizeof(*variable); ptr > buffer; ptr--) {
			*variable <<= 8;
			*variable += *ptr;
		}
		return sizeof(*variable);
	}

	template <typename T>
	static uint16_t serialize(uint8_t* buffer, T variable) {
		for (uint8_t i = 0; i < sizeof(variable); i++, buffer++) {
			*buffer = (uint8_t)(variable & 0xFF);
			variable >>= 8;
		}
		return sizeof(variable);
	}

};


#endif
