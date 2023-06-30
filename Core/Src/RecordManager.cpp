#include "RecordManager.h"

#include <string.h>
#include <fatfs.h>
#include "internal_storage.h"
#include "utils.h"

#define RETURN_FIRST_ID() { LOG_DEBUG(MODULE_TAG, " set first record id - %d\n", 1); return 1; }


typedef RecordManager RM;


const char* RM::MODULE_TAG = "RCRD";
const char* RM::RECORDS_FILENAME = "records.bin";

RM::record_sd_payload_t RM::sd_record = {0};


RM::record_status_t RM::load(uint32_t record_id) {
	memset(&(sd_record), 0, sizeof(sd_record));

	bool record_load_ok = true;

	UINT br = 0;
	UINT ptr = 0;
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORDS_FILENAME);

	FRESULT res;
do_readline:
	br = 0;
	res = intstor_read_line(filename, &(sd_record), sizeof(sd_record), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = false;
		LOG_DEBUG(MODULE_TAG, " read_file(%s) error=%i\n", filename, res);
	}
	if (br == 0) {
		return RECORD_EMPTY;
	}
	if (!sd_record.v1.payload_record.record_id) {
		ptr += sizeof(sd_record);
		goto do_readline;
	}
	if (sd_record.v1.payload_record.record_id != record_id) {
		ptr += sizeof(sd_record);
		goto do_readline;
	}

	if(sd_record.header.magic != STORAGE_SD_PAYLOAD_MAGIC) {
		record_load_ok = false;
		LOG_DEBUG(MODULE_TAG, " bad record magic %08lX!=%08lX\n", sd_record.header.magic, STORAGE_SD_PAYLOAD_MAGIC);
	}

	if(sd_record.header.version != STORAGE_SD_PAYLOAD_VERSION) {
		record_load_ok = false;
		LOG_DEBUG(MODULE_TAG, " bad record version %i!=%i\n", sd_record.header.version, STORAGE_SD_PAYLOAD_VERSION);
	}

	if(!record_load_ok) {
		LOG_DEBUG(MODULE_TAG, " record not loaded\r\n");
		return RECORD_ERROR;
	}

	LOG_DEBUG(MODULE_TAG, " loading record\n");

	if(!record_load_ok) return RECORD_ERROR;

	return RECORD_OK;
}

RM::record_status_t RM::save() {
	RM::sd_record.header.magic = STORAGE_SD_PAYLOAD_MAGIC;
	RM::sd_record.header.version = STORAGE_SD_PAYLOAD_VERSION;

	WORD crc = 0;
	for(uint16_t i = 0; i < sizeof(RM::sd_record.bits); i++)
		DIO_SPI_CardCRC16(&crc, RM::sd_record.bits[i]);
	RM::sd_record.crc = crc;

	LOG_DEBUG(MODULE_TAG, "saving record\n");
	Debug_HexDump(MODULE_TAG, (uint8_t*)&(RM::sd_record), sizeof(RM::sd_record));

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORDS_FILENAME);

	UINT br;
	FRESULT res = intstor_append_file(filename, &(RM::sd_record), sizeof(RM::sd_record), &br);
	if(res != FR_OK) {
		LOG_DEBUG(MODULE_TAG, "settings NOT record\n");
		return RECORD_ERROR;
	}

	LOG_DEBUG(MODULE_TAG, "settings record\n");

	clear_buf();

	return RECORD_OK;
}

void RM::clear_buf() {
	memset(&RM::sd_record, 0, sizeof(RM::sd_record));
}

uint32_t RM::get_new_record_id()
{
	LOG_DEBUG(MODULE_TAG, " get new log id\n");
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	bool record_load_ok = true;

	FRESULT res = instor_find_file(RECORDS_FILENAME);
	if (res != FR_OK) {
		record_load_ok = false;
		LOG_DEBUG(MODULE_TAG, " find_file(%s) error=%i\n", RECORDS_FILENAME, res);
		RETURN_FIRST_ID();
	}

	UINT br = 0;
	UINT ptr = DIOSPIFileInfo.fsize - sizeof(record_sd_payload_t);
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORDS_FILENAME);

	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = false;
		LOG_DEBUG(MODULE_TAG, " read_file(%s) error=%i\n", filename, res);
	}
	if (br == 0) {
		RETURN_FIRST_ID();
	}
	if (!tmpbuf.v1.payload_record.record_id) {
		RETURN_FIRST_ID();
	}

	if(tmpbuf.header.magic != STORAGE_SD_PAYLOAD_MAGIC) {
		record_load_ok = false;
		LOG_DEBUG(MODULE_TAG, " bad record magic %08lX!=%08lX\n", tmpbuf.header.magic, STORAGE_SD_PAYLOAD_MAGIC);
	}

	if(tmpbuf.header.version != STORAGE_SD_PAYLOAD_VERSION) {
		record_load_ok = false;
		LOG_DEBUG(MODULE_TAG, " bad record version %i!=%i\n", tmpbuf.header.version, STORAGE_SD_PAYLOAD_VERSION);
	}

	if(!record_load_ok) {
		LOG_DEBUG(MODULE_TAG, " record not loaded\n");
		RETURN_FIRST_ID();
	}

	uint32_t new_id = tmpbuf.v1.payload_record.record_id + 1;
	if (new_id == 0) {
		RETURN_FIRST_ID();
	}

	LOG_DEBUG(MODULE_TAG, " next record id - %lu\n", new_id);

	return new_id;
}
