/*
	Neuropixel

	(c) Imec 2019

	Debug API exports
*/

#pragma once
#include "NeuropixAPI.h"

#ifdef __cplusplus
extern "C" {
#endif	


#define HARDWAREID_PN_LEN  40
#pragma pack(push, 1)
	struct HardwareID {
		uint8_t version_Major;
		uint8_t version_Minor;
		uint64_t SerialNumber;
		char ProductNumber[HARDWAREID_PN_LEN];
	};
#pragma pack(pop)

	NP_EXPORT NP_ErrorCode NP_APIC getBSCHardwareID(int slotID, struct HardwareID* pHwid);
	NP_EXPORT NP_ErrorCode NP_APIC setBSCHardwareID(int slotID, const struct HardwareID* pHwid);
	NP_EXPORT NP_ErrorCode NP_APIC getHeadstageHardwareID(int slotID, int portID, struct HardwareID* pHwid);
	NP_EXPORT NP_ErrorCode NP_APIC setHeadstageHardwareID(int slotID, int portID, const struct HardwareID* pHwid);
	NP_EXPORT NP_ErrorCode NP_APIC getFlexHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
	NP_EXPORT NP_ErrorCode NP_APIC setFlexHardwareID(int slotID, int portID, int dockID, const struct HardwareID* pHwid);
	NP_EXPORT NP_ErrorCode NP_APIC getProbeHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
	NP_EXPORT NP_ErrorCode NP_APIC setProbeHardwareID(int slotID, int portID, int dockID, const struct HardwareID* pHwid);

#ifdef __cplusplus
}
#endif	