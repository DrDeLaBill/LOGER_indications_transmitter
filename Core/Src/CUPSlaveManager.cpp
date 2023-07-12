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

void CUPSlaveManager::proccess() {
	if (this->response_state == CUP_RESPONSE_SUCCESS) {
		this->load_request_data();
		this->send_success_response();
		this->reset_data();
	} else if (this->response_state == CUP_RESPONSE_ERROR) {
		this->send_error_response();
		this->reset_data();
	}
}

void CUPSlaveManager::char_data_handler(uint8_t msg) {
	(this->*curr_status_action)(msg);
}

void CUPSlaveManager::send_success_response() {
	HAL_GPIO_WritePin(BEDUG_LED_GPIO_Port, BEDUG_LED_Pin, GPIO_PIN_SET);
	uint8_t response_buffer[MESSAGE_BUFFER_SIZE] = {0};

	uint16_t size = this->load_message_to_buffer(response_buffer);
	uint8_t crc8 = this->get_CRC8(response_buffer, size);
	this->message.crc8 = crc8;
	size += serialize(&response_buffer[size], crc8);

	this->send_buffer(response_buffer, size);

	this->reset_data();
	HAL_GPIO_WritePin(BEDUG_LED_GPIO_Port, BEDUG_LED_Pin, GPIO_PIN_RESET);
}

void CUPSlaveManager::send_error_response() {
	this->message.command = (CUP_command)(0xFF ^ (uint8_t)this->message.command);
	this->message.data_len = 1;
	this->message.data[0] = this->cup_error_type;

	this->message.crc8 = this->get_message_crc();

	uint8_t error_buffer[MESSAGE_BUFFER_SIZE] = {0};

	uint16_t counter = 0;
	counter += serialize(&error_buffer[counter], this->message.command);
	counter += serialize(&error_buffer[counter], this->message.data_len);
	counter += serialize(&error_buffer[counter], this->message.data[0]);
	counter += serialize(&error_buffer[counter], this->message.crc8);

	this->send_buffer(error_buffer, counter);

	this->reset_data();
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
		counter += this->load_data_to_buffer_handler(&buffer[counter]);
	}

	return counter;
}

void CUPSlaveManager::load_request_data() {
	if (this->message.command != CUP_CMD_STTNGS) {
		return;
	}

	load_settings_data_handler();
}

void CUPSlaveManager::timeout()
{
  if (this->data_counter > 0) {
	  this->set_response_state_error(CUP_ERROR_TIMEOUT);
  }
}

void CUPSlaveManager::reset_data() {
	memset(&this->message, 0, sizeof(this->message));
	this->set_new_status(&CUPSlaveManager::status_wait);
	this->response_state = CUP_RESPONSE_WAIT;
	this->cup_error_type = CUP_NO_ERROR;
}

void CUPSlaveManager::set_response_state_error(CUP_error error_type) {
	this->cup_error_type = error_type;
	this->response_state = CUP_RESPONSE_ERROR;
	this->set_new_status(&CUPSlaveManager::status_request_error);
}

void CUPSlaveManager::set_response_state_success() {
	this->response_state = CUP_RESPONSE_SUCCESS;
	this->cup_error_type = CUP_NO_ERROR;
}

void CUPSlaveManager::set_new_status(void (CUPSlaveManager::*status_action) (uint8_t)) {
	this->curr_status_action = status_action;
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
			this->set_new_status(&CUPSlaveManager::status_data_len);
			break;
		default:
			this->set_response_state_error(CUP_ERROR_COMMAND);
			break;
	};
}

void CUPSlaveManager::status_data_len(uint8_t msg) {
	if (!fill_param(&(this->message.data_len), msg)) {
		return;
	}

	if (this->message.data_len > sizeof(this->message.data)) {
		this->set_response_state_error(CUP_ERROR_DATA_LEN);
		return;
	}

	if (this->message.data_len > 0) {
		this->set_new_status(&CUPSlaveManager::status_data);
	} else {
		this->set_new_status(&CUPSlaveManager::status_check_crc);
	}
}

void CUPSlaveManager::status_data(uint8_t msg) {
	if (this->data_counter > sizeof(this->message.data)) {
		LOG_BEDUG(MODULE_TAG, " ERROR - CUP buffer out of range\n");
		this->set_response_state_error(CUP_ERROR_DATA_OVERLOAD);
		return;
	}

	this->message.data[this->data_counter++] = msg;

	if (this->data_counter > this->message.data_len) {
		this->set_response_state_error(CUP_ERROR_DATA_OVERLOAD);
	}

	if (this->data_counter == this->message.data_len) {
		this->set_new_status(&CUPSlaveManager::status_check_crc);
	}
}

void CUPSlaveManager::status_check_crc(uint8_t msg) {
	if (!fill_param(&(this->message.crc8), msg)) {
		return;
	}

	if (msg == this->get_message_crc()) {
		this->set_response_state_success();
		this->set_new_status(&CUPSlaveManager::status_request_success);
	} else {
		this->set_response_state_error(CUP_ERROR_CRC);
	}
}

void CUPSlaveManager::status_request_success(uint8_t msg) {
	if (this->cup_error_type != CUP_NO_ERROR) {
		this->reset_data();
	}
}

void CUPSlaveManager::status_request_error(uint8_t msg) {
	if (this->cup_error_type == CUP_NO_ERROR) {
		LOG_BEDUG(MODULE_TAG, " unknown error => reset\n");
		this->reset_data();
	}
}

uint8_t CUPSlaveManager::get_message_crc()
{
	uint8_t buffer[MESSAGE_BUFFER_SIZE] = {0};

	uint16_t count = 0;
	count += serialize(&buffer[count], this->message.command);
	count += serialize(&buffer[count], this->message.data_len);
	for (uint16_t i = 0; i < this->message.data_len; i++) {
		count += serialize(&buffer[count], this->message.data[i]);
	}

	uint8_t crc = get_CRC8(buffer, count);

	return crc;
}

uint8_t CUPSlaveManager::get_CRC8(uint8_t* buffer, uint16_t size) {
  uint8_t crc = 0;
  for (uint16_t i = 0; i < size; i++) {
	uint8_t data = buffer[i];
	for (uint16_t j = 8; j > 0; j--) {
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
