#include "SensorManager.h"

#include <string.h>

#include "DeviceStateBase.h"
#include "SettingsManager.h"
#include "RecordManager.h"

#include "storage_data_manager.h"
#include "modbus_rtu_master.h"


#define MODBUS_CHECK(condition, time) if (!Wait_Event(condition, time)) {return;}

#define MODBUS_RESPONSE_TIME          100 //ms

#define MODBUS_REQ_TIMEOUT            100 //ms

#define MODBUS_MAX_ERRORS             50


const char* SensorManager::MODULE_TAG = "SENS";
SensorManager::sensor_read_status SensorManager::sens_status = SENS_WAIT;
uint8_t SensorManager::current_slave_id = RESERVED_IDS_COUNT;
uint8_t SensorManager::mb_send_data_buffer[MODBUS_SIZE_MAX] = {0};
uint8_t SensorManager::mb_send_data_length = 0;


SensorManager::SensorManager() {
    DeviceStateBase();
    this->current_action = &SensorManager::start_action;
    util_timer_start(&this->wait_record_timer, 0);

    modbus_master_set_request_data_sender(SensorManager::send_request);
    modbus_master_set_response_packet_handler(SensorManager::response_packet_handler);
}

void SensorManager::proccess() {
    (this->*current_action)();

    if (mb_send_data_length) {
        HAL_GPIO_WritePin(MODBUS1_EN_GPIO_Port, MODBUS1_EN_Pin, GPIO_PIN_SET);
        HAL_UART_Transmit(&LOW_MB_UART, mb_send_data_buffer, mb_send_data_length, MODBUS_REQ_TIMEOUT);
        HAL_GPIO_WritePin(MODBUS1_EN_GPIO_Port, MODBUS1_EN_Pin, GPIO_PIN_RESET);

        mb_send_data_length = 0;
    }
}

void SensorManager::sleep() {
    this->current_action = &SensorManager::sleep_action;
}

void SensorManager::awake() {
    this->current_action = &SensorManager::start_action;
}

void SensorManager::start_action() {
    SensorManager::current_slave_id = RESERVED_IDS_COUNT;
    SensorManager::sens_status = SENS_WAIT;
    this->error_count = 0;
    this->current_action = &SensorManager::request_action;
}

void SensorManager::wait_record_action() {
    this->current_action = &SensorManager::request_action;
    if (util_is_timer_wait(&this->wait_record_timer)) {
        return;
    }
    if (!this->write_sensors_data()) {
        LOG_TAG_BEDUG(MODULE_TAG, " error write sensors data\n");
        return;
    }
    util_timer_start(&this->wait_record_timer, SettingsManager::sttngs.sens_record_period);
}

void SensorManager::request_action() {
    if (this->current_slave_id >= LOW_MB_SENS_COUNT) {
        this->current_slave_id = RESERVED_IDS_COUNT;
        this->current_action = &SensorManager::wait_record_action;
        return;
    }

    if (this->current_slave_id < RESERVED_IDS_COUNT) {
        this->current_slave_id = RESERVED_IDS_COUNT;
    }
    //TODO: для проверки
    current_slave_id = 0x01;
    SettingsManager::sttngs.low_sens_val_register[current_slave_id] = 0;

    modbus_master_read_input_registers(current_slave_id, SettingsManager::sttngs.low_sens_val_register[current_slave_id], 1);

    SensorManager::sens_status = SENS_WAIT;
    util_timer_start(&this->wait_modbus_timer, MODBUS_REQ_TIMEOUT);
    this->current_action = &SensorManager::wait_response_action;
}

void SensorManager::wait_response_action() {
    if (SensorManager::sens_status == SENS_SUCCESS) {
        this->current_action = &SensorManager::success_response_action;
        return;
    }
    if (SensorManager::sens_status == SENS_ERROR) {
        this->current_action = &SensorManager::error_response_action;
        return;
    }
    if (util_is_timer_wait(&this->wait_modbus_timer)) {
        return;
    }
    modbus_master_timeout();
    this->current_action = &SensorManager::error_response_action;
}

void SensorManager::success_response_action() {
    this->update_modbus_slave_id();
    this->error_count = 0;
    this->current_action = &SensorManager::request_action;
}

void SensorManager::error_response_action() {
    // TODO: обработка ошибок modbus
    this->error_count++;
    if (this->error_count >= MODBUS_MAX_ERRORS) {
//        this->registrate_modbus_error();
        this->update_modbus_slave_id();
        this->error_count = 0;
    }
    this->current_action = &SensorManager::request_action;
}

void SensorManager::register_action() {}

void SensorManager::sleep_action() {}

bool SensorManager::write_sensors_data() {
    uint32_t new_id = 0;
    RecordManager::record_status_t status = RecordManager::get_new_record_id(&new_id);
    if (status != RecordManager::RECORD_OK) {
        return false;
    }

    RecordManager::record.record_id = new_id;

    RecordManager::record.record_time = HAL_GetTick();

    status = RecordManager::save();
    if (status != RecordManager::RECORD_OK) {
        return false;
    }

    return true;
}

void SensorManager::send_request(uint8_t *data, uint8_t len) {
    if (len > sizeof(mb_send_data_buffer)) {
        LOG_TAG_BEDUG(MODULE_TAG, " modbus buffer out of range\n");
        return;
    }
    mb_send_data_length = len;
    memset(mb_send_data_buffer, 0, sizeof(mb_send_data_buffer));
    memcpy(mb_send_data_buffer, data, len);
}

void SensorManager::response_packet_handler(modbus_response_t* packet) {
    if(packet->status != MODBUS_NO_ERROR) {
        SensorManager::sens_status = SENS_ERROR;
        return;
    }
    RecordManager::record.sensors_values[current_slave_id] = packet->response[0];
    RecordManager::record.sensors_statuses[current_slave_id] = SettingsManager::sttngs.low_sens_status[current_slave_id];
    SensorManager::sens_status = SENS_SUCCESS;
    LOG_TAG_BEDUG(SensorManager::MODULE_TAG, "[%02u] A: %u ", current_slave_id, packet->response[0]);
}


void SensorManager::update_modbus_slave_id() {
    do {
        this->current_slave_id++;
        if (SettingsManager::sttngs.low_sens_status[current_slave_id] >= SENSOR_TYPE_THERMAL) {
            return;
        }
    } while (this->current_slave_id < sizeof(SettingsManager::sttngs.low_sens_status));
}

void SensorManager::registrate_modbus_error() {
    SettingsManager::sttngs.low_sens_status[this->current_slave_id] = SENSOR_ERROR;
    SettingsManager::save();
}
