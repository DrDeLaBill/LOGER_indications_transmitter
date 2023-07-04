#include "CUPSlaveManager.h"

#include <string.h>

#include "SettingsManager.h"
#include "utils.h"


const char* CUPSlaveManager::MODULE_TAG = "CUP";


CUPSlaveManager::CUPSlaveManager() {
	this->reset_data();

	device_info.device_type = SettingsManager::DEVICE_TYPE_LOGER;
	device_info.device_version = DEVICE_VERSION;
	device_info.id_base1 = *((uint16_t*)(UID_BASE));
	device_info.id_base2 = *((uint16_t*)(UID_BASE + 0x02));
	device_info.id_base3 = *((uint32_t*)(UID_BASE + 0x04));
	device_info.id_base4 = *((uint32_t*)(UID_BASE + 0x08));
}

void CUPSlaveManager::char_data_handler(uint8_t msg) {

	(this->*curr_status_action)(msg);
}

void CUPSlaveManager::send_response() {
	this->response.command = this->request.command;

	uint8_t response_buffer[SD_PAYLOAD_BITS_SIZE(STORAGE_SD_MAX_PAYLOAD_SIZE)] = {0};
	uint16_t counter = 0;

	counter += serialize(&response_buffer[counter], this->response.command);

	this->response.data_len = get_data_len((CUP_command)this->response.command);
	counter += serialize(&response_buffer[counter], this->response.data_len);

	if (this->response.command == CUP_CMD_DEVICE) {
		counter += serialize(&response_buffer[counter], device_info.device_type);
		counter += serialize(&response_buffer[counter], device_info.device_version);
		counter += serialize(&response_buffer[counter], device_info.id_base1);
		counter += serialize(&response_buffer[counter], device_info.id_base2);
		counter += serialize(&response_buffer[counter], device_info.id_base3);
		counter += serialize(&response_buffer[counter], device_info.id_base4);
	} else {
		set_request_data_handler(&response_buffer[counter], (CUP_command)this->response.command);
	}
	counter += this->response.data_len;

	uint8_t crc8 = this->get_CRC8(response_buffer, counter - 1);
	this->response.crc8 = crc8;
	counter += serialize(&response_buffer[counter], crc8);

	for (uint16_t i = 0; i < counter; i++) {
		this->send_byte(response_buffer[i]);
	}

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
	this->response.data[0] = error_type;

	this->response.crc8 = this->get_message_crc(&(this->response));

	this->send_response();
}

void CUPSlaveManager::reset_data() {
	memset(&this->request, 0, sizeof(this->request));
	memset(&this->response, 0, sizeof(this->response));
	this->curr_status_action = &CUPSlaveManager::status_wait;
	this->data_counter = 0;
}

void CUPSlaveManager::save_settings_data() {
	if (this->request.command != CUP_CMD_STTNGS) {
		return;
	}

	load_settings_data_handler(this->request.data);

	SettingsManager::save();
}

void CUPSlaveManager::status_wait(uint8_t msg) {
	if (!fill_param(&(this->request.command), msg)) {
		return;
	}

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
	if (!fill_param(&(this->request.data_len), msg)) {
		return;
	}

	if (msg > 0) {
		this->curr_status_action = &CUPSlaveManager::status_data;
	} else {
		this->curr_status_action = &CUPSlaveManager::status_check_crc;
	}
}

void CUPSlaveManager::status_data(uint8_t msg) {
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
		this->save_settings_data();
		this->send_response();
		this->reset_data();
		return;
	}

	this->send_error(CUP_ERROR_CRC);
}

uint8_t CUPSlaveManager::get_message_crc(CUP_message* message)
{
	return get_CRC8((uint8_t*)&message, sizeof(message) - 1);
}

uint8_t CUPSlaveManager::get_CRC8(uint8_t* buffer, uint16_t size) {
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

uint16_t CUPSlaveManager::get_data_len(CUP_command command) {
	if (command == CUP_CMD_DEVICE) {
		return sizeof(device_info_t);
	}
	return get_data_len_handler(command);
}
