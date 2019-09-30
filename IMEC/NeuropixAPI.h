/*
	Neuropixel c API

	(c) Imec 2019

*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif	

#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>

#define NP_EXPORT __declspec(dllexport)
#define NP_APIC __stdcall

//#define PROBE_ELECTRODE_COUNT 960
#define PROBE_CHANNEL_COUNT   384
#define PROBE_SUPERFRAMESIZE  12
#define PROBE_ADC_COUNT       32

#define ELECTRODEPACKET_STATUS_TRIGGER    (1<<0)
#define ELECTRODEPACKET_STATUS_SYNC       (1<<6)

#define ELECTRODEPACKET_STATUS_LFP        (1<<1)
#define ELECTRODEPACKET_STATUS_ERR_COUNT  (1<<2)
#define ELECTRODEPACKET_STATUS_ERR_SERDES (1<<3)
#define ELECTRODEPACKET_STATUS_ERR_LOCK   (1<<4)
#define ELECTRODEPACKET_STATUS_ERR_POP    (1<<5)
#define ELECTRODEPACKET_STATUS_ERR_SYNC   (1<<7)


	typedef enum ElectrodeBanks {
		None = 0,
		BankA = (1 << 0),
		BankB = (1 << 1),
		BankC = (1 << 2),
		BankD = (1 << 3),
	}electrodebanks_t;

	typedef enum StreamSource {
		SourceAP = 0,
		SourceLFP = 1
	}streamsource_t;

	struct PacketInfo {
		uint32_t Timestamp;
		uint16_t Status;
		uint16_t payloadlength;
	};

	struct electrodePacket {
		uint32_t timestamp[PROBE_SUPERFRAMESIZE];
		int16_t apData[PROBE_SUPERFRAMESIZE][PROBE_CHANNEL_COUNT];
		int16_t lfpData[PROBE_CHANNEL_COUNT];
		uint16_t Status[PROBE_SUPERFRAMESIZE];
	};

	typedef struct {
		uint32_t MAGIC; // includes 'Type' field as lower 4 bits

		uint16_t samplecount;
		uint8_t seqnr;
		uint8_t format;

		uint32_t timestamp;

		uint8_t status;
		uint8_t sourceid;
		uint16_t crc;

	}pckhdr_t;

	#define NP_MAXPAYLOADSIZE      1024
	typedef struct {
		pckhdr_t hdr;
		uint8_t payload[NP_MAXPAYLOADSIZE];
	}np_packet_t;

	typedef void(NP_APIC *np_packetcallbackfn_t)(const np_packet_t& packet, const void* userdata);
	typedef void* npcallbackhandle_t;

	struct ADC_Calib {
		int CompP;
		int CompN;
		int Slope;
		int Coarse;
		int Fine;
		int Cfix;
		int offset;
		int threshold;
	};

	/**
	* Neuropix API error codes
	*/
	typedef enum {

		SUCCESS = 0,/**< The function returned sucessfully */
		FAILED = 1, /**< Unspecified failure */
		ALREADY_OPEN = 2,/**< A board was already open */
		NOT_OPEN = 3,/**< The function cannot execute, because the board or port is not open */
		IIC_ERROR = 4,/**< An error occurred while accessing devices on the BS i2c bus */
		VERSION_MISMATCH = 5,/**< FPGA firmware version mismatch */
		PARAMETER_INVALID = 6,/**< A parameter had an illegal value or out of range */
		UART_ACK_ERROR = 7,/**< uart communication on the serdes link failed to receive an acknowledgement */
		TIMEOUT = 8,/**< the function did not complete within a restricted period of time */
		WRONG_CHANNEL = 9,/**< illegal channel number */
		WRONG_BANK = 10,/**< illegal electrode bank number */
		WRONG_REF = 11,/**< a reference number outside the valid range was specified */
		WRONG_INTREF = 12,/**< an internal reference number outside the valid range was specified */
		CSV_READ_ERROR = 13,/**< an parsing error occurred while reading a malformed CSV file. */
		BIST_ERROR = 14,/**< a BIST operation has failed */
		FILE_OPEN_ERROR = 15,/**< The file could not be opened */
		READBACK_ERROR = 16,/**< a BIST readback verification failed */
		READBACK_ERROR_FLEX = 17,/**< a BIST Flex EEPROM readback verification failed */
		READBACK_ERROR_HS = 18,/**< a BIST HS EEPROM readback verification failed */
		READBACK_ERROR_BSC = 19,/**< a BIST HS EEPROM readback verification failed */
		TIMESTAMPNOTFOUND = 20,/**< the specified timestamp could not be found in the stream */
		FILE_IO_ERR = 21,/**< a file IO operation failed */
		OUTOFMEMORY = 22,/**< the operation could not complete due to insufficient process memory */
		LINK_IO_ERROR = 23,/**< serdes link IO error */
		NO_LOCK = 24,/**< missing serializer clock. Probably bad cable or connection */
		WRONG_AP = 25,/**< AP gain number out of range */
		WRONG_LFP = 26,/**< LFP gain number out of range */
		ERROR_SR_CHAIN_1 = 27,/**< Validation of SRChain1 data upload failed */
		ERROR_SR_CHAIN_2 = 28,/**< Validation of SRChain2 data upload failed */
		ERROR_SR_CHAIN_3 = 29,/**< Validation of SRChain3 data upload failed */
		PCIE_IO_ERROR = 30,/**< a PCIe data stream IO error occurred. */
		NO_SLOT = 31,/**< no Neuropix board found at the specified slot number */
		WRONG_SLOT = 32,/**<  the specified slot is out of bound */
		WRONG_PORT = 33,/**<  the specified port is out of bound */
		STREAM_EOF = 34,/**<  The stream is at the end of the file, but more data was expected*/
		HDRERR_MAGIC = 35, /**< The packet header is corrupt and cannot be decoded */
		HDRERR_CRC = 36, /**< The packet header's crc is invalid */
		WRONG_PROBESN = 37, /**< The probe serial number does not match the calibration data */
		PROGRAMMINGABORTED = 39, /**<  the flash programming was aborted */
		VALUE_INVALID = 40, /**<  The parameter value is invalid */
		WRONG_DOCK_ID = 41, /**<  the specified probe id is out of bound */
		INVALID_ARGUMENT = 42,
		NO_BSCONNECT = 43, /**< no base station connect board was found */
		NO_HEADSTAGE = 44, /**< no head stage was detected */
		NO_FLEX = 45, /**< no flex board was detected */
		NO_PROBE = 46, /**< no probe was detected */
		WRONG_ADC = 47, /**< the calibration data contains a wrong ADC identifier */
		WRONG_SHANK = 48, /**< the shank parameter was out of bound */
		UNKNOWN_STREAMSOURCE = 50, /**< the streamsource parameter is unknown */
		ILLEGAL_HANDLE = 51, /**< the value of the 'handle' parameter is not valid. */
		OBJECT_MISMATCH = 52, /**< the object type is not of the expected class */
		NOTSUPPORTED = 0xFE,/**<  the function is not supported */
		NOTIMPLEMENTED = 0xFF/**<  the function is not implemented */
	}NP_ErrorCode;

	/**
	* Operating mode of the probe
	*/
	typedef enum {
		RECORDING = 0, /**< Recording mode: (default) pixels connected to channels */
		CALIBRATION = 1, /**< Calibration mode: test signal input connected to pixel, channel or ADC input */
		DIGITAL_TEST = 2, /**< Digital test mode: data transmitted over the PSB bus is a fixed data pattern */
	}probe_opmode_t;

	/**
	* Test input mode
	*/
	typedef enum {
		PIXEL_MODE = 0, /**< HS test signal is connected to the pixel inputs */
		CHANNEL_MODE = 1, /**< HS test signal is connected to channel inputs */
		NO_TEST_MODE = 2, /**< no test mode */
		ADC_MODE = 3, /**< HS test signal is connected to the ADC inputs */
	}testinputmode_t;

	typedef enum {
		EXT_REF = 0,  /**< External electrode */
		TIP_REF = 1,  /**< Tip electrode */
		INT_REF = 2,   /**< Internal electrode */
		NONE_REF = 0xFF /**< disconnect reference */
	}channelreference_t;

	typedef enum {
		EMUL_OFF = 0, /**< No emulation data is generated */
		EMUL_STATIC = 1, /**< static data per channel: value = channel number */
		EMUL_LINEAR = 2, /**< a linear ramp is generated per channel (1 sample shift between channels) */
	}emulatormode_t;

	typedef enum {
		SIGNALLINE_NONE = 0,
		SIGNALLINE_PXI0 = (1 << 0),
		SIGNALLINE_PXI1 = (1 << 1),
		SIGNALLINE_PXI2 = (1 << 2),
		SIGNALLINE_PXI3 = (1 << 3),
		SIGNALLINE_PXI4 = (1 << 4),
		SIGNALLINE_PXI5 = (1 << 5),
		SIGNALLINE_PXI6 = (1 << 6),
		SIGNALLINE_SHAREDSYNC = (1 << 7),
		SIGNALLINE_LOCALTRIGGER = (1 << 8),
		SIGNALLINE_LOCALSYNC = (1 << 9),
		SIGNALLINE_SMA = (1 << 10),
		SIGNALLINE_SW = (1 << 11),
		SIGNALLINE_LOCALSYNCCLOCK = (1 << 12)
	}signalline_t;



	typedef void* np_streamhandle_t;

	struct np_sourcestats {
		uint32_t timestamp;
		uint32_t packetcount;
		uint32_t samplecount;
		uint32_t fifooverflow;
	};

	struct np_diagstats {
		uint64_t totalbytes;         /**< total amount of bytes received */
		uint32_t packetcount;        /**< Amount of packets received */
		uint32_t triggers;           /**< Amount of triggers received */
		uint32_t err_badmagic;		 /**< amount of packet header bad MAGIC markers */
		uint32_t err_badcrc;		 /**< amount of packet header CRC errors */
		uint32_t err_droppedframes;	 /**< amount of dropped frames in the stream */
		uint32_t err_count;			 /**< Every psb frame has an incrementing count index. If the received frame count value is not as expected possible data loss has occured and this flag is raised. */
		uint32_t err_serdes;		 /**< incremented if a deserializer error (hardware pin) occured during receiption of this frame this flag is raised */
		uint32_t err_lock;			 /**< incremented if a deserializer loss of lock (hardware pin) occured during receiption of this frame this flag is raised */
		uint32_t err_pop;			 /**< incremented whenever the ‘next blocknummer’ round-robin FiFo is flagged empty during request of the next value (for debug purpose only, irrelevant for end-user software) */
		uint32_t err_sync;			 /**< Front-end receivers are out of sync. => frame is invalid. */
	};
	
	/********************* Parameter configuration functions ****************************/

	typedef enum {
		NP_DATAMODE_ELECTRODE = 0,
		NP_DATAMODE_ADC = 1
	}np_datamode_t;

	typedef enum {
		NP_PARAM_BUFFERSIZE = 1,
		NP_PARAM_BUFFERCOUNT = 2,
		NP_PARAM_SYNCMASTER = 3,
		NP_PARAM_SYNCFREQUENCY_HZ = 4,
		NP_PARAM_SYNCPERIOD_MS = 5,
		NP_PARAM_SYNCSOURCE = 6,
		NP_PARAM_SIGNALINVERT = 7,
	}np_parameter_t;


	/* additional functions */
	NP_EXPORT NP_ErrorCode NP_APIC BSC_getFWInfo(int slotID, struct fwversioninfo* info);
	NP_EXPORT NP_ErrorCode NP_APIC BS_getFWInfo(int slotID, struct fwversioninfo* info);


	NP_EXPORT void NP_APIC getAPIVersion(uint8_t* version_major, uint8_t* version_minor);
	NP_EXPORT const char* NP_APIC np_GetErrorMessage(NP_ErrorCode code);
	NP_EXPORT NP_ErrorCode NP_APIC getAvailableSlots(uint32_t* slotmask);

	/* System / Parameter configuration functions *****************************************/
	NP_EXPORT NP_ErrorCode NP_APIC setParameter       (np_parameter_t paramid, int value);
	NP_EXPORT NP_ErrorCode NP_APIC getParameter       (np_parameter_t paramid, int* value);
	NP_EXPORT NP_ErrorCode NP_APIC setParameter_double(np_parameter_t paramid, double value);
	NP_EXPORT NP_ErrorCode NP_APIC getParameter_double(np_parameter_t paramid, double* value);

	/* Base station board/Slot Functions **************************************************/
	NP_EXPORT NP_ErrorCode NP_APIC openBS           (int slotID);
	NP_EXPORT NP_ErrorCode NP_APIC closeBS          (int slotID);
	NP_EXPORT NP_ErrorCode NP_APIC arm              (int slotID);
	NP_EXPORT NP_ErrorCode NP_APIC setSWTrigger     (int slotID);
	NP_EXPORT NP_ErrorCode NP_APIC setTriggerEdge   (int slotID, bool rising);
	NP_EXPORT NP_ErrorCode NP_APIC setTriggerBinding(int slotID, signalline_t outputlines, signalline_t inputlines);
	NP_EXPORT NP_ErrorCode NP_APIC getTriggerBinding(int slotID, signalline_t outputlines, signalline_t* inputlines);
	NP_EXPORT NP_ErrorCode NP_APIC setDataMode      (int slotID, np_datamode_t mode);
	NP_EXPORT NP_ErrorCode NP_APIC getDataMode      (int slotID, np_datamode_t* mode);
	NP_EXPORT NP_ErrorCode NP_APIC writeBSCMM       (int slotID, uint32_t address, uint32_t data);
	NP_EXPORT NP_ErrorCode NP_APIC readBSCMM        (int slotID, uint32_t address, uint32_t* data);
	NP_EXPORT NP_ErrorCode NP_APIC getBSCBootVersion(int slotID, uint8_t* version_major, uint8_t* version_minor, uint16_t* version_build);
	NP_EXPORT NP_ErrorCode NP_APIC getBSBootVersion (int slotID, uint8_t* version_major, uint8_t* version_minor, uint16_t* version_build);
	NP_EXPORT NP_ErrorCode NP_APIC getBSTemperature (int slotID, float* temperature);
	NP_EXPORT NP_ErrorCode NP_APIC getBSCTemperature(int slotID, float* temperature);

	/*** File stream API ******/
	NP_EXPORT NP_ErrorCode NP_APIC setFileStream   (int slotID, const char* filename);
	NP_EXPORT NP_ErrorCode NP_APIC enableFileStream(int slotID, bool enable);

	/********************* Remote update ****************************/
	NP_EXPORT NP_ErrorCode NP_APIC bs_update     (int slotID, const char* filename, int(*callback)(size_t byteswritten));
	NP_EXPORT NP_ErrorCode NP_APIC bsc_update    (int slotID, const char* filename, int(*callback)(size_t byteswritten));

	/* Base station connect Port functions ***********************************************/
	NP_EXPORT NP_ErrorCode NP_APIC setLog(int slotID, int portID, bool enable);

	/*
	* \brief Read the last error message
	*
	* @param bufStart: destination buffer
	* @param bufsize: size of the destination buffer
	* @returns amount of characters written to the destination buffer
	*/
	NP_EXPORT size_t NP_APIC getLastErrorMessage(char* bufStart, size_t bufsize);

	/**
	* @brief Get the amount of ports on the base station connect board
	* @param slotID: Geographic slot ID
	* @param count: output parameter, amount of available ports
	* @returns SUCCESS if successful.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC getBSCSupportedPortCount(int slotID, int* count);
	/**
	* @brief Get the amount of probes the connected headstage supports
	* @param slotID: Geographic slot ID
	* @param portID: portID (1..4)
	* @param count: output parameter, amount of flex/probes
	* @returns SUCCESS if successful.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC getHSSupportedProbeCount(int slotID, int portID, int* count);

	NP_EXPORT NP_ErrorCode NP_APIC openPort      (int slotID, int portID);
	NP_EXPORT NP_ErrorCode NP_APIC closePort     (int slotID, int portID);
	NP_EXPORT NP_ErrorCode NP_APIC setHSLed      (int slotID, int portID, bool enable);
	NP_EXPORT NP_ErrorCode NP_APIC writeI2C      (int slotID, int portID, uint8_t device, uint8_t address, uint8_t data);
	NP_EXPORT NP_ErrorCode NP_APIC readI2C       (int slotID, int portID, uint8_t device, uint8_t address, uint8_t* data);
	NP_EXPORT NP_ErrorCode NP_APIC writeI2Cflex  (int slotID, int portID, int dockID, uint8_t device, uint8_t address, uint8_t data);
	NP_EXPORT NP_ErrorCode NP_APIC readI2Cflex   (int slotID, int portID, int dockID, uint8_t device, uint8_t address, uint8_t* data);
	NP_EXPORT NP_ErrorCode NP_APIC getFlexVersion(int slotID, int portID, int dockID, unsigned char* version_major, unsigned char* version_minor);

	NP_EXPORT NP_ErrorCode NP_APIC readFlexPN    (int slotID, int portID, int dockID, char* pn, size_t maxlen);
	NP_EXPORT NP_ErrorCode NP_APIC readHSPN      (int slotID, int portID, char* pn, size_t maxlen);
	NP_EXPORT NP_ErrorCode NP_APIC readHSSN      (int slotID, int portID, uint64_t* sn);
	NP_EXPORT NP_ErrorCode NP_APIC getHSVersion  (int slotID, int portID, uint8_t* version_major, uint8_t* version_minor);

	NP_EXPORT NP_ErrorCode NP_APIC readBSCPN     (int slotID, char* pn, size_t len);
	NP_EXPORT NP_ErrorCode NP_APIC readBSCSN     (int slotID, uint64_t* sn);
	NP_EXPORT NP_ErrorCode NP_APIC getBSCVersion (int slotID, uint8_t* version_major, uint8_t* version_minor);


	/* Probe functions *******************************************************************/
	NP_EXPORT NP_ErrorCode NP_APIC openProbe              (int slotID, int portID, int dockID);
	NP_EXPORT NP_ErrorCode NP_APIC init                   (int slotID, int portID, int dockID);
	NP_EXPORT NP_ErrorCode NP_APIC writeProbeConfiguration(int slotID, int portID, int dockID, bool readCheck);
	NP_EXPORT NP_ErrorCode NP_APIC setADCCalibration      (int slotID, int portID, const char* filename);
	NP_EXPORT NP_ErrorCode NP_APIC setGainCalibration     (int slotID, int portID, int dockID, const char* filename);

	// only supported for NP2
	NP_EXPORT NP_ErrorCode NP_APIC readElectrodeData      (int slotID, int portID, int dockID, struct electrodePacket* packets, size_t* actualAmount, size_t requestedAmount);
	// only supported for NP2
	NP_EXPORT NP_ErrorCode NP_APIC getElectrodeDataFifoState(int slotID, int portID, int dockID, size_t* packetsavailable, size_t* headroom);

	NP_EXPORT NP_ErrorCode NP_APIC setTestSignal          (int slotID, int portID, int dockID, bool enable);
	NP_EXPORT NP_ErrorCode NP_APIC setOPMODE              (int slotID, int portID, int dockID, probe_opmode_t mode);
	NP_EXPORT NP_ErrorCode NP_APIC setCALMODE             (int slotID, int portID, int dockID, testinputmode_t mode);
	NP_EXPORT NP_ErrorCode NP_APIC setREC_NRESET          (int slotID, int portID, bool state);
	NP_EXPORT NP_ErrorCode NP_APIC readProbeSN            (int slotID, int portID, int dockID, uint64_t* id);
	NP_EXPORT NP_ErrorCode NP_APIC readProbePN            (int slotID, int portID, int dockID, char* pn, size_t maxlen);

	
	/**
	* @brief Read a single packet data from the specified fifo.
	*        This is a non blocking function that tries to read a single packet
	*        from the specified receive fifo.
	* @param slotID: Geographic slot ID
	* @param portID: portID (1..4)
	* @param dock: probe index (0..1 (for NPM))
	* @param source: Select the stream source from the probe (SourceAP or SourceLFP). Note that NPM does not support LFP source 
	* @param pckinfo: output data containing additional packet data: timestamp, stream status, and payload length
	* @param data: unpacked 16 bit right aligned data 
	* @param requestedChannelCount: size of data buffer (maximum amount of channels)
	* @param actualread: optional output parameter that returns the amount of channels unpacked for a single timestamp.
	* @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0)
	*/
	NP_EXPORT NP_ErrorCode NP_APIC readPacket(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pckinfo, int16_t* data, size_t requestedChannelCount, size_t* actualread);

	/**
	* @brief Read multiple packets from the specified fifo.
	*        This is a non blocking function.
	* @param slotID: Geographic slot ID
	* @param portID: portID (1..4)
	* @param dock: probe index (0..1 (for NPM))
	* @param source: Select the stream source from the probe (SourceAP or SourceLFP). Note that NPM does not support LFP source
	* @param pckinfo: output data containing additional packet data: timestamp, stream status, and payload length. 
	*                 size of this buffer is expected to be sizeof(struct PacketInfo)*packetcount
	* @param data: unpacked 16 bit right aligned data. size of this buffer is expected to be 'channelcount*packetcount*sizeof(int16_t)'
	* @param channelcount: amount of channels to read per packet. This value is also the data stride value in the result 'data' buffer.
	* @param packetcount: amount of channels to read per packet. This value is also the data stride value in the result 'data' buffer.
	* @param packetsread: amount of packets read from the fifo.
	* @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0)
	*/
	NP_EXPORT NP_ErrorCode NP_APIC readPackets(
		int slotID,
		int portID,
		int dockID,
		streamsource_t source,
		struct PacketInfo* pckinfo,
		int16_t* data,
		size_t channelcount,
		size_t packetcount,
		size_t* packetsread);

	NP_EXPORT NP_ErrorCode NP_APIC getPacketFifoStatus(int slotID, int portID, int dockID, streamsource_t source, size_t* packetsavailable, size_t* headroom);
	
	/**
	* @brief Create a slot packet receive callback function 
	*        the callback function will be called for each packet received on the specified slot.
	*        Use 'destroyPacketCallback' to unbind from the packet receive handler.
	*        Note: multiple callbacks may be registered to the same slot.
	* @param slotID: geographic slot address
	* @param handle: output parameter that will contain the callback handle.
	* @param callback: user callback function that will be associated with the output handle
	* @param userdata: optional user data that will be supplied to the callback function.
	* @returns SUCCESS if successful
	*/
	NP_EXPORT NP_ErrorCode NP_APIC createSlotPacketCallback  (int slotID, npcallbackhandle_t* handle, np_packetcallbackfn_t callback, const void* userdata);
	/**
	* @brief Create a probe packet receive callback function
	*        the callback function will be called for each packet received on the specified probe/streamsource combination.
	*        Use 'destroyPacketCallback' to unbind from the packet receive handler.
	*        Note: multiple callbacks may be registered to the same probe.
	* @param slotID: geographic slot address
	* @param portID: port ID (1..4)
	* @param dock: the targetted probe (0..1 - only for NPM)
	* @param handle: output parameter that will contain the callback handle.
	* @param callback: user callback function that will be associated with the output handle
	* @param userdata: optional user data that will be supplied to the callback function.
	* @returns SUCCESS if successful
	*/
	NP_EXPORT NP_ErrorCode NP_APIC createProbePacketCallback (int slotID, int portID, int dockID, streamsource_t source, npcallbackhandle_t* handle, np_packetcallbackfn_t callback, const void* userdata);
	/**
	* @brief Unbind a callback from the packet receive handler.
	*        This function stops the receive callback associated with the argument callback handle.
	* @param handle: the handle, created with 'createSlotPacketCallback' or 'createProbePacketCallback'
	* @returns SUCCESS if successful
	*/
	NP_EXPORT NP_ErrorCode NP_APIC destroyPacketCallback     (npcallbackhandle_t* handle);

	/**
	* @brief Unpacks samples from a raw data packet frame.
	* @param packet: the packet to unpack data from. This function will inspect the packet's header
	*                to determine the packing format and data alignment.
	* @param output: output buffer.
	* @param samplestoread: maximum amount of elements to dat.
	* @param actualread: output argument;returns amount of samples succesfully unpacked into 'output' buffer
	* @returns SUCCESS
	*/
	NP_EXPORT NP_ErrorCode NP_APIC unpackData(const np_packet_t* packet, int16_t* output, size_t samplestoread, size_t* actualread);
		
	/* Probe Channel functions ***********************************************************/
	NP_EXPORT NP_ErrorCode NP_APIC setGain                (int slotID, int portID, int dockID, int channel, int ap_gain, int lfp_gain);
	NP_EXPORT NP_ErrorCode NP_APIC getGain                (int slotID, int portID, int dockID, int channel, int* APgainselect, int* LFPgainselect);
	NP_EXPORT NP_ErrorCode NP_APIC selectElectrode        (int slotID, int portID, int dockID, int channel, int shank, int bank);
	NP_EXPORT NP_ErrorCode NP_APIC selectElectrodeMask    (int slotID, int portID, int dockID, int channel, int shank, electrodebanks_t bankmask);
	NP_EXPORT NP_ErrorCode NP_APIC setReference           (int slotID, int portID, int dockID, int channel, int shank, channelreference_t reference, int intRefElectrodeBank);
	NP_EXPORT NP_ErrorCode NP_APIC setAPCornerFrequency   (int slotID, int portID, int dockID, int channel, bool disableHighPass);
	NP_EXPORT NP_ErrorCode NP_APIC setStdb                (int slotID, int portID, int dockID, int channel, bool standby);

	/********************* Built In Self Test ****************************/
	/**
	* @brief Basestation platform BIST
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the chassis range is entered.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistBS(int slotID);

	/**
	* @brief Head Stage heartbeat test
	* The heartbeat signal generated by the PSB_SYNC signal of the probe. The PSB_SYNC signal starts when the probe is powered on, the OP_MODE register in the probes’ memory map set to 1, and the REC_NRESET signal set high.
	* The heartbeat signal is visible on the headstage (can be disabled by API functions) and on the BSC. This is in the first place a visual check.
	* In order to facilitate a software check of the BSC heartbeat signal, the PSB_SYNC signal is also routed to the BS FPGA. A function is provided to check whether the PSB_SYNC signal contains a 0.5Hz clock.
	* The presence of a heartbeat signal acknowledges the functionality of the power supplies on the headstage for serializer and probe, the POR signal, the presence of the master clock signal on the probe, the functionality of the clock divider on the probe, an basic communication over the serdes link.
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistHB(int slotID, int portID, int dockID);

	/**
	* @brief Start Serdes PRBS test
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistStartPRBS(int slotID, int portID);

	/**
	* @brief Stop Serdes PRBS test
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @param prbs_err: the number of prbs errors
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistStopPRBS(int slotID, int portID, int* prbs_err);

	/**
	* @brief Test I2C memory map access
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered, NO_ACK in case no acknowledgment is received, READBACK_ERROR in case the written and readback word are not the same.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistI2CMM(int slotID, int portID, int dockID);

	/**
	* @brief Test all EEPROMs (Flex, headstage, BSC). by verifying a write/readback cycle on an unused memory location
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered, NO_ACK_FLEX in case no acknowledgment is received from the flex eeprom, READBACK_ERROR_FLEX in case the written and readback word are not the same from the flex eeprom, NO_ACK_HS in case no acknowledgment is received from the HS eeprom, READBACK_ERROR_HS in case the written and readback word are not the same from the HS eeprom, NO_ACK_BSC in case no acknowledgment is received from the BSC eeprom, READBACK_ERROR_BSC in case the written and readback word are not the same from the BSC eeprom.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistEEPROM(int slotID, int portID);


	/**
	* @brief Test the shift registers
	* This test verifies the functionality of the shank and base shift registers (SR_CHAIN 1 to 3). The function configures the shift register two times with the same code. After the 2nd write cycle the SR_OUT_OK bit in the STATUS register is read. If OK, the shift register configuration was successful. The test is done for all 3 registers. The configuration code used for the test is a dedicated code (to make sure the bits are not all 0 or 1).
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered, ERROR_SR_CHAIN_1 in case the SR_OUT_OK bit is not ok when writing SR_CHAIN_1, ERROR_SR_CHAIN_2 in case the SR_OUT_OK bit is not ok when writing SR_CHAIN_2, ERROR_SR_CHAIN_3 in case the SR_OUT_OK bit is not ok when writing SR_CHAIN_3.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistSR(int slotID, int portID, int dockID);

	/**
	* @brief Test the PSB bus on the headstage
	* A test mode is implemented on the probe which enables the transmission of a known data pattern over the PSB bus. The following function sets the probe in this test mode, records a small data set, and verifies whether the acquired data matches the known data pattern.
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistPSB(int slotID, int portID, int dockID);

	/**
	* @brief The probe is configured for noise analysis. Via the shank and base configuration registers and the memory map, the electrode inputs are shorted to ground. The data signal is recorded and the noise level is calculated. The function analyses if the probe performance falls inside a specified tolerance range (go/no-go test).
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param port: for which HS (valid range 1 to 4)
	* @returns SUCCESS if successful, BIST_ERROR of test failed. NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
	*/
	NP_EXPORT NP_ErrorCode NP_APIC bistNoise(int slotID, int portID, int dockID);

	/********************* Data Acquisition ****************************/


	/**
	* @brief Open an acquisition stream from an existing file.
	* @param filename Specifies an existing file with probe acquisition data.
	* @param port specifies the target port (1..4)
	* @param dockID 1..2 (depending on type of probe. default=1)
	* @param source data type (AP or LFP) if supported (default = AP)
	* @param psh stream a pointer to the stream pointer that will receive the handle to the opened stream
	* @returns FILE_OPEN_ERROR if unable to open file
	*/
	NP_EXPORT NP_ErrorCode NP_APIC streamOpenFile(const char* filename, int portID, int dockID, streamsource_t source, np_streamhandle_t* pstream);

	/**
	* @brief Closes an acquisition stream.
	* Closes the stream along with the optional recording file.
	* @returns (TBD)
	*/
	NP_EXPORT NP_ErrorCode NP_APIC streamClose(np_streamhandle_t sh);

	/**
	* @brief Moves the stream pointer to given timestamp.
	* Stream seek is only supported on streams that are backed by a recording file store.
	* @param stream: the acquisition stream handle
	* @param filepos: The file position to navigate to.
	* @param actualtimestamp: returns the timestamp at the stream pointer (NULL allowed)
	* @returns TIMESTAMPNOTFOUND if no valid data packet is found beyond the specified file position
	*/
	NP_EXPORT NP_ErrorCode NP_APIC streamSeek(np_streamhandle_t sh, uint64_t filepos, uint32_t* actualtimestamp);

	NP_EXPORT NP_ErrorCode NP_APIC streamSetPos(np_streamhandle_t sh, uint64_t filepos);

	/**
	* @brief Report the current file position in the filestream.
	* @param stream: the acquisition stream handle
	* @returns the current file position at the stream cursor position.
	*/
	NP_EXPORT uint64_t NP_APIC streamTell(np_streamhandle_t sh);

	/**
	* @brief read probe data from a recorded file stream.
	* Example:
	*    #define SAMPLECOUNT 128
	*    uint16_t interleaveddata[SAMPLECOUNT * 384];
	*    uint32_t timestamps[SAMPLECOUNT];
	*
	*    np_streamhandle_t sh;
	*    streamOpenFile("myrecording.bin",1, false, &sh);
	*    size_t actualread;
	*    streamRead(sh, timestamps, interleaveddata, SAMPLECOUNT, &actualread);
	*
	* @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
	* @param timestamps: Optional timestamps buffer (NULL if not used). size should be 'samplecount'
	* @param data: buffer of size samplecount*384. The buffer will be populated with channel interleaved, 16 bit per sample data.
	* @param samplestoread: amount of timestamps to read.
	* @param actualread: output parameter: amount of 16 timestamps actually read from the stream.
	* @returns SUCCESS if succesfully read any sample from the stream
	*/
	NP_EXPORT NP_ErrorCode NP_APIC streamRead(
		np_streamhandle_t sh,
		uint32_t* timestamps,
		int16_t* data,
		size_t samplestoread,
		size_t* actualread);

	NP_EXPORT NP_ErrorCode NP_APIC streamReadPacket(
		np_streamhandle_t sh,
		pckhdr_t* header,
		int16_t* data,
		size_t samplestoread,
		size_t* actualread);


#ifdef __cplusplus
}
#endif	
