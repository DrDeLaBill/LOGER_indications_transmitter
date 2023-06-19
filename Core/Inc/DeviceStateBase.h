#ifndef INC_DEVICESTATEBASE_H
#define INC_DEVICESTATEBASE_H


class DeviceStateBase {

protected:
	const static char* MODULE_TAG;

	void error();

public:
	DeviceStateBase();
	virtual void proccess();
	virtual void sleep();
	virtual void awake();

};


#endif
