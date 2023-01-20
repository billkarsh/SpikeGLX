/*****************************************************************
 * @file   NeuropixAPI.h
 * @brief  Neuropixel c/c++ API header
 *
 * (c) Imec 2021
 *****************************************************************/
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NP_EXPORT __declspec(dllexport)
#define NP_CALLBACK __stdcall

/**
 * @brief Main neuropixels API namespace. All external functions are included in this namespace.
 *
 */
namespace Neuropixels {

    /** Flag set of hardware basestation selection. */
    typedef enum
    {
        /** Do not select any, or none are selected */
        NPPlatform_None = 0,
        /** PXI (PCIe) neuropixels basestation */
        NPPlatform_PXI = 0x1,
        /** USB (Onebox) neuropixels basestation */
        NPPlatform_USB = 0x2,
        /** Any known neuropixel basestation */
        NPPlatform_ALL = NPPlatform_PXI | NPPlatform_USB,
    }NPPlatformID_t;

    /** Unique basestation identification.
     *
     * This struct is a return value only
     * The combination of \c platformid and \c ID form a unique tuple to identify a base station
    */
    struct basestationID
    {
        /** Type of basestation platform. Can be USB/PXI */
        NPPlatformID_t platformid;
        /** Unique ID for the specific platform. Each platform has its own  */
        int   ID;
    };

    struct firmware_Info
    {
        int major;
        int minor;
        int build;
        char name[64];
    };

#pragma pack(push, 1)
    typedef struct {
        uint32_t MAGIC; ///> includes 'Type' field as lower 4 bits

        uint16_t samplecount;
        uint8_t sessionID_seqnr; ///> bXXXXYYYY (XXXX = sessionID, YYYY = sequence number per sourceID)
        uint8_t format;

        uint32_t timestamp;

        uint8_t status;
        uint8_t sourceid;
        uint16_t crc;

    }pckhdr_t;
#pragma pack(pop)

#define NP_MAXPAYLOADSIZE      1024
#pragma pack(push, 1)
    typedef struct {
        pckhdr_t hdr;
        uint8_t payload[NP_MAXPAYLOADSIZE];
    }np_packet_t;
#pragma pack(pop)


    typedef void(NP_CALLBACK* np_packetcallbackfn_t)(const np_packet_t& packet, const void* userdata);


    typedef enum {
        None = 0,
        BankA = (1 << 0),
        BankB = (1 << 1),
        BankC = (1 << 2),
        BankD = (1 << 3),
        BankE = (1 << 4),
        BankF = (1 << 5),
        BankG = (1 << 6),
        BankH = (1 << 7),
        BankI = (1 << 8),
        BankJ = (1 << 9),
        BankK = (1 << 10),
        BankL = (1 << 11),
        BankM = (1 << 12),
        BankN = (1 << 13),
        BankO = (1 << 14),
        BankP = (1 << 15),
        BankQ = (1 << 16),
    }electrodebanks_t;

    typedef enum {
        SourceDefault = 0,  /**< default stream source */
        SourceAP = 0,
        SourceLFP = 1
    }streamsource_t;

    struct PacketInfo {
        uint32_t Timestamp;
        uint16_t Status;
        uint16_t payloadlength;
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
        WRONG_CHANNEL = 9,/**< illegal channel or channel group number */
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
        IO_ERROR = 30,/**< a data stream IO error occurred. */
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
        NO_LINK = 44, /**< no head stage was detected */
        NO_FLEX = 45, /**< no flex board was detected */
        NO_PROBE = 46, /**< no probe was detected */
        NO_HST = 47, /**< no headstage tester detected */
        WRONG_ADC = 48, /**< the calibration data contains a wrong ADC identifier */
        WRONG_SHANK = 49, /**< the shank parameter was out of bound */
        UNKNOWN_STREAMSOURCE = 50, /**< the streamsource parameter is unknown */
        ILLEGAL_HANDLE = 51, /**< the value of the 'handle' parameter is not valid. */
        OBJECT_MISMATCH = 52, /**< the object type is not of the expected class */
        WRONG_DACCHANNEL = 53,  /**< the specified DAC channel is out of bound */
        WRONG_ADCCHANNEL = 54,  /**< the specified ADC channel is out of bound */
        NODATA = 55, /**<  No data available to perform action (fe.: Waveplayer) */
        PROGRAMMING_FAILED = 56, /**<  Firmware programming failed (e.g. incorrect readback) */

        // TODO: IMU integration - subject to change
        NO_IMU = 57,

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
        GND_REF = 3,   /**< Ground reference */
        NONE_REF = 0xFF /**< disconnect reference */
    }channelreference_t;

    /**
     * Column pattern for UHD2 probe
     */
    typedef enum
    {
        INNER = 0,
        OUTER = 1,
        ALL   = 2
    } columnpattern_t;

#define enumspace_SM_Input    0x01
#define enumspace_SM_Output   0x02
#define enumspace_SM_Sync     0x03

#define NP_GetEnumClass(enumvalue) (((enumvalue)>>24) & 0xFF)
#define NP_EnumClass(enumspace, value) ((((enumspace) & 0xFF)<<24) | (value))

#define SM_Input_(value)  NP_EnumClass(enumspace_SM_Input, value)
#define SM_Output_(value) NP_EnumClass(enumspace_SM_Output, value)
#define SM_SyncSource_(value) NP_EnumClass(enumspace_SM_Sync, value)

    typedef enum {
        swcap_none = 0,
        /// @brief Switch matrix signal supports edge selection
        swcap_edgeselect = (1 << 0),
        /// @brief Switch matrix signal supports inversion
        swcap_invert = (1 << 1)
    }swcapflags_t;

    typedef enum {
        SyncSource_None = SM_SyncSource_(0),
        SyncSource_SMA  = SM_SyncSource_(1),
        SyncSource_Clock = SM_SyncSource_(2),
    }syncsource_t;

    typedef enum {
        SM_Input_None = SM_Input_(0),

        SM_Input_SWTrigger1 = SM_Input_(1),
        SM_Input_SWTrigger2 = SM_Input_(2),

        SM_Input_SMA  = SM_Input_(5), /* PXI system SMA input */
        SM_Input_SMA1 = SM_Input_(6), /* OneBox backpanel SMA (SMA1) */

        SM_Input_PXI0 = SM_Input_(0x10),
        SM_Input_PXI1 = SM_Input_(0x11),
        SM_Input_PXI2 = SM_Input_(0x12),
        SM_Input_PXI3 = SM_Input_(0x13),
        SM_Input_PXI4 = SM_Input_(0x14),
        SM_Input_PXI5 = SM_Input_(0x15),
        SM_Input_PXI6 = SM_Input_(0x16),
        SM_Input_PXISYNC = SM_Input_(0x17),

        SM_Input_ADC0  = SM_Input_(0x20),
        SM_Input_ADC1  = SM_Input_(0x21),
        SM_Input_ADC2  = SM_Input_(0x22),
        SM_Input_ADC3  = SM_Input_(0x23),
        SM_Input_ADC4  = SM_Input_(0x24),
        SM_Input_ADC5  = SM_Input_(0x25),
        SM_Input_ADC6  = SM_Input_(0x26),
        SM_Input_ADC7  = SM_Input_(0x27),
        SM_Input_ADC8  = SM_Input_(0x28),
        SM_Input_ADC9  = SM_Input_(0x29),
        SM_Input_ADC10 = SM_Input_(0x3A),
        SM_Input_ADC11 = SM_Input_(0x3B),
        SM_Input_ADC12 = SM_Input_(0x3C),
        SM_Input_ADC13 = SM_Input_(0x3D),
        SM_Input_ADC14 = SM_Input_(0x3E),
        SM_Input_ADC15 = SM_Input_(0x3F),

        SM_Input_SyncClk = SM_Input_(0x40),
        SM_Input_TimeStampClk = SM_Input_(0x41),
        SM_Input_ADCClk = SM_Input_(0x42),
    }switchmatrixinput_t;

    typedef enum {
        SM_Output_None = SM_Output_(0),

        SM_Output_SMA = SM_Output_(1), /* PXI system SMA output */
        SM_Output_AcquisitionTrigger = SM_Output_(2),
        SM_Output_StatusBit = SM_Output_(3),

        SM_Output_PXI0 = SM_Output_(4),
        SM_Output_PXI1 = SM_Output_(5),
        SM_Output_PXI2 = SM_Output_(6),
        SM_Output_PXI3 = SM_Output_(7),
        SM_Output_PXI4 = SM_Output_(8),
        SM_Output_PXI5 = SM_Output_(9),
        SM_Output_PXI6 = SM_Output_(10),
        SM_Output_PXISYNC = SM_Output_(11),

        SM_Output_SMA1 = SM_Output_(16), /* OneBox backpanel SMA (SMA1) */
        SM_Output_DAC0 = SM_Output_(32),
        SM_Output_DAC1 = SM_Output_(33),
        SM_Output_DAC2 = SM_Output_(34),
        SM_Output_DAC3 = SM_Output_(35),
        SM_Output_DAC4 = SM_Output_(36),
        SM_Output_DAC5 = SM_Output_(37),
        SM_Output_DAC6 = SM_Output_(38),
        SM_Output_DAC7 = SM_Output_(39),
        SM_Output_DAC8 = SM_Output_(40),
        SM_Output_DAC9 = SM_Output_(41),
        SM_Output_DAC10 = SM_Output_(42),
        SM_Output_DAC11 = SM_Output_(43),

        SM_Output_WavePlayerTrigger = SM_Output_(64),
    }switchmatrixoutput_t;

    typedef enum {
        triggeredge_rising  = 1,
        triggeredge_falling = 2,
    }triggeredge_t;

    typedef enum {
        swtrigger_none = 1,
        swtrigger1 = 2,
        swtrigger2 = 4,
    }swtriggerflags_t;

    typedef void* np_streamhandle_t;

    /* System Functions *******************************************************************/
    /*
     * \brief Returns the major/minor version number of the API library
     */
    NP_EXPORT void getAPIVersion(int* version_major, int* version_minor);
    /*
     * \brief Returns the full version string of the API library
     *
     * @param buffer: destination buffer
     * @param size: size of destination buffer
     */
    NP_EXPORT size_t getAPIVersionFull(char* buffer, size_t size);
    /*
     * \brief Read the last error message
     *
     * @param bufStart: destination buffer
     * @param bufsize: size of the destination buffer
     * @returns amount of characters written to the destination buffer
     */
    NP_EXPORT size_t getLastErrorMessage(char* bufStart, size_t bufsize);

    /**
     * Get an error message for a given error code.
     *
     * @param code Error code
     * @returns const pointer to string containing error message
     */
    NP_EXPORT const char* getErrorMessage(NP_ErrorCode code);

    /*
     * \brief Get a cached list of available devices. Use 'scanBS' to update this list
     *        Note that this list contains all discovered devices. Use 'mapBS' to map a device to a 'slot'
     * @param list: output list of available devices
     * @param count: entry count of list buffer
     * @returns amount of devices found
     */
    NP_EXPORT int getDeviceList(struct basestationID* list, int count);

    /*
     * \brief Get the basestation info descriptor for a mapped device.
     */
    NP_EXPORT NP_ErrorCode getDeviceInfo(int slotID, struct basestationID* info);

    /*
     * \brief try to get the associated slotID
     * @param bsid: basestation ID (use getDeviceList to retrieve list of known devices)
     * @param slotID: output value associated slot ID
     * @returns true if the bsid is found and mapped to a slot, false otherwise
     */
    NP_EXPORT bool tryGetSlotID(const basestationID* bsid, int* slotID);

    /*
     * \brief Scan the system for available devices. This function updates the cached device list. (See getDeviceList)
     */
    NP_EXPORT NP_ErrorCode scanBS(void);

    /*
     * \brief Maps a specific basestation device to a slot.
     *        If a device with the specified platform and serial number is discovered (using 'scanBS'), it will be automatically
     *        mapped to the specified slot.
     *        Note: A PXI basestation device is automatically mapped to a slot corresponding to its PXI geographic location
     */
    NP_EXPORT NP_ErrorCode mapBS(int serialnr, int slot);

    /**
     * Unmaps a basestation mapped to slot.
     * Returns NO_SLOT if slot is not mapped.
     *
     * @param slot: slot to unmap.
     */
    NP_EXPORT NP_ErrorCode unmapBS(int slot);

    typedef void* npcallbackhandle_t;
    /*
     * \brief destroys a callback handle.
     */
    NP_EXPORT NP_ErrorCode destroyHandle(npcallbackhandle_t* phandle);

    typedef enum {
        NP_PARAM_BUFFERSIZE = 1,
        NP_PARAM_BUFFERCOUNT = 2,
        NP_PARAM_SYNCMASTER = 3,
        NP_PARAM_SYNCFREQUENCY_HZ = 4,
        NP_PARAM_SYNCPERIOD_MS = 5,
        NP_PARAM_SYNCSOURCE = 6,
        NP_PARAM_SIGNALINVERT = 7,

        // internal use only
        NP_PARAM_IGNOREPROBESN = 0x1000,
        NP_PARAM_ALLOWMANUALPXISYNCCONFIG = 0x1001
    }np_parameter_t;
    /*
     * \brief Set the value of a system-wide parameter
    */
    NP_EXPORT NP_ErrorCode setParameter(np_parameter_t paramid, int value);
    /*
     * \brief Get the value of a system-wide parameter
    */
    NP_EXPORT NP_ErrorCode getParameter(np_parameter_t paramid, int* value);
    /*
     * \brief Set the value of a system-wide parameter
    */
    NP_EXPORT NP_ErrorCode setParameter_double(np_parameter_t paramid, double value);
    /*
     * \brief Get the value of a system-wide parameter
    */
    NP_EXPORT NP_ErrorCode getParameter_double(np_parameter_t paramid, double* value);

    /* Base station board/Slot Functions **************************************************/

    NP_EXPORT NP_ErrorCode detectBS(int slotID, bool* detected);
    /*
    * \brief Open a basestation at the specified slot.
    *        Note(1) The basestation device needs to be mapped to a slot first (using mapBS)
    */
    NP_EXPORT NP_ErrorCode openBS(int slotID);
    /*
    * \brief Closes a basestation and release all associated resources
    */
    NP_EXPORT NP_ErrorCode closeBS(int slotID);
    /*
    * \brief Set the basestation in a 'armed' state. In a armed state, the basestation
    *        is ready to start acquisition after a trigger.
    */
    NP_EXPORT NP_ErrorCode arm(int slotID);
    /*
    * \brief Force a software trigger 1.
    */
    NP_EXPORT NP_ErrorCode setSWTrigger(int slotID);

    /*
    * \brief Force software triggers on multiple software trigger channels. Onebox supports 2 trigger channels, PXI only 1
    * @param triggerflags: A mask of software triggers to set.
    */
    NP_EXPORT NP_ErrorCode setSWTriggerEx(int slotID, swtriggerflags_t triggerflags);

    /*
    * \brief Configures the input/output trigger signal edge as rising/falling sensitive
    */
    NP_EXPORT NP_ErrorCode setTriggerEdge(int slotID, bool rising);

    /*
     * \brief Connect/disconnect a switch matrix input to/from an output signal
     */
    NP_EXPORT NP_ErrorCode switchmatrix_set(int slotID, switchmatrixoutput_t output, switchmatrixinput_t inputline, bool connect);
    /*
     * \brief Get the switch matrix input to a output signal connection state
     */
    NP_EXPORT NP_ErrorCode switchmatrix_get(int slotID, switchmatrixoutput_t output, switchmatrixinput_t inputline, bool* isconnected);
    /*
     * \brief Clear all connections to a switch matrix output signal
     */
    NP_EXPORT NP_ErrorCode switchmatrix_clear(int slotID, switchmatrixoutput_t output);
    NP_EXPORT NP_ErrorCode switchmatrix_setInputInversion(int slotID, switchmatrixinput_t input, bool invert);
    NP_EXPORT NP_ErrorCode switchmatrix_getInputInversion(int slotID, switchmatrixinput_t input, bool* invert);
    NP_EXPORT NP_ErrorCode switchmatrix_setOutputInversion(int slotID, switchmatrixoutput_t output, bool invert);
    NP_EXPORT NP_ErrorCode switchmatrix_getOutputInversion(int slotID, switchmatrixoutput_t output, bool* invert);
    NP_EXPORT NP_ErrorCode switchmatrix_setOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t edge);
    NP_EXPORT NP_ErrorCode switchmatrix_getOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t* edge);



    typedef enum {
        NP_DATAMODE_OFF = 0,
        NP_DATAMODE_ELECTRODE = 1,
        NP_DATAMODE_ADC = 2
    }np_datamode_t;

    NP_EXPORT NP_ErrorCode setDataMode(int slotID, int portID, np_datamode_t mode);
    NP_EXPORT NP_ErrorCode getDataMode(int slotID, int portID, np_datamode_t* mode);


    /*
     * \brief Get the basestation temperature (in degrees Celsius)
     */
    NP_EXPORT NP_ErrorCode bs_getTemperature(int slotID, double* temperature_degC);
    /*
     * \brief Get the basestation firmware version info
     */
    NP_EXPORT NP_ErrorCode bs_getFirmwareInfo(int slotID, struct firmware_Info* info);

    /**
     * Update the basestation firmware.
     *
     * If programming is aborted by the callback returning 0, a PROGRAMMINGABORTED error is returned.
     * If the firmware image readback was incorrect after the programming, a PROGRAMMING_FAILED error is returned.
     *
     * @param slotID target slot to update
     * @param filename firmware binary file
     * @param callback (optional, may be null). Progress callback function. if callback returns 0, the update aborts
     */
    NP_EXPORT NP_ErrorCode bs_updateFirmware(int slotID, const char* filename, int(*callback)(size_t byteswritten));

    /*
     * \brief (Only on PXI platform) Get the basestation connect board temperature (in degrees Celsius)
     */
    NP_EXPORT NP_ErrorCode bsc_getTemperature(int slotID, double* temperature_degC);
    /*
     * \brief (Only on PXI platform) Get the basestation connect board firmware version info
     */
    NP_EXPORT NP_ErrorCode bsc_getFirmwareInfo(int slotID, struct firmware_Info* info);

    /**
     * Update the basestation connect board firmware.
     *
     * If programming is aborted by the callback returning 0, a PROGRAMMINGABORTED error is returned.
     * If the firmware image readback was incorrect after the programming, a PROGRAMMING_FAILED error is returned.
     *
     * @param slotID target slot to update
     * @param filename firmware binary file
     * @param callback (optional, may be null). Progress callback function. if callback returns 0, the update aborts
     */
    NP_EXPORT NP_ErrorCode bsc_updateFirmware(int slotID, const char* filename, int(*callback)(size_t byteswritten));


    /* File stream API *****************************************************************************************/
    /*
     * \brief Associate a raw packet file dump stream with a mapped basestation
    */
    NP_EXPORT NP_ErrorCode setFileStream   (int slotID, const char* filename);
    /*
     * \brief Enable writing to the file stream (See setFileStream)
    */
    NP_EXPORT NP_ErrorCode enableFileStream(int slotID, bool enable);

    /**
    * @brief Get the amount of ports on the base station connect board
    * @param slotID: slot ID
    * @param count: output parameter, amount of available ports
    * @returns SUCCESS if successful.
    */
    NP_EXPORT NP_ErrorCode getBSCSupportedPortCount(int slotID, int* count);
    /**
    * @brief Get the amount of probes the connected headstage supports
    * @param slotID: slot ID
    * @param portID: portID (1..4)
    * @param count: output parameter, amount of flex/probes
    * @returns SUCCESS if successful.
    */
    NP_EXPORT NP_ErrorCode getHSSupportedProbeCount(int slotID, int portID, int* count);

    NP_EXPORT NP_ErrorCode openPort                (int slotID, int portID);
    NP_EXPORT NP_ErrorCode closePort               (int slotID, int portID);
    NP_EXPORT NP_ErrorCode detectHeadStage         (int slotID, int portID, bool* detected);
    NP_EXPORT NP_ErrorCode detectFlex              (int slotID, int portID, int dockID, bool* detected);
    NP_EXPORT NP_ErrorCode setHSLed                (int slotID, int portID, bool enable);

    NP_EXPORT NP_ErrorCode getFlexVersion          (int slotID, int portID, int dockID, int* version_major, int* version_minor);
    NP_EXPORT NP_ErrorCode readFlexPN              (int slotID, int portID, int dockID, char* pn, size_t maxlen);

    NP_EXPORT NP_ErrorCode getHSVersion            (int slotID, int portID, int* version_major, int* version_minor);
    NP_EXPORT NP_ErrorCode readHSPN                (int slotID, int portID, char* pn, size_t maxlen);
    NP_EXPORT NP_ErrorCode readHSSN                (int slotID, int portID, uint64_t* sn);
    NP_EXPORT NP_ErrorCode readProbeSN             (int slotID, int portID, int dockID, uint64_t* id);
    NP_EXPORT NP_ErrorCode readProbePN             (int slotID, int portID, int dockID, char* pn, size_t maxlen);

    NP_EXPORT NP_ErrorCode readBSCPN               (int slotID, char* pn, size_t len);
    NP_EXPORT NP_ErrorCode readBSCSN               (int slotID, uint64_t* sn);
    NP_EXPORT NP_ErrorCode getBSCVersion           (int slotID, int* version_major, int* version_minor);

    /** Configures the sync pattern on the FPGA (NPM2b/c specific)
     *
     *	Using this procedure the sync pattern can be configured on the FPGA.
     *	This sync pattern is used to detect sync errors by comparing the pattern
     *	with the pattern sent by the probe ASIC.
     *
     * @param slotID: slot ID
     * @param portID: port ID
     * @param sync_pattern: Pointer to three consecutive uint32_t values to be used as sync pattern
     */
    NP_EXPORT NP_ErrorCode setPortSyncPattern(int slotID, int portID, uint32_t* sync_pattern);
    NP_EXPORT NP_ErrorCode getPortSyncPattern(int slotID, int portID, uint32_t* sync_pattern);

    /* Probe functions *******************************************************************/
    NP_EXPORT NP_ErrorCode openProbe               (int slotID, int portID, int dockID);
    NP_EXPORT NP_ErrorCode closeProbe              (int slotID, int portID, int dockID);
    NP_EXPORT NP_ErrorCode init                    (int slotID, int portID, int dockID);
    NP_EXPORT NP_ErrorCode writeProbeConfiguration (int slotID, int portID, int dockID, bool readCheck);
    NP_EXPORT NP_ErrorCode setADCCalibration       (int slotID, int portID, const char* filename);
    NP_EXPORT NP_ErrorCode setGainCalibration      (int slotID, int portID, int dockID, const char* filename);

    /** Configures the sync pattern on the probe ASIC (NPM2b/c specific)
     *
     * Using this procedure the sync pattern can be configured on the probe ASIC.
     * This sync pattern is sent as sync word in the PSB frames.
     *
     * @param slotID: slot ID
     * @param portID: port ID
     * @param dockID: dock ID
     * @param sync_pattern: pointer to three consecutive uint32_t values to be used as sync pattern
     */
    NP_EXPORT NP_ErrorCode setProbeSyncPattern(int slotID, int portID, int dockID, uint32_t* sync_pattern);
    NP_EXPORT NP_ErrorCode getProbeSyncPattern(int slotID, int portID, int dockID, uint32_t* sync_pattern);

// <NP1 Specific>
#define NP1_PROBE_CHANNEL_COUNT   384
#define NP1_PROBE_SUPERFRAMESIZE  12
#define NP1_PROBE_ADC_COUNT       32

#define ELECTRODEPACKET_STATUS_TRIGGER    (1<<0)
#define ELECTRODEPACKET_STATUS_SYNC       (1<<6)

#define ELECTRODEPACKET_STATUS_LFP        (1<<1)
#define ELECTRODEPACKET_STATUS_ERR_COUNT  (1<<2)
#define ELECTRODEPACKET_STATUS_ERR_SERDES (1<<3)
#define ELECTRODEPACKET_STATUS_ERR_LOCK   (1<<4)
#define ELECTRODEPACKET_STATUS_ERR_POP    (1<<5)
#define ELECTRODEPACKET_STATUS_ERR_SYNC   (1<<7)

    struct electrodePacket {
        uint32_t timestamp[NP1_PROBE_SUPERFRAMESIZE];
        int16_t apData[NP1_PROBE_SUPERFRAMESIZE][NP1_PROBE_CHANNEL_COUNT];
        int16_t lfpData[NP1_PROBE_CHANNEL_COUNT];
        uint16_t Status[NP1_PROBE_SUPERFRAMESIZE];
    };
    NP_EXPORT NP_ErrorCode readElectrodeData         (int slotID, int portID, int dockID, struct electrodePacket* packets, int* actualAmount, int requestedAmount);
    NP_EXPORT NP_ErrorCode getElectrodeDataFifoState (int slotID, int portID, int dockID, int* packetsavailable, int* headroom);
// </NP1 Specific>

    NP_EXPORT NP_ErrorCode setTestSignal             (int slotID, int portID, int dockID, bool enable);
    NP_EXPORT NP_ErrorCode setOPMODE                 (int slotID, int portID, int dockID, probe_opmode_t mode);
    NP_EXPORT NP_ErrorCode setCALMODE                (int slotID, int portID, int dockID, testinputmode_t mode);
    NP_EXPORT NP_ErrorCode setREC_NRESET             (int slotID, int portID, bool state);

    /**
    * @brief Read a single packet data from the specified fifo.
    *        This is a non blocking function that tries to read a single packet
    *        from the specified receive fifo.
    * @param slotID: slot ID
    * @param portID: portID (1..4)
    * @param dock: probe index (1..2 (for NPM))
    * @param source: Select the stream source from the probe (SourceAP or SourceLFP). Ignored if probe does not support multiple sources
    * @param pckinfo: output data containing additional packet data: timestamp, stream status, and payload length
    * @param data: unpacked 16 bit right aligned data
    * @param requestedChannelCount: size of data buffer (maximum amount of channels)
    * @param actualread: optional output parameter that returns the amount of channels unpacked for a single timestamp.
    * @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0)
    */
    NP_EXPORT NP_ErrorCode readPacket(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pckinfo, int16_t* data, int requestedChannelCount, int* actualread);

    /**
    * @brief Read multiple packets from the specified fifo.
    *        This is a non blocking function.
    * @param slotID: slot ID
    * @param portID: portID (1..4)
    * @param dock: probe index (1..2 (for NPM))
    * @param source: Select the stream source from the probe (SourceAP or SourceLFP). Ignored if probe does not support multiple sources
    * @param pckinfo: output data containing additional packet data: timestamp, stream status, and payload length.
    *                 size of this buffer is expected to be sizeof(struct PacketInfo)*packetcount
    * @param data: unpacked 16 bit right aligned data. size of this buffer is expected to be 'channelcount*packetcount*sizeof(int16_t)'
    * @param channelcount: amount of channels to read per packet. This value is also the data stride value in the result 'data' buffer.
    * @param packetcount: amount of channels to read per packet. This value is also the data stride value in the result 'data' buffer.
    * @param packetsread: amount of packets read from the fifo.
    * @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0)
    */
    NP_EXPORT NP_ErrorCode readPackets(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pckinfo, int16_t* data, int channelcount, int packetcount, int* packetsread);

    NP_EXPORT NP_ErrorCode getPacketFifoStatus(int slotID, int portID, int dockID, streamsource_t source, int* packetsavailable, int* headroom);

    NP_EXPORT NP_ErrorCode destroyHandle(npcallbackhandle_t* phandle);

    /**
    * @brief Create a slot packet receive callback function
    *        the callback function will be called for each packet received on the specified slot.
    *        Use 'destroyHandle' to unbind from the packet receive handler.
    *        Note: multiple callbacks may be registered to the same slot.
    * @param slotID: slot address
    * @param handle: output parameter that will contain the callback handle.
    * @param callback: user callback function that will be associated with the output handle
    * @param userdata: optional user data that will be supplied to the callback function.
    * @returns SUCCESS if successful
    */
    NP_EXPORT NP_ErrorCode createSlotPacketCallback(int slotID, npcallbackhandle_t* handle, np_packetcallbackfn_t callback, const void* userdata);
    /**
    * @brief Create a probe packet receive callback function
    *        the callback function will be called for each packet received on the specified probe/streamsource combination.
    *        Use 'destroyHandle' to unbind from the packet receive handler.
    *        Note: multiple callbacks may be registered to the same probe.
    * @param slotID: slot address
    * @param portID: port ID (1..4)
    * @param dock: the targetted probe (1..2 - only for NPM)
    * @param handle: output parameter that will contain the callback handle.
    * @param callback: user callback function that will be associated with the output handle
    * @param userdata: optional user data that will be supplied to the callback function.
    * @returns SUCCESS if successful
    */
    NP_EXPORT NP_ErrorCode createProbePacketCallback(int slotID, int portID, int dockID, streamsource_t source, npcallbackhandle_t* handle, np_packetcallbackfn_t callback, const void* userdata);


    /**
    * @brief Unpacks samples from a raw data packet frame.
    * @param packet: the packet to unpack data from. This function will inspect the packet's header
    *                to determine the packing format and data alignment.
    * @param output: output buffer.
    * @param samplestoread: maximum amount of elements to dat.
    * @param actualread: output argument;returns amount of samples succesfully unpacked into 'output' buffer
    * @returns SUCCESS
    */
    NP_EXPORT NP_ErrorCode unpackData(const np_packet_t* packet, int16_t* output, int samplestoread, int* actualread);

    /* Probe Channel functions ***********************************************************/
    NP_EXPORT NP_ErrorCode setGain(int slotID, int portID, int dockID, int channel, int ap_gain, int lfp_gain);
    NP_EXPORT NP_ErrorCode getGain(int slotID, int portID, int dockID, int channel, int* APgainselect, int* LFPgainselect);
    NP_EXPORT NP_ErrorCode selectElectrode(int slotID, int portID, int dockID, int channel, int shank, int bank);
    NP_EXPORT NP_ErrorCode selectElectrodeMask(int slotID, int portID, int dockID, int channel, int shank, electrodebanks_t bankmask);
    NP_EXPORT NP_ErrorCode setReference(int slotID, int portID, int dockID, int channel, int shank, channelreference_t reference, int intRefElectrodeBank);
    NP_EXPORT NP_ErrorCode setAPCornerFrequency(int slotID, int portID, int dockID, int channel, bool disableHighPass);
    NP_EXPORT NP_ErrorCode setStdb(int slotID, int portID, int dockID, int channel, bool standby);
    /**
     * Set the column pattern of electrodes on the shank.
     *
     * Before selecting the connection between channel groups and electrodes,
     * the user must first set which column pattern of electrodes on the shank is activated.
     * If the function is called, all existing channel group to electrode group connections are removed.
     *
     * @param slotID Slot address
     * @param portID Port ID
     * @param dockID Probe to configure
     * @param pattern Pattern to use
     */
    NP_EXPORT NP_ErrorCode selectColumnPattern(int slotID, int portID, int dockID, columnpattern_t pattern);
    /**
     * Connect a single bank to a channel group.
     *
     * The following function is used to connect a single bank to a channel group.
     * Calling this function disconnects any previously bank(s) connected to the selected channel group.
     * If the paramater 'bank' is set to 0xFF the channelgroup is disconnected from all banks.
     *
     * @param slotID Slot address
     * @param portID Port ID
     * @param dockID Probe to configure
     * @param channelgroup Channel group to connect, can have value from 0 to 23.
     * @param bank Bank to connect, can have value from 0 to 15
     */
    NP_EXPORT NP_ErrorCode selectElectrodeGroup(int slotID, int portID, int dockID, int channelgroup, int bank);
    /**
     * Connect a maximum of two banks to channel group.
     *
     * In the case the EN_A or EN_B bit in the shank register is set low,
     * it is allowed to connect a channel group to maximum two banks.
     *
     * @param slotID Slot address
     * @param portID Port ID
     * @param dockID Probe to configure
     * @param channelgroup Channel group to connect, can have value from 0 to 23.
     * @param mask Banks to connect, combining maximum two values.
     */
    NP_EXPORT NP_ErrorCode selectElectrodeGroupMask(int slotID, int portID, int dockID, int channelgroup, electrodebanks_t mask);

    /* Onebox AUXilary IO functions */

    // Onebox Waveplayer
    /**
    * @brief Write to the waveplayer's sample buffer.
    * @param slotID: the slot number of the device
    * @param data: A buffer of length (len) of 16 bit signed data samples.
    * @param len: amount of samples in 'data'
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode waveplayer_writeBuffer(int slotID, const int16_t* data, int len);

    /**
    * @brief 'Arm' the waveplayer. This prepares the output SMA channel to playback the waveform programmed with 'waveplayer_writeBuffer'.
    *         The waveplayer must be triggered by a signal in the switch matrix. (See switchmatrix_set)
    *         By default, SM_Input_SWTrigger2 is bound as the WavePlayer software trigger.
    * @param slotID: the slot number of the device
    * @param singleshot: if true, the waveplayer will play the programmed waveform once after trigger. if false, the waveform is repeat until rearmed in other mode.
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode waveplayer_arm(int slotID, bool singleshot);
    /**
    * @brief Set the waveplayer's sampling frequency in Hz.
    *        The actual sampling frequency being used can be read using waveplayer_getSampleFrequency.
    * @param slotID: the slot number of the device
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode waveplayer_setSampleFrequency(int slotID, double frequency_Hz);
    /**
    * @brief Get the actual waveplayer's sampling frequency in Hz
    * @param slotID: the slot number of the device
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode waveplayer_getSampleFrequency(int slotID, double* frequency_Hz);

    /**
    * @brief Directly reads the voltage of a particular ADC Channel.
    * @param slotID: the slot number of the device
    * @param ADCChannel: The ADC channel to read the data from (valid range 0 to 11)
    * @param voltage: return voltage of the ADC Channel
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_read(int slotID, int ADCChannel, double* voltage);

    /**
    * @brief Directly reads the ADC comparator output state.
    *        The low/high comparator threshold values can be set using (ADC_setComparatorThreshold)
    * @param slotID: the slot number of the device
    * @param ADCChannel: The ADC channel to read the data from
    * @param state: returns the comparator output state.
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_readComparator(int slotID, int ADCChannel, bool* state);

    /**
    * @brief Directly reads the ADC comparator state of all ADC channels in a single output word.
    *        The low/high comparator threshold values can be set using (ADC_setComparatorThreshold)
    * @param slotID: the slot number of the device
    * @param flags: A word containing the comparator state of each ADC channel (bit0 = ADCCH0, bit 1 = ADCCH1, etc...)
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_readComparators(int slotID, uint32_t* flags);

    /**
    * @brief Enable/Disables the auxiliary ADC probe
    *        If disabled, no ADC channel or comparator values are updated.
    * @param slotID: the slot number of the device
    * @param state: true to enable, false to disable
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_enableProbe(int slotID, bool enable);

    /**
    * @brief Get the LSB to voltage conversion factor and bitdepth for the ADC probe channel
    *        This conversion changes with programmed ADC range (ADC_setVoltageRange)
    * @param slotID: the slot number of the device
    * @param lsb_to_voltage: conversion factor to convert 16 bit signed value to voltage
    * @param bitdepth: optional return value that indicates the number of bits in the ADC stream.
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_getStreamConversionFactor(int slotID, double* lsb_to_voltage, int* bitdepth);

    /**
    * Set the ADC comparator low/high threshold voltages per channel.
    *
    * @param slotID The slot number of the device
    * @param ADCChannel ADC channel (valid range 0 to 11)
    * @param vlow Low comparator threshold voltage. Comparator state will toggle to 0 if the input is below this value.
    * @param vhigh High comparator threshold voltage. Comparator state will toggle to 1 if the input is above this value.
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_setComparatorThreshold(int slotID, int ADCChannel, double vlow, double vhigh);

    /**
    * Get the programmed ADC comparator low/high threshold voltages.
    *
    * @param slotID The slot number of the device
    * @param ADCChannel ADC channel (valid range 0 to 11)
    * @param vlow Get the low comparator threshold voltage. Comparator state will toggle to 0 if the input is below this value.
    * @param vhigh Get the high comparator threshold voltage. Comparator state will toggle to 1 if the input is above this value.
    * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_getComparatorThreshold(int slotID, int ADCChannel, double* vlow, double* vhigh);

    /**
     * Enum to configure ADC voltage range.
     * Actual range is from -range .. +range, e.g. -5V to 5V.
    */
    typedef enum {
        ADC_RANGE_2_5V, /**< 2.5V */
        ADC_RANGE_5V,   /**< 5V */
        ADC_RANGE_10V   /**< 10V */
    } ADCrange_t;

    /**
    * Set the ADC Voltage range.
    *
    * This voltage range is used for all ADC channels.
    *
    * @param slotID The slot number of the device
    * @param range Programmed range will be -range .. +range.
    * @return SUCCESS if successful.
    *         NOTSUPPORTED if this functionality is not supported by the device.
    *		  PARAMETER_INVALID if the range is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_setVoltageRange(int slotID, ADCrange_t range);

    /**
    * Get the programmed ADC Voltage range.
    *
    * @param slotID The slot number of the device
    * @param range Programmed range will be -range .. +range.
    * @return SUCCESS if successful.
    *         NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_getVoltageRange(int slotID, ADCrange_t* range);

    /**
    * @brief Set a DAC channel to a fixed voltage.
    * @param slotID: the slot number of the device
    * @param DACChannel: The DAC channel to configure
    * @param voltage: The requested fixed output voltage.
    * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode DAC_setVoltage(int slotID, int DACChannel, double voltage);

    /**
    * @brief Set a DAC channel in digital tracking mode, and program its low and high voltage.
    *        In this mode, the DAC channel acts as an output of the switch matrix (See switchmatrix_set).
    * @param slotID: the slot number of the device
    * @param DACChannel: The DAC channel to configure
    * @param vhigh: DAC voltage for Digital 'H'
    * @param vlow: DAC voltage for Digital 'L'
    * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode DAC_setDigitalLevels(int slotID, int DACChannel, double vhigh, double vlow);

    /**
    * @brief Enable DAC channel output on SDR connector.
    * @param slotID: the slot number of the device
    * @param DACChannel: The DAC channel to configure
    * @param state: true to enable output, false for high impedance
    * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode DAC_enableOutput(int slotID, int DACChannel, bool state);

    /**
    * @brief Set a DAC channel in probe sniffing mode.
    *        The output of the DAC will now track a programmed probe channel.
    * @param slotID: the slot number of the device
    * @param DACChannel: the target DAC channel
    * @param portID: the port number of the probe
    * @param dockID: the dock number of the probe
    * @param channelnr: the probe's channel nr that will be tracked
    * @param sourcetype: source stream. (Default AP)
    * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode DAC_setProbeSniffer(int slotID, int DACChannel, int portID, int dockID, int channelnr, streamsource_t sourcetype);
    /**
    * @brief Read multiple packets from the auxiliary ADC probe stream.
    *        (Note that the ADC probe needs to be enabled (ADC_enableProbe))
    *        This is a non blocking function.
    * @param slotID: slot ID
    * @param pckinfo: output data containing additional packet data: timestamp, stream status, and payload length.
    *                 size of this buffer is expected to be sizeof(struct PacketInfo)*packetcount
    * @param data: unpacked 16 bit right aligned data. size of this buffer is expected to be 'channelcount*packetcount*sizeof(int16_t)'
    * @param channelcount: amount of channels to read per packet. This value is also the data stride value in the result 'data' buffer. Onebox supports 12 ADC channels + 12 comparator channels
    * @param packetcount: amount of channels to read per packet. This value is also the data stride value in the result 'data' buffer.
    * @param packetsread: amount of packets read from the fifo.
    * @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0). NOTSUPPORTED if this functionality is not supported by the device
    */
    NP_EXPORT NP_ErrorCode ADC_readPackets(int slotID, struct PacketInfo* pckinfo, int16_t* data, int channelcount, int packetcount, int* packetsread);

    /**
     * Get status (available packets and remaining capacity) of auxilary ADC probe stream FIFO.
     *
     * @param slotID slot ID
     * @param packetsavailable number of packets available for read
     * @param headroom remaining capacity of the FIFO
     */
    NP_EXPORT NP_ErrorCode ADC_getPacketFifoStatus(int slotID, int* packetsavailable, int* headroom);

    /********************* Built In Self Test ****************************/
    /**
    * @brief Basestation platform BIST
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the chassis range is entered.
    */
    NP_EXPORT NP_ErrorCode bistBS(int slotID);

    /**
    * @brief Head Stage heartbeat test
    *
    * The heartbeat signal generated by the PSB_SYNC signal of the probe. The PSB_SYNC signal starts when the probe is powered on, the OP_MODE register in the probes' memory map set to 1, and the REC_NRESET signal set high.
    * The heartbeat signal is visible on the headstage (can be disabled by API functions) and on the BSC. This is in the first place a visual check.
    * In order to facilitate a software check of the BSC heartbeat signal, the PSB_SYNC signal is also routed to the BS FPGA. A function is provided to check whether the PSB_SYNC signal contains a 0.5Hz clock.
    * The presence of a heartbeat signal acknowledges the functionality of the power supplies on the headstage for serializer and probe, the POR signal, the presence of the master clock signal on the probe, the functionality of the clock divider on the probe, an basic communication over the serdes link.
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
    */
    NP_EXPORT NP_ErrorCode bistHB(int slotID, int portID, int dockID);

    /**
    * @brief Start Serdes PRBS test
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
    */
    NP_EXPORT NP_ErrorCode bistStartPRBS(int slotID, int portID);

    /**
    * @brief Stop Serdes PRBS test
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @param prbs_err: the number of prbs errors
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
    */
    NP_EXPORT NP_ErrorCode bistStopPRBS(int slotID, int portID, int* prbs_err);

    /**
    * @brief Read intermediate Serdes PRBS test result
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @param prbs_err: the number of prbs errors
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
    */
    NP_EXPORT NP_ErrorCode bistReadPRBS(int slotID, int portID, int* prbs_err);

    /**
    * @brief Test I2C memory map access
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered, NO_ACK in case no acknowledgment is received, READBACK_ERROR in case the written and readback word are not the same.
    */
    NP_EXPORT NP_ErrorCode bistI2CMM(int slotID, int portID, int dockID);

    /**
    * @brief Test all EEPROMs (Flex, headstage, BSC). by verifying a write/readback cycle on an unused memory location
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered, NO_ACK_FLEX in case no acknowledgment is received from the flex eeprom, READBACK_ERROR_FLEX in case the written and readback word are not the same from the flex eeprom, NO_ACK_HS in case no acknowledgment is received from the HS eeprom, READBACK_ERROR_HS in case the written and readback word are not the same from the HS eeprom, NO_ACK_BSC in case no acknowledgment is received from the BSC eeprom, READBACK_ERROR_BSC in case the written and readback word are not the same from the BSC eeprom.
    */
    NP_EXPORT NP_ErrorCode bistEEPROM(int slotID, int portID);


    /**
    * @brief Test the shift registers
    * This test verifies the functionality of the shank and base shift registers (SR_CHAIN 1 to 3). The function configures the shift register two times with the same code. After the 2nd write cycle the SR_OUT_OK bit in the STATUS register is read. If OK, the shift register configuration was successful. The test is done for all 3 registers. The configuration code used for the test is a dedicated code (to make sure the bits are not all 0 or 1).
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered, ERROR_SR_CHAIN_1 in case the SR_OUT_OK bit is not ok when writing SR_CHAIN_1, ERROR_SR_CHAIN_2 in case the SR_OUT_OK bit is not ok when writing SR_CHAIN_2, ERROR_SR_CHAIN_3 in case the SR_OUT_OK bit is not ok when writing SR_CHAIN_3.
    */
    NP_EXPORT NP_ErrorCode bistSR(int slotID, int portID, int dockID);

    /**
    * @brief Test the PSB bus on the headstage
    * A test mode is implemented on the probe which enables the transmission of a known data pattern over the PSB bus. The following function sets the probe in this test mode, records a small data set, and verifies whether the acquired data matches the known data pattern.
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @returns SUCCESS if successful, NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
    */
    NP_EXPORT NP_ErrorCode bistPSB(int slotID, int portID, int dockID);

    /**
    * @brief The probe is configured for noise analysis. Via the shank and base configuration registers and the memory map, the electrode inputs are shorted to ground. The data signal is recorded and the noise level is calculated. The function analyses if the probe performance falls inside a specified tolerance range (go/no-go test).
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param port: for which HS (valid range 1 to 4)
    * @returns SUCCESS if successful, BIST_ERROR of test failed. NO_LINK if no datalink, NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, WRONG_SLOT in case a slot number outside the valid range is entered, WRONG_PORT in case a port number outside the valid range is entered.
    */
    NP_EXPORT NP_ErrorCode bistNoise(int slotID, int portID, int dockID);
    NP_EXPORT NP_ErrorCode bistSignal(int slotID, int portID, int dockID);

    /********************* Headstage tester API functions ****************************/
    NP_EXPORT NP_ErrorCode HST_GetVersion(int slotID, int portID, int* vmaj, int* vmin);
    NP_EXPORT NP_ErrorCode HSTestVDDA1V2(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestVDDD1V2(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestVDDA1V8(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestVDDD1V8(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestOscillator(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestMCLK(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestPCLK(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestPSB(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestI2C(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestNRST(int slotID, int portID);
    NP_EXPORT NP_ErrorCode HSTestREC_NRESET(int slotID, int portID);


    /********************* Raw Stream file API functions ****************************/

    /**
    * @brief Open an acquisition stream from an existing file.
    * @param filename Specifies an existing file with probe acquisition data.
    * @param port specifies the target port (1..4)
    * @param dockID 1..2 (depending on type of probe. default=1)
    * @param source data type (AP or LFP) if supported (default = AP)
    * @param psh stream a pointer to the stream pointer that will receive the handle to the opened stream
    * @returns FILE_OPEN_ERROR if unable to open file
    */
    NP_EXPORT NP_ErrorCode streamOpenFile(const char* filename, int portID, int dockID, streamsource_t source, np_streamhandle_t* pstream);

    /**
    * @brief Closes an acquisition stream.
    * Closes the stream along with the optional recording file.
    */
    NP_EXPORT NP_ErrorCode streamClose(np_streamhandle_t sh);

    /**
    * @brief Moves the stream pointer to given timestamp.
    * Stream seek is only supported on streams that are backed by a recording file store.
    * @param stream: the acquisition stream handle
    * @param filepos: The file position to navigate to.
    * @param actualtimestamp: returns the timestamp at the stream pointer (NULL allowed)
    * @returns TIMESTAMPNOTFOUND if no valid data packet is found beyond the specified file position
    */
    NP_EXPORT NP_ErrorCode streamSeek(np_streamhandle_t sh, uint64_t filepos, uint32_t* actualtimestamp);

    NP_EXPORT NP_ErrorCode streamSetPos(np_streamhandle_t sh, uint64_t filepos);

    /**
    * @brief Report the current file position in the filestream.
    * @param stream: the acquisition stream handle
    * @returns the current file position at the stream cursor position.
    */
    NP_EXPORT uint64_t streamTell(np_streamhandle_t sh);

    /**
    * @brief read probe data from a recorded file stream.
    * Example:
    *    #define SAMPLECOUNT 128
    *    uint16_t interleaveddata[SAMPLECOUNT * 384];
    *    uint32_t timestamps[SAMPLECOUNT];
    *
    *    np_streamhandle_t sh;
    *    streamOpenFile("myrecording.bin",1, false, &sh);
    *    int actualread;
    *    streamRead(sh, timestamps, interleaveddata, SAMPLECOUNT, &actualread);
    *
    * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
    * @param timestamps: Optional timestamps buffer (NULL if not used). size should be 'samplecount'
    * @param data: buffer of size samplecount*384. The buffer will be populated with channel interleaved, 16 bit per sample data.
    * @param samplestoread: amount of timestamps to read.
    * @param actualread: output parameter: amount of 16 timestamps actually read from the stream.
    * @returns SUCCESS if succesfully read any sample from the stream
    */
    NP_EXPORT NP_ErrorCode streamRead(np_streamhandle_t sh, uint32_t* timestamps, int16_t* data, int samplestoread, int* actualread);

    NP_EXPORT NP_ErrorCode streamReadPacket(np_streamhandle_t sh, pckhdr_t* header, int16_t* data, int samplestoread, int* actualread);


// Configuration

    const int HARDWAREID_PN_LEN = 400;
#pragma pack(push, 1)
    struct HardwareID {
        uint8_t version_Major;
        uint8_t version_Minor;
        uint64_t SerialNumber;
        char ProductNumber[HARDWAREID_PN_LEN];
    };
#pragma pack(pop)

    NP_EXPORT NP_ErrorCode getBasestationDriverID(int slotID, char* name, size_t len);
    NP_EXPORT NP_ErrorCode getHeadstageDriverID(int slotID, int portID, char* name, size_t len);
    NP_EXPORT NP_ErrorCode getFlexDriverID(int slotID, int portID, int dockID, char* name, size_t len);
    NP_EXPORT NP_ErrorCode getProbeDriverID(int slotID, int portID, int dockID, char* name, size_t len);

    NP_EXPORT NP_ErrorCode getBSCHardwareID(int slotID, struct HardwareID* pHwid);
    NP_EXPORT NP_ErrorCode setBSCHardwareID(int slotID, const struct HardwareID* pHwid);
    NP_EXPORT NP_ErrorCode getHeadstageHardwareID(int slotID, int portID, struct HardwareID* pHwid);
    NP_EXPORT NP_ErrorCode setHeadstageHardwareID(int slotID, int portID, const struct HardwareID* pHwid);
    NP_EXPORT NP_ErrorCode getFlexHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
    NP_EXPORT NP_ErrorCode setFlexHardwareID(int slotID, int portID, int dockID, const struct HardwareID* pHwid);
    NP_EXPORT NP_ErrorCode getProbeHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
    NP_EXPORT NP_ErrorCode setProbeHardwareID(int slotID, int portID, int dockID, const struct HardwareID* pHwid);


// Debug

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

    struct np_diagstats {
        uint64_t totalbytes;         /**< total amount of bytes received */
        uint32_t packetcount;        /**< Amount of packets received */
        uint32_t triggers;           /**< Amount of triggers received */
        uint32_t err_badmagic;		 /**< amount of packet header bad MAGIC markers */
        uint32_t err_badcrc;		 /**< amount of packet header CRC errors */
        uint32_t err_count;			 /**< Every psb frame has an incrementing count index. If the received frame count value is not as expected possible data loss has occured and this flag is raised. */
        uint32_t err_serdes;		 /**< incremented if a deserializer error (hardware pin) occured during receiption of this frame this flag is raised */
        uint32_t err_lock;			 /**< incremented if a deserializer loss of lock (hardware pin) occured during receiption of this frame this flag is raised */
        uint32_t err_pop;			 /**< incremented whenever the next blocknummer round-robin FiFo is flagged empty during request of the next value (for debug purpose only, irrelevant for end-user software) */
        uint32_t err_sync;			 /**< Front-end receivers are out of sync. => frame is invalid. */
    };

    struct np_sourcestats {
        uint32_t timestamp;          /**< last recorded timestamp */
        uint32_t packetcount;        /**< last recorded packet counter */
        uint32_t samplecount;        /**< last recorded sample counter */
        uint32_t fifooverflow;       /**< last recorded sample counter */
    };

    typedef enum {
        NPSlotEmulatorMode_Off = 0, /**< No emulation data is generated */
        NPSlotEmulatorMode_Static = 1, /**< static data per channel: value = channel number */
        NPSlotEmulatorMode_Linear = 2, /**< a linear ramp is generated per channel (1 sample shift between channels) */
    } slotemulatormode_t;

    typedef enum
    {
        NPSlotEmulatorType_NP2,
        NPSlotEmulatorType_NPM,
        NPSlotEmulatorType_NPM2,
        NPSlotEmulatorType_Colibri
    } slotemulatortype_t;

    typedef enum {
        NPPortEmulatorMode_Off     = 0,
        NPPortEmulatorMode_NP1_0   = 1,
        NPPortEmulatorMode_NHP     = 2,
        NPPortEmulatorMode_NPM     = 3,
        NPPortEmulatorMode_NPM2b   = 4,
        NPPortEmulatorMode_NPM2c   = 5,
        NPPortEmulatorMode_Colibri = 6
    } portemulatormode_t;

    typedef enum
    {
        // A new device was found
        DeviceFound = 1,
        // A detected devices is no longer present
        DeviceLost = 2,
        // A device is mapped to a slot (callback 'eventdata' holds the slot number)
        DeviceMapped = 3,
        // The device was unmapped from a slot (callback 'eventdata' holds the slot number)
        DeviceUnMapped = 4
    }npsystemevent_t;

    typedef void(NP_CALLBACK* np_systemeventcallbackfn_t)(npsystemevent_t eventtype, const void* eventdata, const void* userdata);
    /*
     * \brief Register a system event callback handler.
     */
    NP_EXPORT NP_ErrorCode createSystemEventCallback(npcallbackhandle_t* handle, np_systemeventcallbackfn_t callback, const void* userdata);

    NP_EXPORT NP_ErrorCode writeBSCMM(int slotID, uint32_t address, uint32_t data);
    NP_EXPORT NP_ErrorCode readBSCMM(int slotID, uint32_t address, uint32_t* data);
    NP_EXPORT NP_ErrorCode writeI2C(int slotID, int portID, uint8_t device, uint8_t address, uint8_t data);
    NP_EXPORT NP_ErrorCode readI2C(int slotID, int portID, uint8_t device, uint8_t address, uint8_t* data);
    NP_EXPORT NP_ErrorCode writeI2Cex(int slotID, int portID, uint8_t device, uint8_t address, const void* data, size_t len);
    NP_EXPORT NP_ErrorCode readI2Cex(int slotID, int portID, uint8_t device, uint8_t address, void* data, size_t len);
    NP_EXPORT NP_ErrorCode writeI2Cflex(int slotID, int portID, int dockID, uint8_t device, uint8_t address, uint8_t data);
    NP_EXPORT NP_ErrorCode readI2Cflex(int slotID, int portID, int dockID, uint8_t device, uint8_t address, uint8_t* data);

    /* Neuropixels OPTO specific **********************************************************************************************************************/
    typedef enum
    {
        wavelength_blue, // 450nm
        wavelength_red   // 638nm
    }wavelength_t;

    /*
     * \brief Program the optical switch calibration using a calibration file
     * A OPTO headstage must be attached to SlotID/PortID
     * dockID is ignored
     *
     * @param slotID: which slot in the PXI chassis (valid range depends on the chassis)
     * @param portID: specifies the target port (valid range depends on slot type)
     * @param dockID: ignored
     * @param filename: comma separated text file (csv) containing following data:
     *       first line : probe serial number
     *       next lines: <wavelengthindex>, <thermalswitchindex>, <off_mA>, <on_mA>
     *        wavelengthindex    : 0 = blue(450nm), 1 = red(638nm)
     *        thermalswitchindex :
     *                0    : 1_1
     *				  1..2 : 2_1, 2_2
     *				  3..8 : 3_1, 3_2, 3_3, 3_4
     *				  9..14: 4_1, 4_2, 4_3, 4_4, 4_5, 4_6, 4_7, 4_8
     *        off_mA/on_mA : on/off current setting
     *
     * example csv file content:
     * 21050005
     * 0, 0,  0.0, 4.0
     * 0, 1,  0.0, 4.0
     * 0, 2,  0.0, 4.0
     * 0, 3,  0.0, 4.0
     * 0, 4,  0.0, 4.0
     * 0, 5,  0.0, 4.0
     * 0, 6,  0.0, 4.0
     * 0, 7,  0.0, 4.0
     * 0, 8,  0.0, 4.0
     * 0, 9,  0.0, 4.0
     * 0, 10, 0.0, 4.0
     * 0, 11, 0.0, 4.0
     * 0, 12, 0.0, 4.0
     * 0, 13, 0.0, 4.0
     * 0, 14, 0.0, 4.0
     * 1, 0,  0.0, 4.0
     * 1, 1,  0.0, 4.0
     * 1, 2,  0.0, 4.0
     * 1, 3,  0.0, 4.0
     * 1, 4,  0.0, 4.0
     * 1, 5,  0.0, 4.0
     * 1, 6,  0.0, 4.0
     * 1, 7,  0.0, 4.0
     * 1, 8,  0.0, 4.0
     * 1, 9,  0.0, 4.0
     * 1, 10, 0.0, 4.0
     * 1, 11, 0.0, 4.0
     * 1, 12, 0.0, 4.0
     * 1, 13, 0.0, 4.0
     * 1, 14, 0.0, 4.0
     */
    NP_EXPORT NP_ErrorCode setOpticalCalibration(int slotID, int portID, int dockID, const char* filename);
    /*
     * \brief program calibration current for a single optical thermal switch.
     * @param slotID:    which slot in the PXI chassis (valid range depends on the chassis)
     * @param portID:    specifies the target port (valid range depends on slot type)
     * @param dockID:    ignored
     * @param wavelength : optical path selection
     * @param thermalswitchindex : thermal switch calibration target.
     *                0    : 1_1
     *				  1..2 : 2_1, 2_2
     *				  3..8 : 3_1, 3_2, 3_3, 3_4
     *				  9..14: 4_1, 4_2, 4_3, 4_4, 4_5, 4_6, 4_7, 4_8
     * @param On_mA:  ON current for the target switch
     * @param Off_mA: OFF current for the target switch
     */
    NP_EXPORT NP_ErrorCode setOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermalswitchindex, double On_mA, double Off_mA);
    /*
     * \brief get the calibration currents for a single optical thermal switch.
     * @param slotID:    which slot in the PXI chassis (valid range depends on the chassis)
     * @param portID:    specifies the target port (valid range depends on slot type)
     * @param dockID:    ignored
     * @param wavelength : optical path selection
     * @param thermalswitchindex : thermal switch calibration target.
     *                0    : 1_1
     *				  1..2 : 2_1, 2_2
     *				  3..8 : 3_1, 3_2, 3_3, 3_4
     *				  9..14: 4_1, 4_2, 4_3, 4_4, 4_5, 4_6, 4_7, 4_8
     * @param On_mA:  ON current for the target switch
     * @param Off_mA: OFF current for the target switch
     */
    NP_EXPORT NP_ErrorCode getOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermalswitchindex, double* On_mA, double* Off_mA);
    /*
     * \brief activate an optical emission site.
     * @param slotID:    which slot in the PXI chassis (valid range depends on the chassis)
     * @param portID:    specifies the target port (valid range depends on slot type)
     * @param dockID:    ignored
     * @param wavelength: optical path selection
     * @param site:      emission site index (0..13) or -1 to disable the optical path
     */
    NP_EXPORT NP_ErrorCode setEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int site);
    /*
     * \brief activate an optical emission site.
     * @param slotID:    which slot in the PXI chassis (valid range depends on the chassis)
     * @param portID:    specifies the target port (valid range depends on slot type)
     * @param dockID:    ignored
     * @param wavelength: optical path selection
     * @param site:      get the active emission site index (0..13, or -1 if the path is disabled)
     */
    NP_EXPORT NP_ErrorCode getEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int* site);

    /*
     * \brief get the light power attenuation factor for an emission site
     * @param slotID:    which slot in the PXI chassis (valid range depends on the chassis)
     * @param portID:    specifies the target port (valid range depends on slot type)
     * @param dockID:    ignored
     * @param wavelength: optical path selection
     * @param site:      the emission site index (0..13)
     * @param attenuation: get the laser power attenuation factor
     */
    NP_EXPORT NP_ErrorCode getEmissionSiteAttenuation(int slotID, int portID, int dockID, wavelength_t wavelength, int site, double* attenuation);
    /*
     * \brief Disable an optical emission path.
     * Note: only the current to the optical thermal switches is disabled. Laser power is not affected
     * @param slotID:    which slot in the PXI chassis (valid range depends on the chassis)
     * @param portID:    specifies the target port (valid range depends on slot type)
     * @param dockID:    ignored
     * @param wavelength: optical path selection
     */
    NP_EXPORT NP_ErrorCode disableEmissionPath(int slotID, int portID, int dockID, wavelength_t wavelength);

    /* Neuropixels IMU ********************************************************************************************************************************/

    // TODO: IMU integration - subject to change

    typedef enum accelerometer_scale_t {
        ACC_SCALE_2G,
        ACC_SCALE_4G,
        ACC_SCALE_8G,
        ACC_SCALE_16G
    } accelerometer_scale_t;

    typedef enum gyroscope_scale_t {
        GYRO_SCALE_250,
        GYRO_SCALE_500,
        GYRO_SCALE_1000,
        GYRO_SCALE_2000
    } gyroscope_scale_t;

#pragma pack(push, 1)
    typedef struct IMUPacket {
        uint32_t timestamp;
        uint8_t status;

        uint8_t packet_counter;
        uint8_t delay;

        int16_t accel_x;
        int16_t accel_y;
        int16_t accel_z;

        int16_t gyro_x;
        int16_t gyro_y;
        int16_t gyro_z;

        int16_t temperature;

        int16_t magn_x;
        int16_t magn_y;
        int16_t magn_z;

    } IMUPacket;
#pragma pack(pop)

    NP_EXPORT NP_ErrorCode IMU_detect(int slot, int port, bool* detected);
    NP_EXPORT NP_ErrorCode IMU_enable(int slot, int port, bool enable);
    NP_EXPORT NP_ErrorCode IMU_setAccelerometerSampleRateDivider(int slot, int port, uint16_t divider);
    NP_EXPORT NP_ErrorCode IMU_setAccelerometerScale(int slot, int port, accelerometer_scale_t scale);
    NP_EXPORT NP_ErrorCode IMU_setGyroscopeSampleRateDivider(int slot, int port, uint8_t divider);
    NP_EXPORT NP_ErrorCode IMU_setGyroscopeScale(int slot, int port, gyroscope_scale_t scale);
    NP_EXPORT NP_ErrorCode IMU_readPackets(int slot, int port, int packets_requested, IMUPacket* packets, int* packets_read);
    NP_EXPORT NP_ErrorCode IMU_getFIFOStatus(int slot, int port, int* packets_available, int* headroom);
    NP_EXPORT NP_ErrorCode IMU_getPllTimeBaseCorrection(int slot, int port, int* ppl_timebasecorrection);
    NP_EXPORT NP_ErrorCode IMU_DfuRead(int slot, int port, uint8_t* data, size_t len, size_t* bytes_read);
    NP_EXPORT NP_ErrorCode IMU_DfuWrite(int slot, int port, const uint8_t* data, size_t len, size_t* bytes_written);

    /* Debug support functions ************************************************************************************************************************/
    NP_EXPORT void         dbg_setlevel(int level);
    NP_EXPORT int          dbg_getlevel(void);
    NP_EXPORT void         dbg_setlogcallback(int minlevel, void(*callback)(int level, time_t ts, const char* module, const char* msg));
    NP_EXPORT void         dbg_getversion_datetime(char* dst, size_t maxlen);
    NP_EXPORT NP_ErrorCode dbg_setSlotEmulatorMode(int slotID, slotemulatormode_t mode);
    NP_EXPORT NP_ErrorCode dbg_getSlotEmulatorMode(int slotID, slotemulatormode_t* mode);
    /** Sets the emulation type of the slot.
     *
     * To avoid conflicts between slot and port emulator types,
     * the following actions will be executed when calling this function:
     * - All open ports will be closed
     * - The emulator mode of the port will be set to "Off"
     *
     * @param slotID slot ID
     * @param type emulation type
     */
    NP_EXPORT NP_ErrorCode dbg_setSlotEmulatorType(int slotID, slotemulatortype_t type);
    NP_EXPORT NP_ErrorCode dbg_getSlotEmulatorType(int slotID, slotemulatortype_t* type);
    NP_EXPORT NP_ErrorCode dbg_setSlotEmulatorSyncPattern(int slotID, uint32_t* sync_pattern);
    NP_EXPORT NP_ErrorCode dbg_getSlotEmulatorSyncPattern(int slotID, uint32_t* sync_pattern);
    NP_EXPORT NP_ErrorCode dbg_setPortEmulatorMode(int slotID, int portID, portemulatormode_t emulationmode);
    NP_EXPORT NP_ErrorCode dbg_getPortEmulatorMode(int slotID, int portID, portemulatormode_t* emulationmode);
    NP_EXPORT NP_ErrorCode dbg_stats_reset(int slotID);
    NP_EXPORT NP_ErrorCode dbg_diagstats_read(int slotID, struct np_diagstats* stats);
    NP_EXPORT NP_ErrorCode dbg_sourcestats_read(int slotID, uint8_t sourceID, struct np_sourcestats* stats);
    NP_EXPORT NP_ErrorCode dbg_read_srchain(int slotID, int portID, int dockID, uint8_t SRChain_registeraddress, uint8_t* dst, size_t len, size_t* actualread);
    NP_EXPORT NP_ErrorCode setVirtualHeadstage(int slotID, int portID);




#define NP_APIC __stdcall
    extern "C" {
        //NeuropixAPI.h
        NP_EXPORT void         NP_APIC np_getAPIVersion(int* version_major, int* version_minor);
        NP_EXPORT size_t       NP_APIC np_getAPIVersionFull(char* buffer, size_t size);
        NP_EXPORT size_t       NP_APIC np_getLastErrorMessage(char* bufStart, size_t bufsize);
        NP_EXPORT const char*  NP_APIC np_getErrorMessage(NP_ErrorCode code);
        NP_EXPORT int          NP_APIC np_getDeviceList(struct basestationID* list, int count);
        NP_EXPORT NP_ErrorCode NP_APIC np_getDeviceInfo(int slotID, struct basestationID* info);
        NP_EXPORT bool         NP_APIC np_tryGetSlotID(const basestationID* bsid, int* slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_scanBS(void);
        NP_EXPORT NP_ErrorCode NP_APIC np_mapBS(int serialnr, int slot);
        NP_EXPORT NP_ErrorCode NP_APIC np_unmapBS(int slot);
        NP_EXPORT NP_ErrorCode NP_APIC np_destroyHandle(npcallbackhandle_t* phandle);
        NP_EXPORT NP_ErrorCode NP_APIC np_setParameter(np_parameter_t paramid, int value);
        NP_EXPORT NP_ErrorCode NP_APIC np_getParameter(np_parameter_t paramid, int* value);
        NP_EXPORT NP_ErrorCode NP_APIC np_setParameter_double(np_parameter_t paramid, double value);
        NP_EXPORT NP_ErrorCode NP_APIC np_getParameter_double(np_parameter_t paramid, double* value);
        NP_EXPORT NP_ErrorCode NP_APIC np_detectBS(int slotID, bool* detected);
        NP_EXPORT NP_ErrorCode NP_APIC np_openBS(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_closeBS(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_arm(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_setSWTrigger(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_setSWTriggerEx(int slotID, swtriggerflags_t triggerflags);
        NP_EXPORT NP_ErrorCode NP_APIC np_setTriggerEdge(int slotID, bool rising);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_set(int slotID, switchmatrixoutput_t output, switchmatrixinput_t inputline, bool connect);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_get(int slotID, switchmatrixoutput_t output, switchmatrixinput_t inputline, bool* isconnected);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_clear(int slotID, switchmatrixoutput_t output);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_setInputInversion(int slotID, switchmatrixinput_t input, bool invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_getInputInversion(int slotID, switchmatrixinput_t input, bool* invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_setOutputInversion(int slotID, switchmatrixoutput_t output, bool invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_getOutputInversion(int slotID, switchmatrixoutput_t output, bool* invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_setOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t edge);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_getOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t* edge);
        NP_EXPORT NP_ErrorCode NP_APIC np_setDataMode(int slotID, int portID, np_datamode_t mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_getDataMode(int slotID, int portID, np_datamode_t* mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_getTemperature(int slotID, double* temperature_degC);
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_getFirmwareInfo(int slotID, struct firmware_Info* info);
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_updateFirmware(int slotID, const char* filename, int(*callback)(size_t byteswritten));
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_getTemperature(int slotID, double* temperature_degC);
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_getFirmwareInfo(int slotID, struct firmware_Info* info);
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_updateFirmware(int slotID, const char* filename, int(*callback)(size_t byteswritten));
        NP_EXPORT NP_ErrorCode NP_APIC np_setFileStream(int slotID, const char* filename);
        NP_EXPORT NP_ErrorCode NP_APIC np_enableFileStream(int slotID, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_getBSCSupportedPortCount(int slotID, int* count);
        NP_EXPORT NP_ErrorCode NP_APIC np_getHSSupportedProbeCount(int slotID, int portID, int* count);
        NP_EXPORT NP_ErrorCode NP_APIC np_openPort(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_closePort(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_detectHeadStage(int slotID, int portID, bool* detected);
        NP_EXPORT NP_ErrorCode NP_APIC np_detectFlex(int slotID, int portID, int dockID, bool* detected);
        NP_EXPORT NP_ErrorCode NP_APIC np_setHSLed(int slotID, int portID, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_getFlexVersion(int slotID, int portID, int dockID, int* version_major, int* version_minor);
        NP_EXPORT NP_ErrorCode NP_APIC np_readFlexPN(int slotID, int portID, int dockID, char* pn, size_t maxlen);
        NP_EXPORT NP_ErrorCode NP_APIC np_getHSVersion(int slotID, int portID, int* version_major, int* version_minor);
        NP_EXPORT NP_ErrorCode NP_APIC np_readHSPN(int slotID, int portID, char* pn, size_t maxlen);
        NP_EXPORT NP_ErrorCode NP_APIC np_readHSSN(int slotID, int portID, uint64_t* sn);
        NP_EXPORT NP_ErrorCode NP_APIC np_readProbeSN(int slotID, int portID, int dockID, uint64_t* id);
        NP_EXPORT NP_ErrorCode NP_APIC np_readProbePN(int slotID, int portID, int dockID, char* pn, size_t maxlen);
        NP_EXPORT NP_ErrorCode NP_APIC np_readBSCPN(int slotID, char* pn, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_readBSCSN(int slotID, uint64_t* sn);
        NP_EXPORT NP_ErrorCode NP_APIC np_getBSCVersion(int slotID, int* version_major, int* version_minor);
        NP_EXPORT NP_ErrorCode NP_APIC np_setPortSyncPattern(int slotID, int portID, uint32_t* sync_pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_getPortSyncPattern(int slotID, int portID, uint32_t* sync_pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_openProbe(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_closeProbe(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_init(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_writeProbeConfiguration(int slotID, int portID, int dockID, bool readCheck);
        NP_EXPORT NP_ErrorCode NP_APIC np_setADCCalibration(int slotID, int portID, const char* filename);
        NP_EXPORT NP_ErrorCode NP_APIC np_setGainCalibration(int slotID, int portID, int dockID, const char* filename);
        NP_EXPORT NP_ErrorCode NP_APIC np_setProbeSyncPattern(int slotID, int portID, int dockID, uint32_t* sync_pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_getProbeSyncPattern(int slotID, int portID, int dockID, uint32_t* sync_pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_readElectrodeData(int slotID, int portID, int dockID, struct electrodePacket* packets, int* actualAmount, int requestedAmount);
        NP_EXPORT NP_ErrorCode NP_APIC np_getElectrodeDataFifoState(int slotID, int portID, int dockID, int* packetsavailable, int* headroom);
        NP_EXPORT NP_ErrorCode NP_APIC np_setTestSignal(int slotID, int portID, int dockID, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_setOPMODE(int slotID, int portID, int dockID, probe_opmode_t mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_setCALMODE(int slotID, int portID, int dockID, testinputmode_t mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_setREC_NRESET(int slotID, int portID, bool state);
        NP_EXPORT NP_ErrorCode NP_APIC np_readPacket(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pckinfo, int16_t* data, int requestedChannelCount, int* actualread);
        NP_EXPORT NP_ErrorCode NP_APIC np_readPackets(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pckinfo, int16_t* data, int channelcount, int packetcount, int* packetsread);
        NP_EXPORT NP_ErrorCode NP_APIC np_getPacketFifoStatus(int slotID, int portID, int dockID, streamsource_t source, int* packetsavailable, int* headroom);
        NP_EXPORT NP_ErrorCode NP_APIC np_createSlotPacketCallback(int slotID, npcallbackhandle_t* handle, np_packetcallbackfn_t callback, const void* userdata);
        NP_EXPORT NP_ErrorCode NP_APIC np_createProbePacketCallback(int slotID, int portID, int dockID, streamsource_t source, npcallbackhandle_t* handle, np_packetcallbackfn_t callback, const void* userdata);
        NP_EXPORT NP_ErrorCode NP_APIC np_unpackData(const np_packet_t* packet, int16_t* output, int samplestoread, int* actualread);
        NP_EXPORT NP_ErrorCode NP_APIC np_setGain(int slotID, int portID, int dockID, int channel, int ap_gain, int lfp_gain);
        NP_EXPORT NP_ErrorCode NP_APIC np_getGain(int slotID, int portID, int dockID, int channel, int* APgainselect, int* LFPgainselect);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrode(int slotID, int portID, int dockID, int channel, int shank, int bank);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrodeMask(int slotID, int portID, int dockID, int channel, int shank, electrodebanks_t bankmask);
        NP_EXPORT NP_ErrorCode NP_APIC np_setReference(int slotID, int portID, int dockID, int channel, int shank, channelreference_t reference, int intRefElectrodeBank);
        NP_EXPORT NP_ErrorCode NP_APIC np_setAPCornerFrequency(int slotID, int portID, int dockID, int channel, bool disableHighPass);
        NP_EXPORT NP_ErrorCode NP_APIC np_setStdb(int slotID, int portID, int dockID, int channel, bool standby);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectColumnPattern(int slotID, int portID, int dockID, columnpattern_t pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrodeGroup(int slotID, int portID, int dockID, int channelgroup, int bank);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrodeGroupMask(int slotID, int portID, int dockID, int channelgroup, electrodebanks_t mask);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_writeBuffer(int slotID, const int16_t* data, int len);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_arm(int slotID, bool singleshot);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_setSampleFrequency(int slotID, double frequency_Hz);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_getSampleFrequency(int slotID, double* frequency_Hz);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_read(int slotID, int ADCChannel, double* voltage);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_readComparator(int slotID, int ADCChannel, bool* state);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_readComparators(int slotID, uint32_t* flags);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_enableProbe(int slotID, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getStreamConversionFactor(int slotID, double* lsb_to_voltage, int* bitdepth);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_setComparatorThreshold(int slotID, int ADCChannel, double vlow, double vhigh);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getComparatorThreshold(int slotID, int ADCChannel, double* vlow, double* vhigh);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_setVoltageRange(int slotID, ADCrange_t range);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getVoltageRange(int slotID, ADCrange_t* range);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_setVoltage(int slotID, int DACChannel, double voltage);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_enableOutput(int slotID, int DACChannel, bool state);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_setDigitalLevels(int slotID, int DACChannel, double vhigh, double vlow);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_setProbeSniffer(int slotID, int DACChannel, int portID, int dockID, int channelnr, streamsource_t sourcetype);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_readPackets(int slotID, struct PacketInfo* pckinfo, int16_t* data, int channelcount, int packetcount, int* packetsread);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getPacketFifoStatus(int slotID, int* packetsavailable, int* headroom);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistBS(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistHB(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistStartPRBS(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistStopPRBS(int slotID, int portID, int* prbs_err);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistReadPRBS(int slotID, int portID, int* prbs_err);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistI2CMM(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistEEPROM(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistSR(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistPSB(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistNoise(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistSignal(int slotID, int portID, int dockID);

        NP_EXPORT NP_ErrorCode NP_APIC np_HST_GetVersion(int slotID, int portID, int* vmaj, int* vmin);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestVDDA1V2(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestVDDD1V2(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestVDDA1V8(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestVDDD1V8(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestOscillator(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestMCLK(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestPCLK(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestPSB(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestI2C(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestNRST(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_HSTestREC_NRESET(int slotID, int portID);

        NP_EXPORT NP_ErrorCode NP_APIC np_streamOpenFile(const char* filename, int portID, int dockID, streamsource_t source, np_streamhandle_t* pstream);
        NP_EXPORT NP_ErrorCode NP_APIC np_streamClose(np_streamhandle_t sh);
        NP_EXPORT NP_ErrorCode NP_APIC np_streamSeek(np_streamhandle_t sh, uint64_t filepos, uint32_t* actualtimestamp);
        NP_EXPORT NP_ErrorCode NP_APIC np_streamSetPos(np_streamhandle_t sh, uint64_t filepos);
        NP_EXPORT uint64_t     NP_APIC np_streamTell(np_streamhandle_t sh);
        NP_EXPORT NP_ErrorCode NP_APIC np_streamRead(np_streamhandle_t sh, uint32_t* timestamps, int16_t* data, int samplestoread, int* actualread);
        NP_EXPORT NP_ErrorCode NP_APIC np_streamReadPacket(np_streamhandle_t sh, pckhdr_t* header, int16_t* data, int samplestoread, int* actualread);

        //NeuropixAPI_configuration.h
        NP_EXPORT NP_ErrorCode NP_APIC np_getBasestationDriverID(int slotID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getHeadstageDriverID(int slotID, int portID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getFlexDriverID(int slotID, int portID, int dockID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getProbeDriverID(int slotID, int portID, int dockID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getBSCHardwareID(int slotID, struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_setBSCHardwareID(int slotID, const struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_getHeadstageHardwareID(int slotID, int portID, struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_setHeadstageHardwareID(int slotID, int portID, const struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_getFlexHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_setFlexHardwareID(int slotID, int portID, int dockID, const struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_getProbeHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_setProbeHardwareID(int slotID, int portID, int dockID, const struct HardwareID* pHwid);

        //NeuropixAPI_debug.h
        NP_EXPORT NP_ErrorCode NP_APIC np_createSystemEventCallback(npcallbackhandle_t* handle, np_systemeventcallbackfn_t callback, const void* userdata);
        NP_EXPORT NP_ErrorCode NP_APIC np_writeBSCMM(int slotID, uint32_t address, uint32_t data);
        NP_EXPORT NP_ErrorCode NP_APIC np_readBSCMM(int slotID, uint32_t address, uint32_t* data);
        NP_EXPORT NP_ErrorCode NP_APIC np_writeI2C(int slotID, int portID, uint8_t device, uint8_t address, uint8_t data);
        NP_EXPORT NP_ErrorCode NP_APIC np_readI2C(int slotID, int portID, uint8_t device, uint8_t address, uint8_t* data);
        NP_EXPORT NP_ErrorCode NP_APIC np_writeI2Cflex(int slotID, int portID, int dockID, uint8_t device, uint8_t address, uint8_t data);
        NP_EXPORT NP_ErrorCode NP_APIC np_readI2Cflex(int slotID, int portID, int dockID, uint8_t device, uint8_t address, uint8_t* data);
        NP_EXPORT void         NP_APIC np_dbg_setlevel(int level);
        NP_EXPORT int          NP_APIC np_dbg_getlevel(void);
        NP_EXPORT void         NP_APIC np_dbg_setlogcallback(int minlevel, void(*callback)(int level, time_t ts, const char* module, const char* msg));
        NP_EXPORT void         NP_APIC np_dbg_getversion_datetime(char* dst, size_t maxlen);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_setSlotEmulatorMode(int slotID, slotemulatormode_t mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_getSlotEmulatorMode(int slotID, slotemulatormode_t* mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_setSlotEmulatorType(int slotID, slotemulatortype_t type);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_getSlotEmulatorType(int slotID, slotemulatortype_t* type);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_setSlotEmulatorSyncPattern(int slotID, uint32_t* sync_pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_getSlotEmulatorSyncPattern(int slotID, uint32_t* sync_pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_setPortEmulatorMode(int slotID, int portID, portemulatormode_t emulationmode);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_getPortEmulatorMode(int slotID, int portID, portemulatormode_t* emulationmode);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_stats_reset(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_diagstats_read(int slotID, struct np_diagstats* stats);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_sourcestats_read(int slotID, uint8_t sourceID, struct np_sourcestats* stats);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_read_srchain(int slotID, int portID, int dockID, uint8_t SRChain_registeraddress, uint8_t* dst, size_t len, size_t* actualread);
        NP_EXPORT NP_ErrorCode NP_APIC np_setVirtualHeadstage(int slotID, int portID);

        // Opto specific
        NP_EXPORT NP_ErrorCode NP_APIC np_setOpticalCalibration(int slotID, int portID, int dockID, const char* filename);
        NP_EXPORT NP_ErrorCode NP_APIC np_setOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermalswitchindex, double On_mA, double Off_mA);
        NP_EXPORT NP_ErrorCode NP_APIC np_getOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermalswitchindex, double* On_mA, double* Off_mA);
        NP_EXPORT NP_ErrorCode NP_APIC np_setEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int site);
        NP_EXPORT NP_ErrorCode NP_APIC np_getEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int* site);
        NP_EXPORT NP_ErrorCode NP_APIC np_getEmissionSiteAttenuation(int slotID, int portID, int dockID, wavelength_t wavelength, int site, double* attenuation);
        NP_EXPORT NP_ErrorCode NP_APIC np_disableEmissionPath(int slotID, int portID, int dockID, wavelength_t wavelength);

        // Neuropixels IMU
        // TODO: IMU integration - subject to change
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_detect(int slot, int port, bool* detected);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_enable(int slot, int port, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_setAccelerometerSampleRateDivider(int slot, int port, uint16_t divider);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_setAccelerometerScale(int slot, int port, accelerometer_scale_t scale);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_setGyroscopeSampleRateDivider(int slot, int port, uint8_t divider);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_setGyroscopeScale(int slot, int port, gyroscope_scale_t scale);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_readPackets(int slot, int port, int packets_requested, IMUPacket* packets, int* packets_read);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_getFIFOStatus(int slot, int port, int* packets_available, int* headroom);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_getPllTimeBaseCorrection(int slot, int port, int* ppl_timebasecorrection);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_DfuRead(int slot, int port, uint8_t* data, size_t len, size_t* bytes_read);
        NP_EXPORT NP_ErrorCode NP_APIC np_IMU_DfuWrite(int slot, int port, const uint8_t* data, size_t len, size_t* bytes_written);
    }
} // namespace Neuropixels
