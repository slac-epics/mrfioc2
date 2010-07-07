#ifndef EVGMXC_H
#define EVGMXC_H

#include <epicsTypes.h>   

class evgMxc {
public:
	evgMxc(const epicsUInt32, volatile epicsUInt8* const);
	~evgMxc();

	bool getMxcOutStatus();

	bool getMxcOutPolarity();
	epicsStatus setMxcOutPolarity(bool);

	epicsUInt32 getMxcPrescaler();
	epicsStatus setMxcPrescaler(epicsUInt32);
	
	epicsUInt8 getMxcTrigEvtMap();
	epicsStatus setMxcTrigEvtMap(epicsUInt16);

private:
	const epicsUInt32 			m_id;
	volatile epicsUInt8* const	m_pReg;
};

#endif//EVGMXC_H