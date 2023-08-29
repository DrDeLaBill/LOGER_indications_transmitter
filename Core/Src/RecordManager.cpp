#include "RecordManager.h"

#include <string.h>

#include "utils.h"
#include "storage_data_manager.h"


typedef RecordManager RM;


const char* RM::MODULE_TAG = "RCRD";

RM::record_t RM::record    = { 0 };


RM::RecordManager()
{
	memset((uint8_t*)&RM::record, 0, sizeof(RM::record));
}

RM::record_status_t RM::load(uint32_t record_id)
{

	return RECORD_ERROR;
}

RM::record_status_t RM::save()
{

	return RECORD_ERROR;
}

RM::record_status_t RM::get_new_record_id(uint32_t* new_id)
{
#if RECORD_BEDUG
	LOG_TAG_BEDUG(MODULE_TAG, "get new log id: begin\n");
#endif
#if RECORD_BEDUG
	LOG_TAG_BEDUG(MODULE_TAG, "get new log id: error\n");
#endif
	return RECORD_ERROR;
}
