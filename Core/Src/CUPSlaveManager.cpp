#include "CUPSlaveManager.h"

#include <string.h>

#include "SettingsManager.h"


const char* CUPSlaveManager::MODULE_TAG = "CUP";


CUPSlaveManager::CUPSlaveManager() {
	this->reset_data();
}

void CUPSlaveManager::char_data_handler(uint8_t msg) {
	(this->*curr_status_action)(msg);
}

void CUPSlaveManager::send_response() {
	this->response.command = this->request.command;

	switch (this->response.command) {
		case CUP_CMD_DEVICE:
			this->update_device_handler();
			this->response.data = this->device_data;
			this->response.data_len = this->device_data_len;
			break;
		case CUP_CMD_DATA:
			this->update_sensors_handler();
			this->response.data = this->sensors_data;
			this->response.data_len = this->sensors_data_len;
			break;
		case CUP_CMD_STTNGS:
			this->update_settings_handler();
			this->response.data = this->settings_data;
			this->response.data_len = this->settings_data_len;
			break;
		default:
			break;
	};

	uint16_t buffer_size = 3 + this->response.data_len;
	uint8_t* response_buffer = new uint8_t[buffer_size];
	response_buffer[0] = this->response.command;
	response_buffer[1] = this->response.data_len;
	for (uint8_t i = 0; i < this->response.data_len; i++) {
		response_buffer[2 + i] = this->response.data[i];
	}
	this->response.crc8 = this->get_CRC8(response_buffer, buffer_size - 1);
	response_buffer[buffer_size - 1] = this->response.crc8;

	for (uint16_t i = 0; i < buffer_size; i++) {
		this->send_byte(response_buffer[i]);
	}

	delete [] response_buffer;

	this->reset_data();
}

void CUPSlaveManager::timeout()
{
  if (this->curr_status_action == &CUPSlaveManager::status_wait) {
    return;
  }
	this->send_error(CUP_ERROR_TIMEOUT);
}

void CUPSlaveManager::send_error(CUP_error error_type) {
	this->request.command = (CUP_command)(0xFF ^ (uint8_t)this->request.command);
	this->response.data_len = 1;
	uint8_t data_buf[1] = {error_type};
	this->response.data = (uint8_t*)data_buf;

	this->response.crc8 = this->get_message_crc(&(this->response));

	this->send_response();
}

void CUPSlaveManager::reset_data() {
	memset(&this->request, 0, sizeof(this->request));
	memset(&this->response, 0, sizeof(this->response));
	this->curr_status_action = &CUPSlaveManager::status_wait;
	this->data_counter = 0;
}

void CUPSlaveManager::save_request_data() {
	if (this->request.command != CUP_CMD_STTNGS) {
		return;
	}

	uint8_t* ptr = (uint8_t*)this->settings_data;

	if (!ptr) {
		LOG_DEBUG(MODULE_TAG, " ERROR - no data to save\n");
		return;
	}

	if (this->request.data_len != sizeof(ptr)) {
		LOG_DEBUG(MODULE_TAG, " ERROR - invalid request data\n");
		return;
	}

	if (!this->validate_settings_handler(this->request.data, this->request.data_len)) {
		return;
	}

	memcpy(ptr, (uint8_t*)this->request.data, this->request.data_len);
	Debug_HexDump(MODULE_TAG, ptr, this->request.data_len);

	SettingsManager::save();
}

void CUPSlaveManager::status_wait(uint8_t msg) {
	this->request.command = (CUP_command)msg;
	switch (msg) {
		case CUP_CMD_DEVICE:
		case CUP_CMD_STTNGS:
		case CUP_CMD_DATA:
			this->curr_status_action = &CUPSlaveManager::status_data_len;
			break;
		default:
			this->send_error(CUP_ERROR_COMMAND);
			break;
	};
}

void CUPSlaveManager::status_data_len(uint8_t msg) {
	this->request.data_len = msg;
	if (msg > 0) {
		this->curr_status_action = &CUPSlaveManager::status_data;
	} else {
		this->curr_status_action = &CUPSlaveManager::status_check_crc;
	}
}

void CUPSlaveManager::status_data(uint8_t msg) {
	if (!sizeof(this->request.data)) {
		LOG_DEBUG(MODULE_TAG, " ERROR - CUP data buffer is NULL\n");
		this->send_error(CUP_ERROR_DATA_DOES_NOT_EXIST);
		return;
	}
	if (this->data_counter > sizeof(this->request.data)) {
		LOG_DEBUG(MODULE_TAG, " ERROR - CUP buffer out of range\n");
		this->send_error(CUP_ERROR_DATA_OVERLOAD);
		return;
	}
	this->request.data[this->data_counter++] = msg;
	if (this->data_counter >= this->request.data_len) {
		this->curr_status_action = &CUPSlaveManager::status_check_crc;
	}
}

void CUPSlaveManager::status_check_crc(uint8_t msg) {
	this->request.crc8 = msg;

	if (msg == this->get_message_crc(&(this->request))) {
		this->save_request_data();
		this->send_response();
		this->reset_data();
		return;
	}

	this->send_error(CUP_ERROR_CRC);
}

uint8_t CUPSlaveManager::get_message_crc(CUP_message* message)
{
	uint8_t* crc_buffer = new uint8_t[2 + message->data_len];
	crc_buffer[0] = message->command;
	crc_buffer[1] = message->data_len;
	for (uint8_t i = 0; i < message->data_len; i++) {
		crc_buffer[2 + i] = message->data[i];
	}
	uint8_t crc = get_CRC8(crc_buffer, sizeof(crc_buffer));
	delete [] crc_buffer;
	return crc;
}

uint8_t CUPSlaveManager::get_CRC8(uint8_t* buffer, uint8_t size) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < size; i++) {
	uint8_t data = buffer[i];
	for (int j = 8; j > 0; j--) {
	  crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
	  data >>= 1;
	}
  }
  return crc;
}
