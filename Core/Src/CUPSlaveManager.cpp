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
	uint8_t* response_buffer = new uint8_t[sizeof(CUP_message)] {0};

	uint16_t size = this->load_message_to_buffer(response_buffer);
	uint8_t crc8 = this->get_CRC8(response_buffer, size);
	this->message.crc8 = crc8;
	size += serialize(&response_buffer[size], crc8);

	this->send_buffer(response_buffer, size);

	this->reset_data();

	delete[] response_buffer;
}

uint16_t CUPSlaveManager::load_message_to_buffer(uint8_t* buffer) {
	uint16_t counter = 0;

	counter += serialize(&buffer[counter], this->message.command);

	if (this->message.command == CUP_CMD_DEVICE) {
		this->message.data_len = sizeof(device_info_t);
		counter += serialize(&buffer[counter], this->message.data_len);

		counter += serialize(&buffer[counter], device_info.device_type);
		counter += serialize(&buffer[counter], device_info.device_version);
		counter += serialize(&buffer[counter], device_info.id_base1);
		counter += serialize(&buffer[counter], device_info.id_base2);
		counter += serialize(&buffer[counter], device_info.id_base3);
		counter += serialize(&buffer[counter], device_info.id_base4);
	} else {
		load_data_to_buffer_handler(&buffer[counter]);
	}

	return counter;
}

void CUPSlaveManager::load_request_data() {
	if (this->message.command != CUP_CMD_STTNGS) {
		return;
	}

	load_settings_data_handler();

	SettingsManager::save();
}

void CUPSlaveManager::timeout()
{
  if (this->curr_status_action == &CUPSlaveManager::status_wait) {
    return;
  }
  this->send_error(CUP_ERROR_TIMEOUT);
}

void CUPSlaveManager::send_error(CUP_error error_type) {
	this->message.command = (CUP_command)(0xFF ^ (uint8_t)this->message.command);
	this->message.data_len = 1;
	this->message.data[0] = error_type;

	this->message.crc8 = this->get_message_crc();

	uint16_t size = sizeof(this->message.command) + sizeof(this->message.data_len) + sizeof(this->message.data[0]) + sizeof(this->message.crc8);
	uint8_t* error_buffer = new uint8_t[size];

	uint16_t counter = 0;
	counter += serialize(&error_buffer[counter], this->message.command);
	counter += serialize(&error_buffer[counter], this->message.data_len);
	counter += serialize(&error_buffer[counter], this->message.data[0]);
	counter += serialize(&error_buffer[counter], this->message.crc8);

	this->send_buffer(error_buffer, counter);

	delete[] error_buffer;

	this->reset_data();
}

void CUPSlaveManager::reset_data() {
	memset(&this->message, 0, sizeof(this->message));
	this->curr_status_action = &CUPSlaveManager::status_wait;
	this->data_counter = 0;
}

void CUPSlaveManager::status_wait(uint8_t msg) {
	if (!fill_param(&(this->message.command), msg)) {
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
	if (!fill_param(&(this->message.data_len), msg)) {
		return;
	}

	if (this->message.data_len > 0) {
		this->curr_status_action = &CUPSlaveManager::status_data;
	} else {
		this->curr_status_action = &CUPSlaveManager::status_check_crc;
	}
}

void CUPSlaveManager::status_data(uint8_t msg) {
	if (this->data_counter > sizeof(this->message.data)) {
		LOG_DEBUG(MODULE_TAG, " ERROR - CUP buffer out of range\n");
		this->send_error(CUP_ERROR_DATA_OVERLOAD);
		return;
	}
	this->message.data[this->data_counter++] = msg;
	if (this->data_counter >= this->message.data_len) {
		this->data_counter = 0;
		this->curr_status_action = &CUPSlaveManager::status_check_crc;
	}
}

void CUPSlaveManager::status_check_crc(uint8_t msg) {
	if (!fill_param(&(this->message.crc8), msg)) {
		return;
	}

	if (msg == this->get_message_crc()) {
		this->load_request_data();
		this->send_response();
		this->reset_data();
		return;
	}

	this->send_error(CUP_ERROR_CRC);
}

uint8_t CUPSlaveManager::get_message_crc()
{
	uint8_t* buffer = new uint8_t[sizeof(CUP_command)] {0};

	uint16_t count = 0;
	count += serialize(&buffer[count], this->message.command);
	count += serialize(&buffer[count], this->message.data_len);
	for (uint16_t i = 0; i < this->message.data_len; i++) {
		count += serialize(&buffer[count], this->message.data[i]);
	}

	uint8_t crc = get_CRC8(buffer, count);

	delete[] buffer;

	return crc;
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

void CUPSlaveManager::send_buffer(uint8_t* buffer, uint16_t len) {
	for (uint16_t i = 0; i < len; i++) {
		this->send_byte(buffer[i]);
	}
}
