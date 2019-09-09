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

// A fatal error occurred, the system must be reset completely to proceed
#define DBG_FATAL    0
// An error occurred, the function could not complete its operation
#define DBG_ERROR    1
// A condition occurred that may indicate a unintended use or configuration. 
// The function will still run/resume as normal
#define DBG_WARNING  2
// Additional runtime information is printed to the log.
// (example: status information during BIST operation)
#define DBG_INFO     3
// More detailed debug-specific log information is printed
#define DBG_DEBUG    4
// Print all available log statements
#define DBG_VERBOSE  5

	NP_EXPORT void NP_APIC dbg_setlevel(int level);
	NP_EXPORT int NP_APIC dbg_getlevel(void);
	NP_EXPORT void NP_APIC dbg_getversion_datetime(char* dst, size_t maxlen);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_getEmulatorMode(int slotID, emulatormode_t* mode);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_setProbeEmulationMode(int slotID, int portID, int dockID, bool state);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_getProbeEmulationMode(int slotID, int portID, int dockID, bool* state);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_stats_reset(int slotID);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_diagstats_read(int slotID, struct np_diagstats* stats);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_sourcestats_read(int slotID, uint8_t sourceID, struct np_sourcestats* stats);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_disablescale(int slotID, bool state);
	NP_EXPORT NP_ErrorCode NP_APIC dbg_read_srchain(int slotID, int portID, int dockID, byte SRChain_registeraddress, byte* dst, size_t len, size_t* actualread);

#ifdef __cplusplus
}
#endif	