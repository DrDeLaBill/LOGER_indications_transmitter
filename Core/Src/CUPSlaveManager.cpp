#include "CUPSlaveManager.h"

#include <string.h>


const char* CUPSlaveManager::MODULE_TAG = "CUP";


CUPSlaveManager::CUPSlaveManager() {
	this->reset_data();
}

void CUPSlaveManager::set_settings_data(uint8_t* data) {
	this->settings_data = data;
}

void CUPSlaveManager::set_sensors_data(uint8_t* data) {
	this->sensors_data = data;
}

void CUPSlaveManager::char_data_handler(uint8_t msg) {
	(this->*curr_status_action)(msg);
}

void CUPSlaveManager::send_response() {
	switch (this->request.command) {
		case CUP_CMD_STATUS:
			break;
		case CUP_CMD_SENSRS:
			this->update_sensors_handler();
			this->response.data = this->sensors_data;
			break;
		case CUP_CMD_STTNGS:
			this->update_settings_handler();
			this->response.data = this->settings_data;
			break;
		default:
			this->send_error();
			return;
	};
	this->response.command = this->request.command;
	this->response.data_len = sizeof(this->response.data);

	uint8_t* ptr = (uint8_t*)&this->response;
	this->response.crc8 = this->get_CRC8(ptr, sizeof(response) - 1);

	for (uint8_t *i = ptr; i < ptr + sizeof(this->response); i++) {
		this->send_byte(*i);
	}

	this->reset_data();
}

void CUPSlaveManager::send_error() {
	this->response.command = (CUP_command)(0xFF ^ (uint8_t)this->request.command);
	this->response.data_len = 1;
	uint8_t data_buf[1] = {0xFF};
	this->response.data = (uint8_t*)data_buf;

	uint8_t* ptr = (uint8_t*)&this->response;
	this->response.crc8 = this->get_CRC8(ptr, sizeof(response) - 1);

	for (uint8_t *i = ptr; i < ptr + sizeof(this->response); i++) {
		this->send_byte(*i);
	}

	this->reset_data();
}

void CUPSlaveManager::reset_data() {
	memset(&this->request, 0, sizeof(this->request));
	memset(&this->response, 0, sizeof(this->response));
	this->curr_status_action = &CUPSlaveManager::status_wait;
	this->data_counter = 0;
}

void CUPSlaveManager::save_request_data() {
	uint8_t* ptr = NULL;
	switch (this->request.command) {
		case CUP_CMD_STATUS:
			break;
		case CUP_CMD_SENSRS:
			ptr = (uint8_t*)this->sensors_data;
			break;
		case CUP_CMD_STTNGS:
			ptr = (uint8_t*)this->settings_data;
			break;
		default:
			break;
	};
	if (!ptr) {
		LOG_DEBUG(MODULE_TAG, " ERROR - no data to save\n");
		return;
	}
	if (this->request.data_len != sizeof(ptr)) {
		LOG_DEBUG(MODULE_TAG, " ERROR - invalid request data\n");
		return;
	}
	memcpy(ptr, (uint8_t*)this->request.data, this->request.data_len);
	Debug_HexDump(MODULE_TAG, ptr, this->request.data_len);
}

void CUPSlaveManager::status_wait(uint8_t msg) {
	this->request.command = (CUP_command)msg;
	switch (msg) {
		case CUP_CMD_STATUS:
		case CUP_CMD_SENSRS:
		case CUP_CMD_STTNGS:
			this->curr_status_action = &CUPSlaveManager::status_data_len;
			break;
		default:
			this->send_error();
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
		this->reset_data();
		return;
	}
	if (this->data_counter > sizeof(this->request.data)) {
		LOG_DEBUG(MODULE_TAG, " ERROR - CUP buffer out of range\n");
		this->reset_data();
		return;
	}
	this->request.data[this->data_counter++] = msg;
	if (this->data_counter >= this->request.data_len) {
		this->curr_status_action = &CUPSlaveManager::status_check_crc;
	}
}

void CUPSlaveManager::status_check_crc(uint8_t msg) {
	this->request.crc8 = msg;
	uint8_t* ptr = (uint8_t*)&this->request;
	if (this->request.crc8 == this->get_CRC8(ptr, sizeof(this->request) - 1)) {
		this->save_request_data();
		this->send_response();
	}
	this->reset_data();
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
