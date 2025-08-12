/********************************
 * Copyright (C) Imec 2024      *
 *                              *
 * Neuropixels C/C++ API header *
 ********************************/

/**
 * @file NeuropixAPI.h
 * @brief  Neuropixels c/c++ API header
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NP_EXPORT __declspec(dllexport)
#define NP_CALLBACK __stdcall

/**
 * @brief Main Neuropixels API namespace. All external functions are included in this namespace.
 */
namespace Neuropixels {

    /** Flag set of hardware basestation selection. */
    typedef enum {

        NPPlatform_None = 0, /**< Do not select any, or none are selected */
        NPPlatform_PXI = 0x1, /**< PXI (PCIe) neuropixels basestation */
        NPPlatform_USB = 0x2, /**< USB (Onebox) neuropixels basestation */
        NPPlatform_ALL = NPPlatform_PXI | NPPlatform_USB, /**< Any known neuropixel basestation */
    } NPPlatformID_t;

    /** Unique basestation identification.
     *
     * This struct is a return value only
     * The combination of \c platformid and \c ID form a unique tuple to identify a basestation
     */
    struct basestationID {
        NPPlatformID_t platformid; /**< Type of basestation platform. Can be USB/PXI */
        int   ID; /**< Unique ID for the specific platform. Each platform has its own  */
    };

    struct firmware_Info {
        int major;
        int minor;
        int build;
        char name[64];
    };

    typedef struct {
        int vmajor;
        int vminor;
        int vbuild;
    } ftdi_driver_version_t;

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
        SourceDefault = 0,  /**< Default stream source. */
        SourceAP = 0, /**<Source for the AP channel for NP1.0 and channel 0-383 for NP2.0.*/
        SourceLFP = 1, /**<Source for the LFP channel for NP1.0 and channel 384-767 for NP2.0 quad-base.*/
        SourceSt2 = 2, /**<Source for Channel 768-1151 for NP2.0 quad-base.*/
        SourceSt3 = 3 /**<Source for Channel 1152-1535 for NP2.0 quad-base.*/
    } streamsource_t;

    struct PacketInfo {
        uint32_t Timestamp; /**<Timestamp from the 100kHz clock */
        uint16_t Status; /**<The status word */
        uint16_t payloadlength; /**<Length of the payload*/
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
        NO_LOCK = 24,/**< missing serializer clock, probably a bad cable or connection */
        WRONG_AP = 25,/**< AP gain number out of range */
        WRONG_LFP = 26,/**< LFP gain number out of range */
        ERROR_SR_CHAIN = 27,/**< Validation of SRChain data upload failed */
        IO_ERROR = 30,/**< a data stream IO error occurred. */
        NO_SLOT = 31,/**< no Neuropix board found at the specified slot number */
        WRONG_SLOT = 32,/**<  the specified slot is out of bound */
        WRONG_PORT = 33,/**<  the specified port is out of bound */
        STREAM_EOF = 34,/**<  The stream is at the end of the file, but more data was expected*/
        HDRERR_MAGIC = 35, /**< The packet header is corrupt and cannot be decoded */
        HDRERR_CRC = 36, /**< The packet header's crc is invalid */
        WRONG_PROBESN = 37, /**< The probe serial number does not match the calibration data */
        PROGRAMMINGABORTED = 39, /**<  the flash programming was aborted */
        WRONG_DOCK_ID = 41, /**<  the specified probe id is out of bound */
        NO_BSCONNECT = 43, /**< no basestation connect board was found */
        NO_LINK = 44, /**< no headstage was detected */
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
        PROGRAMMING_FAILED = 56, /**< Firmware programming failed (e.g.: incorrect readback) */
        NO_IMU = 57, /**< Function requires a connected IMU but none was detected */
        DESERIALIZER_MODE_ERROR = 58, /**< Deserializer was powered on with incorrect configuration */
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
        NO_TEST_MODE = 2, /**< no test mode (Cal_SIGNAL disconnected) */
        ADC_MODE = 3, /**< HS test signal is connected to the ADC inputs */
    }testinputmode_t;

    typedef enum {
        EXT_REF = 0,  /**< External reference electrode: an input pin located on the probe flex. */
        TIP_REF = 1,  /**< Tip electrode: a large electrode located at the tip of the shank. */
        INT_REF = 2,   /**< Internal reference line: reference electrode on the shank, between recording electrodes. */
        GND_REF = 3,   /**< Ground reference: the reference input of the channel amplifiers is connected to the headstage ground signal. */
        NONE_REF = 0xFF /**< No reference: reference input disconnected, not recommended. */
    }channelreference_t;

    /**
     * Column pattern for UHD2 probe
     */
    typedef enum
    {
        INNER = 0, /**< Inner electrode columns */
        OUTER = 1, /**< Outer electrode columns */
        ALL   = 2 /**< All electrode columns */
    } columnpattern_t;

#define enumspace_SM_Input    0x01
#define enumspace_SM_Output   0x02

#define NP_GetEnumClass(enumvalue) (((enumvalue)>>24) & 0xFF)
#define NP_EnumClass(enumspace, value) ((((enumspace) & 0xFF)<<24) | (value))

#define SM_Input_(value)  NP_EnumClass(enumspace_SM_Input, value)
#define SM_Output_(value) NP_EnumClass(enumspace_SM_Output, value)

    typedef enum {
        SM_Input_None = SM_Input_(0), /**<No Input Connection */

        SM_Input_SWTrigger1 = SM_Input_(1), /**<Software trigger 1*/
        SM_Input_SWTrigger2 = SM_Input_(2), /**<Software trigger 2*/

        SM_Input_SMA  = SM_Input_(5), /**< SMA connector signal */

        SM_Input_PXI0 = SM_Input_(0x10), /**<PXIe backplane signal*/
        SM_Input_PXI1 = SM_Input_(0x11),
        SM_Input_PXI2 = SM_Input_(0x12),
        SM_Input_PXI3 = SM_Input_(0x13),
        SM_Input_PXI4 = SM_Input_(0x14),
        SM_Input_PXI5 = SM_Input_(0x15),
        SM_Input_PXI6 = SM_Input_(0x16),
        SM_Input_PXI7 = SM_Input_(0x17),

        SM_Input_ADC0  = SM_Input_(0x20), /**<ADC comparator output channel (OneBox only)*/
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

        SM_Input_SyncClk = SM_Input_(0x40), /**<Internal SYNC clock*/
        SM_Input_TimeStampClk = SM_Input_(0x41), /**<Internal timestamp clock*/
        SM_Input_ADCClk = SM_Input_(0x42), /**<ADC sample clock*/
    } switchmatrixinput_t;

    typedef enum {
        SM_Output_None = SM_Output_(0), /**<No Output Connection */

        SM_Output_SMA = SM_Output_(1),  /**<PXIe acquisition card front panel SMA connector or OneBox back panel SMA connector.*/
        SM_Output_AcquisitionTrigger = SM_Output_(2), /**<Neural data acquisition trigger or ADC data aquisition trigger on OneBox.*/
        SM_Output_StatusBit = SM_Output_(3), /**<bit #6 in the STATUS byte of the data packets.*/

        SM_Output_PXI0 = SM_Output_(4), /**<PXIe backplane trigger signal*/
        SM_Output_PXI1 = SM_Output_(5),
        SM_Output_PXI2 = SM_Output_(6),
        SM_Output_PXI3 = SM_Output_(7),
        SM_Output_PXI4 = SM_Output_(8),
        SM_Output_PXI5 = SM_Output_(9),
        SM_Output_PXI6 = SM_Output_(10),
        SM_Output_PXI7 = SM_Output_(11),

        SM_Output_DAC0 = SM_Output_(32), /**<DAC output channel (OneBox only)*/
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

        SM_Output_WavePlayerTrigger = SM_Output_(64), /**<DAC WavePlayer trigger (OneBox only)*/
    } switchmatrixoutput_t;

    typedef enum {
        triggeredge_rising  = 1,
        triggeredge_falling = 2,
    } triggeredge_t;

    typedef enum {
        swtrigger_none = 1,
        swtrigger1 = 2,
        swtrigger2 = 4,
    } swtriggerflags_t;

    /**
     * @brief Error rate for SerDes error generator
     * The SerDes uses 8b/10b encoding, so an error rate of e.g. 2560 bits
     * should be equivalent to an error every 256 actual data bytes.
     */
    typedef enum {
        SERDES_ERROR_RATE_LOW = 0, /**< One error every 10,485,760 bits */
        SERDES_ERROR_RATE_MEDIUM, /**< One error every 655,360 bits*/
        SERDES_ERROR_RATE_HIGH, /**< One error every 40,960 bits*/
        SERDES_ERROR_RATE_EXTREME /**< One error every 2560 bits */
    } serdes_error_rate_t;

    /**
     * @brief Number of errors generated by SerDes error generator
     */
    typedef enum {
        SERDES_ERROR_COUNT_CONTINUOUS = 0,
        SERDES_ERROR_COUNT_16,
        SERDES_ERROR_COUNT_128,
        SERDES_ERROR_COUNT_1024,
    } serdes_error_count_t;

    typedef void* np_streamhandle_t;

    typedef void* probeinfo_result_t;

    /**** System Functions *******************************************************************/

    /**
     * \brief Returns the major, minor and patch version number of the API library.
     */
    NP_EXPORT void getAPIVersion(int* version_major, int* version_minor, int* version_patch);
    /**
     * \brief Returns the full version string of the API library
     *
     * @param[out] buffer:   The destination buffer.
     * @param[in]  size:     The size of the destination buffer.
     */
    NP_EXPORT size_t getAPIVersionFull(char* buffer, size_t size);

    /**
     * \brief Returns the minimum required FTDI Driver version number that is expected by the API library
     */
    NP_EXPORT void getMinimumFtdiDriverVersion(int* version_major, int* version_minor, int* version_build);

    /**
     * \brief Checks if FTDI Driver is installed and the version of the driver is okay for the API.
     *
     * This function will only be able to report the current installed driver version and its correctness
     * if a OneBox is connected and powered on. This is due to an unfortunate limitation of the
     * FTDI library: we can only detect the driver version in use by instantiating it and connecting to
     * a working OneBox.
     *
     * The value of \p required will always be set when the function is called. If \p is_driver_present is
     * true, then the API was able to determine the currently installed driver version and the values
     * in \p current and \p is_version_ok can be used.
     *
     * @param[out] required Returns:    the minimum driver version required by the API.
     * @param[out] current Returns:     the currently installed driver version (if found).
     * @param[out] is_driver_present:   True if we were able to determine the installed driver version.
     * @param[out] is_version_ok:       True if the installed driver version meets the minimum requirements.
     */
    NP_EXPORT void checkFtdiDriver(ftdi_driver_version_t* required,
                                   ftdi_driver_version_t* current,
                                   bool* is_driver_present,
                                   bool* is_version_ok);

    /**
     * \brief Checks if firmware on a PXIe-based basestation is supported by the API.
     *
     * If the reported minimum firmware version is zero, the API was not able to determine the BSC type.
     * In this case we assume that the BSC is not supported by default.
     *
     * @param slotID Target slot (should be a PXIe slot)
     * @param carrier[out] Minimum required version of carrier firmware
     * @param bsc[out] Minimum required version of BSC firmware
     * @param carrier_supported[out] True if current carrier firmware is supported by the API
     * @param bsc_supported[out] True if current BSC firmware is supported by the API
     * @returns NOTSUPPORTED if slotID is not a PXIe slot
     */
    NP_EXPORT NP_ErrorCode checkBasestationSupported(int slotID, firmware_Info* carrier, firmware_Info* bsc, bool* carrier_supported, bool* bsc_supported);

    /**
     * \brief Read the last error message.
     *
     * @param[out] buffer: Destination buffer.
     * @param[in]  buffer_size: Size of the destination buffer.
     * @returns Amount of characters written to the destination buffer.
     */
    NP_EXPORT size_t getLastErrorMessage(char* buffer, size_t buffer_size);

    /**
     * \brief Get an error message for a given error code.
     *
     * @param[in] code: Error code.
     * @returns const char pointer to string containing error message.
     */
    NP_EXPORT const char* getErrorMessage(NP_ErrorCode code);

    /**
     * \brief Get a cached list of available devices. Use 'scanBS' to update this list.
     *
     * Note that this list contains all discovered devices. Use 'mapBS' to map a device to a 'slot'.
     *
     * @param[out] list:  Output list of available devices.
     * @param[in]  count: Entry count of list buffer.
     * @returns Amount of devices found.
     */
    NP_EXPORT int getDeviceList(struct basestationID* list, int count);

    /**
     * \brief Get the basestation info descriptor for a mapped device.
     */
    NP_EXPORT NP_ErrorCode getDeviceInfo(int slotID, struct basestationID* info);

    /**
     * \brief Try to get the associated slotID.
     * @param[in] bsid:   Basestation ID (use #getDeviceList to retrieve list of known devices).
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @returns true if the bsid is found and mapped to a slot, false otherwise.
     */
    NP_EXPORT bool tryGetSlotID(const basestationID* bsid, int* slotID);

    /**
     * \brief scans the USB ports for OneBox systems and checks the slots in a PXIe chassis (if connected) for PXIe Acquisition modules.
     * The function populates a list with devices, which can be accessed using the #getDeviceList function.
     */
    NP_EXPORT NP_ErrorCode scanBS(void);

    /**
     * \brief Maps a specific basestation device to a slot.
     *
     * If a device with the specified platform and serial number is discovered (using #scanBS), it will be automatically
     * mapped to the specified slot.
     * Note: A PXI basestation device is automatically mapped to a slot corresponding to its PXI geographic location
     *
     * @param[in] serial_number: Serial number of the basestation.
     * @param[in] slotID:   ID for slot in the PXIe chassis or the virtual slot for OneBox.
     */
    NP_EXPORT NP_ErrorCode mapBS(int serial_number, int slotID);

    /**
     * Unmaps a basestation mapped to slot.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @returns NO_SLOT if slot is not mapped.
     */
    NP_EXPORT NP_ErrorCode unmapBS(int slotID);

    typedef enum {
        NP_PARAM_INPUT_LATENCY_US = 1,  //!< Desired input buffer latency in micro-seconds
        NP_PARAM_ENABLE_HS_NPM_PH1 = 2,
        NP_PARAM_SIGNALINVERT = 7,

        // internal use only
        NP_PARAM_IGNOREPROBESN = 0x1000 /**< Disables probe serial number check for configuration files. */
    }np_parameter_t;

    /**
     * \brief Set the value of a system-wide parameter.
     */
    NP_EXPORT NP_ErrorCode setParameter(np_parameter_t paramID, int value);
    /**
     * \brief Get the value of a system-wide parameter.
     */
    NP_EXPORT NP_ErrorCode getParameter(np_parameter_t paramID, int* value);
    /**
     * \brief Set the value of a system-wide parameter.
     */
    NP_EXPORT NP_ErrorCode setParameter_double(np_parameter_t paramID, double value);
    /**
     * \brief Get the value of a system-wide parameter.
     */
    NP_EXPORT NP_ErrorCode getParameter_double(np_parameter_t paramID, double* value);

    /***** Probe Info / Metadata Functions ***************************************************/

    /**
     * \brief Get all field names linked to probe info data
     * \details The probes info data contains additional data and properties for each registered probe part-number.
     * The data comes from a spread-sheet (Probe_feature_table.xlsx). Since the column names are not constant this
     * function can be used to get all stored field names. Those field names can later on be used to aquire the actual values.
     * \code{.c}
     * size_t num_fields;
     * char** field_names = probeInfo_GetAllFieldNames(&num_fields);
     * for (int i=0; i<num_fields; i++) {
     *     printf("%s, ", field_names[i]);
     * }
     * \endcode
     * \param[out] Number of fields in returned string array
     * \returns Array of strings with field names
     */
    NP_EXPORT char** probeInfo_GetAllFieldNames(size_t* numFields);

    /**
     * \brief Query probe info by a part number search string.
     * \details Query the system by a probe product number for more probe information by using a simple search string. \n
     * The \ref qResult of type \c probeinfo_result_t is a sort iterator for the pattern matching table rows.\n
     * Functions \ref probeInfoResult_GetValueByField and \ref probeInfoResult_GetAllFieldsAndValues can be used to retrieve
     * values from the current row where the result iterator points to.
     * To advance the result iterator function \ref probeInfoResult_NextRecord is used.\n
     * <b>Query syntax</b>\n
     * The query string supports \b * and \b ? wildcards, use \c "*" to get information of all supported probes.\n
     * The question mark character is used for an arbitrary character in the search string, for example \c "NP20?3" would
     * find NP2003 and NP2013. The asterix \b * character stands for any character and length, so querying with \c "NP2*"
     * would find all Neuropixels 2.0 probes (mouse).\n
     *
     * \b Important
     * The \c probeinfo_result_t object must be deallocated when not used anymore using \ref probeInfoResult_Free
     *
     * \code{.c}
     * probeinfo_result_t q_res = NULL;
     * NP_ErrorCode err = probeInfo_QueryPn("NP1??2", &q_res);
     * \endcode
     *
     * \param[in] queryStr Part number query, accepting \c * and \c ? placeholders.
     * \param[in,out] qResult Result object of query.
     * \returns Error code, 0 on success.
     */
    NP_EXPORT NP_ErrorCode probeInfo_QueryPn(const char* queryStr, probeinfo_result_t* qResult);

    /**
     * \brief Get the number of matching part numbers of a probeInfo-query
     * \param qResult Query result to use.
     * \param[out] count Number of matches in result.
     */
    NP_EXPORT NP_ErrorCode probeInfoResult_Count(probeinfo_result_t qResult, size_t* count);

    /**
     * \brief Get the string value for a specified field-name from the current record in the query result.
     * \param qResult Query result
     * \param fieldName Field name to access
     * \param[out] fieldValue Pointer to char pointer where result should point to.
     */

    NP_EXPORT NP_ErrorCode probeInfoResult_GetValueByField(probeinfo_result_t qResult, const char* fieldName, char** fieldValue);
    /**
     * \brief Get the copied string value for a specified field-name from the current record in the query result.
     * \param qResult Query result
     * \param fieldName Field name to access
     * \param fieldValue Pointer to string where result should be copied to.
     * \param maxStrLen Maximum number of bytes available in \ref fieldValue
     */
    NP_EXPORT NP_ErrorCode probeInfoResult_GetValueByField(probeinfo_result_t qResult, const char* fieldName, char* fieldValue, const size_t maxStrLen);

    /**
     * \brief Get all field names and values of the current result-record in a probe-query result
     * \param qResult Query result
     * \param fields Pointer array of string to link field-names to.
     * \param values Pointer array of string to link values to.
     * \param num_fields Number of fields/values in record.
     *
     * \code{.c}
     char** values = NULL;
     char** fields = NULL;
     size_t num_fields;
     size_t num_res;
     probeinfo_result_t q_res = NULL;

     probeInfo_QueryPn("NP202?", &q_res);
     probeInfoResult(q_res, &num_res);
     if (num_res > 0) {
          probeInfoResult_GetAllFieldsAndValues(res, &field_names, &values, &num_fields);
          printf("%s: %s", field_names[0], values[0]);
     }
     \endcode
     */
    NP_EXPORT NP_ErrorCode probeInfoResult_GetAllFieldsAndValues(probeinfo_result_t qResult, char*** fields, char*** values, size_t* num_fields);

    /**
     * \brief Deallocate memory used by a query-result object
     * \param qResult Pointer to \c probeinfo_result_t object.
     */
    NP_EXPORT NP_ErrorCode probeInfoResult_Free(probeinfo_result_t* qResult);

    /**
     * \brief Advance to next record in query result.
     * \details Advances the internal iterator to the next record of the query result.
     * If the current record is the last record calling argument \ref success will be false.
     * Use function \ref probeInfoResult_Reset() to reset the internal iterator.
     *
     * \code{.c}
         probeinfo_result_t q_res = NULL;
         bool ok;
         char str[50] {0};
         probeInfo_QueryPn("NP20*", &q_res);
         do {
             probeInfoResult_GetValueByField(q_res, "Description", str, 50);
             printf("%s ", str);
         }
         while (probeInfoResult_NextRecord(q_res, &ok) == SUCCESS && ok==true);
     * \endcode
     *
     * \param qResult Query result
     * \param[out] success true if next record exits, false if no further record exits in query result.
     */
    NP_EXPORT NP_ErrorCode probeInfoResult_NextRecord(probeinfo_result_t qResult, bool* success);

    /**
     * \brief Reset the internal record iterator to the first record
     * \param qResult
     */
    NP_EXPORT NP_ErrorCode probeInfoResult_Reset(probeinfo_result_t qResult);

    /***** Basestation board/Slot Functions **************************************************/

    /**
     * \brief Detects a basestation on the given slot.
     *
     * @param[in] slotID: which slot in the PXIe chassis or the virtual slot for OneBox to detect.
     * @param[out] detected: True if basestation found.
     */
    NP_EXPORT NP_ErrorCode detectBS(int slotID, bool* detected);
    /**
     * \brief Opens the basestation at the specified slot.
     *        The basestation needs to be mapped to a slot first (see mapBS).
     *
     * @param[in] slotID: which slot in the PXIe chassis or the virtual slot for OneBox to open.
     */
    NP_EXPORT NP_ErrorCode openBS(int slotID);
    /**
     * \brief Closes a basestation and release all associated resources.
     *
     * @param[in] slotID: which slot in the PXIe chassis or the virtual slot for OneBox to close.
     */
    NP_EXPORT NP_ErrorCode closeBS(int slotID);
    /**
     * \brief Set the basestation in a 'armed' state. In a armed state, the basestation
     *        is ready to start acquisition after a trigger.
     *
     * @param[in] slotID: which slot in the PXIe chassis or the virtual slot for OneBox to arm
     */
    NP_EXPORT NP_ErrorCode arm(int slotID);
    /**
     * \brief Force a trigger on software trigger 1.
     *
     * @param[in] slotID: which slot in the PXIe chassis or the virtual slot for OneBox to trigger
     */
    NP_EXPORT NP_ErrorCode setSWTrigger(int slotID);

    /**
     * \brief Force software triggers on multiple software trigger channels. Onebox supports 2 trigger channels, PXI only 1
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] trigger_flags: A mask of software triggers to set.
     */
    NP_EXPORT NP_ErrorCode setSWTriggerEx(int slotID, swtriggerflags_t trigger_flags);

    /**
     * \brief Connect/disconnect a switch matrix input to/from an output signal.
     * 
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] output: Output Signal.
     * @param[in] input: Input Signal.
     * @param[in] connect:  true if to connect, false if to disconnect.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_set(int slotID, switchmatrixoutput_t output, switchmatrixinput_t input, bool connect);
    /**
     * \brief Get the switch matrix input to a output signal connection state.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_get(int slotID, switchmatrixoutput_t output, switchmatrixinput_t input, bool* is_connected);
    /**
     * \brief Clear all connections to a switch matrix output signal.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_clear(int slotID, switchmatrixoutput_t output);
    /**
     * \brief Set the inversion of the Switch Matrix input signal.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_setInputInversion(int slotID, switchmatrixinput_t input, bool invert);
    /**
     * \brief Read the Switch Matrix input signal inversion setting.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_getInputInversion(int slotID, switchmatrixinput_t input, bool* invert);
    /**
     * \brief Set the inversion of the Switch Matrix output signal.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_setOutputInversion(int slotID, switchmatrixoutput_t output, bool invert);
    /**
     * \brief Get the inversion setting of the Switch Matrix output signal.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_getOutputInversion(int slotID, switchmatrixoutput_t output, bool* invert);
    /**
     * \brief Set edge sensitivity of a switch matrix output line.
     * 
     * Only applicable to #SM_Output_WavePlayerTrigger or #SM_Output_AcquisitionTrigger.
     * 
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] output: Output signal.
     * @param[in] edge: Set the edge sensitivity to rising or falling.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_setOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t edge);
    /**
     * \brief Gets edge sensitivity of a switch matrix output line.
     */
    NP_EXPORT NP_ErrorCode switchmatrix_getOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t* edge);

    /**
     * \brief Set the frequency of the internal sync clock.
     *
     * This function will attempt to set the frequency of the internal sync clock to
     * the target frequency. Because of rounding and conversion of the frequency to
     * a half period in milliseconds, the actual frequency may differ slightly from
     * the target frequency.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] frequency: Target frequency in Hz
     * @returns PARAMETER_INVALID if the target frequency is out-of-bounds or otherwise illegal
     */
    NP_EXPORT NP_ErrorCode setSyncClockFrequency(int slotID, double frequency);

    /**
     * \brief Get the actual frequency of the internal sync clock.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] frequency: Pointer to result
     */
    NP_EXPORT NP_ErrorCode getSyncClockFrequency(int slotID, double* frequency);

    /**
     * \brief Set the period of the internal sync clock.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] period_ms: Target period in milliseconds
     * @returns PARAMETER_INVALID if period is out-of-bounds or otherwise illegal
     */
    NP_EXPORT NP_ErrorCode setSyncClockPeriod(int slotID, int period_ms);

    /**
     * \brief Get the period of the internal sync clock.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] period_ms: Pointer to result
     */
    NP_EXPORT NP_ErrorCode getSyncClockPeriod(int slotID, int* period_ms);

    typedef enum {
        NP_DATAMODE_OFF = 0, 
        NP_DATAMODE_ELECTRODE = 1, /**<Neural data is demultiplexed and gain/offset calibration data is applied*/
        NP_DATAMODE_ADC = 2 /**<Neural data is not demultiplexed and no calibration is applied (ADC calibration mode)*/
    }np_datamode_t;

    /**
     * \brief Sets the Datamode of the basestation port.
     */
    NP_EXPORT NP_ErrorCode setDataMode(int slotID, int portID, np_datamode_t mode);
    /**
     * \brief Gets the Datamode of the basestation port.
     */ 
    NP_EXPORT NP_ErrorCode getDataMode(int slotID, int portID, np_datamode_t* mode);

    /**
     * \brief Get the basestation temperature (in degrees Celsius)
     */
    NP_EXPORT NP_ErrorCode bs_getTemperature(int slotID, double* temperature_degC);
    /**
     * \brief Get the basestation firmware version info
     */
    NP_EXPORT NP_ErrorCode bs_getFirmwareInfo(int slotID, struct firmware_Info* info);

    /**
     * \brief Update the basestation firmware.
     *
     * @param[in] slotID: ID for slot in the PXIe.
     * @param[in] filename: Firmware binary file.
     * @param[in] callback: (optional, may be null). Progress callback function. if callback returns 0, the update aborts.
     */
    NP_EXPORT NP_ErrorCode bs_updateFirmware(int slotID, const char* filename, int (*callback)(size_t bytes_written));

    /**
     * \brief sets the firmware for the basestation version to the required version.
     *
     * @param[in] slotID: ID for slot in the PXIe.
     * @param[in] callback: (optional, may be null). Progress callback function. if callback returns 0, the update
     * aborts.
     */
    NP_EXPORT NP_ErrorCode bs_resetFirmware(int slotID, int (*callback)(size_t bytes_written));

    /**
     * \brief Get the size of the built-in firmware blob for the basestation.
     *
     * @param[in] slotID: ID for slot in the PXIe.
     * @param[in] size: Size of firmware blob in bytes.
     * @returns SUCCESS if successful,\n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot,\n
     *          NOTSUPPORTED if the slot is not a PXI slot,\n
     *          FAILED if the type of basestation could not be established,\n
     *          PARAMETER_INVALID if size is NULL.
     */
    NP_EXPORT NP_ErrorCode bs_getFirmwareSize(int slotID, size_t* size);

    /**
     * \brief (Only on PXI platform) Get the basestation connect board temperature (in degrees Celsius).
     */
    NP_EXPORT NP_ErrorCode bsc_getTemperature(int slotID, double* temperature_degC);
    /**
     * \brief (Only on PXI platform) Get the basestation connect board firmware version info.
     */
    NP_EXPORT NP_ErrorCode bsc_getFirmwareInfo(int slotID, struct firmware_Info* info);

    /**
     * \brief Update the basestation connect board firmware.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis.
     * @param[in] filename: Firmware binary file.
     * @param[in] callback: (optional, may be null). Progress callback function. if callback returns 0, the update aborts.
     * @returns SUCCESS if successful, PROGRAMMINGABORTED if aborted by the callback returning 0, PROGRAMMING_FAILED if the firmware image was incorrect after programming
     */
    NP_EXPORT NP_ErrorCode bsc_updateFirmware(int slotID, const char* filename, int (*callback)(size_t bytes_written));

    /**
     * \brief sets the firmware version of the basestation connect board to the required version.
     *
     * @param[in] slotID: ID for slot in the PXIe.
     * @param[in] callback: (optional, may be null). Progress callback function. if callback returns 0, the update
     * aborts.
     */
    NP_EXPORT NP_ErrorCode bsc_resetFirmware(int slotID, int (*callback)(size_t bytes_written));

    /**
     * \brief Get the size of the built-in firmware blob for the basestation connect board.
     *
     * @param[in] slotID: ID for slot in the PXIe.
     * @param[in] size: Size of firmware blob in bytes.
     * @returns SUCCESS if successful,\n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot,\n
     *          NOTSUPPORTED if the slot is not a PXI slot,\n
     *          FAILED if the type of BSC could not be established,\n
     *          PARAMETER_INVALID if size is NULL.
     */
    NP_EXPORT NP_ErrorCode bsc_getFirmwareSize(int slotID, size_t* size);

    /**
     * @brief Get a OneBox's firmware version info
     * @param slotID Target OneBox slot
     * @param[out] info Firmware version info
     * @return NOTSUPPORTED if target slot is not a OneBox
     */
    NP_EXPORT NP_ErrorCode ob_getFirmwareInfo(int slotID, struct firmware_Info* info);

    /**
     * \brief Read firmware metadata from headstage.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID for basestation port. Valid range 1 to 4 for PXIe, 1 to 2 for OneBox
     * @param[out] info: Pointer to result.
     */
    NP_EXPORT NP_ErrorCode hs_getFirmwareInfo(int slotID, int portID, firmware_Info* info);

    /**
     * \brief Update the headstage firmware.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID for basestation port. Valid range 1 to 4 for PXIe, 1 to 2 for OneBox
     * @param[in] filename: Firmware binary file.
     * @param[in] read_check: Read back firmware and compare.
     */
    NP_EXPORT NP_ErrorCode hs_updateFirmware(int slotID, int portID, const char* filename, bool read_check);

    /**
     * \brief Get the amount of ports on the basestation connect board.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] count: output parameter, amount of available ports.
     * @returns SUCCESS if successful.
     */
    NP_EXPORT NP_ErrorCode getBSCSupportedPortCount(int slotID, int* count);
    /**
     * \brief Get the amount of probes the connected headstage supports
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: portID (1..4)
     * @param[out] count: output parameter, amount of flex/probes
     * @returns SUCCESS if successful.
     */
    NP_EXPORT NP_ErrorCode getHSSupportedProbeCount(int slotID, int portID, int* count);

    /**
     * \brief Opens the port on the basestation
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID for basestation port. Valid range 1 to 4 for PXIe, 1 to 2 for OneBox
     * @returns SUCCESS if successful.
     */
    NP_EXPORT NP_ErrorCode openPort(int slotID, int portID);
    /**
     * \brief Closes the port on the basestation
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID for basestation port. Valid range 1 to 4 for PXIe, 1 to 2 for OneBox
     * @returns SUCCESS if successful.
     */
    NP_EXPORT NP_ErrorCode closePort(int slotID, int portID);
    /**
     * \brief Detects if a headstage is present or not.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID for basestation port. Valid range 1 to 4 for PXIe, 1 to 2 for OneBox.
     * @param[out] detected: true if headstage is detected, false if not.
     * @returns SUCCESS if successful.
     */
    NP_EXPORT NP_ErrorCode detectHeadStage(int slotID, int portID, bool* detected);
    /**
     * \brief Detects if flex is present or not.
     *
     * @param[in] slotID: ID for slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID for basestation port. Valid range 1 to 4 for PXIe, 1 to 2 for OneBox
     * @param[in] dockID: ID of the dock of the probe on the headstage.
     * @param[out] detected: True if headstage is detected, false if not.
     * @returns SUCCESS if successful.
     */
    NP_EXPORT NP_ErrorCode detectFlex(int slotID, int portID, int dockID, bool* detected);
    /**
     * \brief Enable or disable the headstage LED.
     */
    NP_EXPORT NP_ErrorCode setHSLed(int slotID, int portID, bool enable);

    /***** Probe functions *******************************************************************/

    /**
     * \brief Opens the connection to the probe.
     *
     * @param[in] slotID: ID of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID of the port on the basestation.
     * @param[in] dockID: ID of the dock of the probe on the headstage.
     */
    NP_EXPORT NP_ErrorCode openProbe(int slotID, int portID, int dockID);
    /**
     * \brief Closes the connection to the probe.
     */
    NP_EXPORT NP_ErrorCode closeProbe(int slotID, int portID, int dockID);
    /**
     * \brief Resets the connected probe to the default settings.
     * 
     * Loads the default settings for configuration registers and memory map; and subsequently initialize the probe in recording mode:
     *  - Configure the probe shank to the default electrode configuration.
     *  - Configure the probe base to the default channel configuration.
     */
    NP_EXPORT NP_ErrorCode init(int slotID, int portID, int dockID);
    
    /**
     * \brief Writes the probe configuration
     *
     * The user needs to call this function to apply the change that was made in:
     *   - electrode selection,
     *   - electrode group selection,
     *   - electrode column selection,
     *   - reference selection,
     *   - gain setting,
     *   - bandwidth setting,
     *   - standby setting
     *
     * @param[in] slotID: ID of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID of the port on the basestation.
     * @param[in] dockID: ID of the dock of the probe on the headstage.
     * @param[in] read_check: verify the configuration if true, not if false.
     */
    NP_EXPORT NP_ErrorCode writeProbeConfiguration(int slotID, int portID, int dockID, bool read_check);
    
    /**
     * @brief Commited channel configuration
     */
    typedef struct {
        int channel_index; /**< Index of the channel (zero-indexed) */
        electrodebanks_t bank_mask; /**< Mask of selected banks for the channel */
        int bank_shank; /**< Shank containing selected banks */
        channelreference_t reference; /**< Selected reference input */
        int shank_reference; /**< Shank containing reference input */
        int int_ref_electrode_bank; /**< Selected internal reference electrode, or -1 if
                                         probe has no internal reference or other reference is used. */
        bool standby_enabled; /**< True if standby is enabled. */
        bool high_pass_disabled; /**< True for disabling the 300Hz high-pass cut-off filter,
                                      false for enabling (NP1 only) */
        int ap_gain_select; /**< Selected AP gain value (NP1 only) */
        int lfp_gain_select; /**< Selected LFP gain value (NP1 only) */
    } ProbeChannelConfiguration;

    /**
     * @brief Gets last successfully written probe configuration
     *
     * Channel configuration details are in the form of the ProbeChannelConfiguration struct.
     * Although this struct contains a `channel_index` field, the index of the struct in
     * the `configuration` array will match the channel so configuration[0] contains
     * the configuration of channel 0, and so on.
     *
     * Note that when during the last call of `writeProbeConfiguration` something went wrong,
     * the configuration returned by this function may not be valid as parts of it may have
     * been overwritten during the earlier failed attempt.
     *
     * \note This function is not supported for UHD2 probes.
     *
     * @param[in] slotID: ID of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: ID of the port on the basestation.
     * @param[in] dockID: ID of the dock of the probe on the headstage.
     * @param[in] channel_count: Number of channels to get the configuration for.
     * @param[out] configuration: Array of ProbeChannelConfiguration structs (must match channel_count in size).
     * @return NODATA if no configuration committed.
     */
    NP_EXPORT NP_ErrorCode getCommittedProbeConfiguration(
        int slotID, int portID, int dockID, int channel_count, ProbeChannelConfiguration* configuration);

    /**
     * \brief Loads the ADC calibration parameters from the provided calibration file to the probe.
     *
     * \note this function is not valid for Neuropixel 2.0 probes.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] filename: the filename for the calibration file.
     */
    NP_EXPORT NP_ErrorCode setADCCalibration(int slotID, int portID, const char* filename);
    /**
     * \brief Loads the gain correction parameters from the provided calibration csv file to the probe.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] dockID: Id of the dock of the probe on the headstage.
     * @param[in] filename: the filename for the calibration file.
     */
    NP_EXPORT NP_ErrorCode setGainCalibration(int slotID, int portID, int dockID, const char* filename);

// <NP1 Specific>
#define NP1_PROBE_CHANNEL_COUNT 384
#define NP1_PROBE_SUPERFRAMESIZE 12
#define NP1_PROBE_ADC_COUNT 32
#define NP1_PROBE_ADC_SAMPLE_RATE 390'000

// These macros are for the status byte
#define ELECTRODEPACKET_STATUS_TRIGGER    (1<<0)
#define ELECTRODEPACKET_STATUS_SYNC       (1<<6)
#define ELECTRODEPACKET_STATUS_LFP        (1<<1)
#define ELECTRODEPACKET_STATUS_ERR_COUNT  (1<<2)
#define ELECTRODEPACKET_STATUS_ERR_SERDES (1<<3)
#define ELECTRODEPACKET_STATUS_ERR_LOCK   (1<<4)
#define ELECTRODEPACKET_STATUS_ERR_POP    (1<<5)
#define ELECTRODEPACKET_STATUS_ERR_SYNC   (1<<7)

    struct electrodePacket {
        uint32_t timestamp[NP1_PROBE_SUPERFRAMESIZE]; /**< samples from the 100kHz timestamp clock*/
        int16_t apData[NP1_PROBE_SUPERFRAMESIZE][NP1_PROBE_CHANNEL_COUNT];  /**< samples for each AP channel*/
        int16_t lfpData[NP1_PROBE_CHANNEL_COUNT]; /**< samples for each LFP channel*/
        uint16_t Status[NP1_PROBE_SUPERFRAMESIZE]; /**< samples of the status byte*/
    };

    /**
     * \brief reads the data out in electrodePacket format: 12 samples of AP data, 1 sample of LFP data,
     *
     * \note not valid for Neuropixels 2.0 probes.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] dockID: Id of the dock of the probe on the headstage.
     * @param[out] packets: packets of data in format electrodePacket.
     * @param[out] actual_amount: the amount of data packets received.
     * @param[out] requested_amount: the requested amount of data packets.
     */
    NP_EXPORT NP_ErrorCode readElectrodeData(int slotID, int portID, int dockID, struct electrodePacket* packets, int* actual_amount, int requested_amount);
    /**
     * \brief reads the amount of packets available in the RAM FIFO.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] dockID: Id of the dock of the probe on the headstage.
     * @param[out] packets_available: returns the amount unread of packets in the FIFO. NULL allowed for no return.
     * @param[out] headroom: returns the amount of space left in the FIFO. NULL allowed for no return.
     */
    NP_EXPORT NP_ErrorCode getElectrodeDataFifoState (int slotID, int portID, int dockID, int* packets_available, int* headroom);
// </NP1 Specific>

    /**
     * \brief turns on the test signal on the headstage.
     * 
     * The headstage contains a test signal generator which can be used to test the probes' functionality. 
     *
     * \note Only valid for Neuropixels 1.0 probes.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] dockID: Id of the dock of the probe on the headstage.
     * @param[in] enable: enables the test signal
     */
    NP_EXPORT NP_ErrorCode setTestSignal(int slotID, int portID, int dockID, bool enable);
    /**
     * \brief Sets the operating mode of the probe. 
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] dockID: Id of the dock of the probe on the headstage.
     * @param[in] mode: operating mode of the probe.
     */
    NP_EXPORT NP_ErrorCode setOPMODE(int slotID, int portID, int dockID, probe_opmode_t mode);

    /**
     * \brief Sets the calibration mode of the probe
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] dockID: Id of the dock of the probe on the headstage.
     * @param[in] mode: calibration mode
     */
    NP_EXPORT NP_ErrorCode setCALMODE(int slotID, int portID, int dockID, testinputmode_t mode);
    /**
     * \brief sets the REC_NRESET signal to the probe. This signal sets the probes' channel in reset state.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation.
     * @param[in] state: requested REC_NRESET state
     */
    NP_EXPORT NP_ErrorCode setREC_NRESET(int slotID, int portID, bool state);

    /**
     * \brief Read a single packet data from the specified fifo.
     *        This is a non blocking function that tries to read a single packet
     *        from the specified receive fifo.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] source: Select the source type from the NP1.0 probe (SourceAP or SourceLFP) or the base source for the NP2.0 quad-base probe.
     *                    Ignored if the probe does not support multiple sources.
     * @param[out] pck_info: Output data containing additional packet data: timestamp, stream status, and payload length.
     * @param[out] data: unpacked 16 bit right aligned data.
     * @param[out] requested_channel_count: size of data buffer (maximum amount of channels).
     * @param[out] actual_read: optional output parameter that returns the amount of channels unpacked for a single timestamp.
     * @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0).
     */
    NP_EXPORT NP_ErrorCode readPacket(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pck_info, int16_t* data, int requested_channel_count, int* actual_read);

    /**
     * \brief Read multiple packets from the specified fifo.
     *        This is a non blocking function.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] source: Select the source type from the NP1.0 probe (SourceAP or SourceLFP) or the base source for the NP2.0 quad-base probe.
     *                    Ignored if the probe does not support multiple sources.
     * @param[out] pck_info: Output data containing additional packet data: timestamp, stream status, and payload length.
     *                       Size of this buffer is expected to be sizeof(struct PacketInfo)*packet_count.
     * @param[out] data: Unpacked 16 bit right aligned data. size of this buffer is expected to be 'channel_count*packet_count*sizeof(int16_t)'.
     * @param[out] channel_count: Number of channels to read per packet. This value is also the data stride value in the result 'data' buffer.
     * @param[out] packet_count: Maximum number of packets to read per call.
     * @param[out] packets_read: Number of packets read from the fifo.
     * @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0).
     */
    NP_EXPORT NP_ErrorCode readPackets(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pck_info, int16_t* data, int channel_count, int packet_count, int* packets_read);

    /**
     * \brief The amount of packets in the RAM FIFO.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] source: Select the stream source from the probe (SourceAP or SourceLFP). Ignored if probe does not support multiple sources.
     * @param[out] packets_available: returns the number of unread packets in the FIFO. NULL allowed for no return.
     * @param[out] headroom: returns the amount of space left in the FIFO. NULL allowed for no return.
     */
    NP_EXPORT NP_ErrorCode getPacketFifoStatus(int slotID, int portID, int dockID, streamsource_t source, int* packets_available, int* headroom);

    /*********** Probe Channel functions ***********************************************************/
    /**
     * \brief Set the gain of each channel individually. 
     *        The gain of the AP and LFP band can be set separately. 
     * 
     * AP and LFP gain can be selected from 8 predefined values:
     * 
     * | gain parameter | Gain |
     * |----------------|------|
     * |  0             | 50   |
     * |  1             | 125  |
     * |  2             | 250  |
     * |  3             | 500  |
     * |  4             | 1000 |
     * |  5             | 1500 |
     * |  6             | 2000 |
     * |  7             | 3000 |
     * 
     * It also applies gain correction factors as applied in the BSC FPGA. 
     * 
     * It will not apply this change to the probe until 'writeProbeConfiguration' is called.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The channel number. (0 to 383)
     * @param[in] ap_gain_select: The AP gain value. (0 to 7)
     * @param[in] lfg_gain_select: The LFP gain value. (0 to 7)
     */
    NP_EXPORT NP_ErrorCode setGain(int slotID, int portID, int dockID, int channel, int ap_gain_select, int lfg_gain_select);
    /**
     * \brief Gets the gain parameters from the given channel.
     * 
     * \note This function is not valid for Neuropixels 2.0 probes.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The channel number. (0 to 383)
     * @param[out] ap_gain_select: The AP gain value. (0 to 7)
     * @param[out] lfg_gain_select: The LFP gain value. (0 to 7)
     */
    NP_EXPORT NP_ErrorCode getGain(int slotID, int portID, int dockID, int channel, int* ap_gain_select, int* lfg_gain_select);
    /**
     * \brief Maps an electrode bank to a channel. 
     * 
     * \note This function is not valid for Neuropixels 2.0 probes.
     * 
     * This is dependent on probe type, please check the manual for allowed combinations.
     * 
     * Setting the bank parameter to 0xFF clears all previous connections made. 
     * 
     * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The channel number. (0 to 383)
     * @param[in] shank: The shank number (valid range 0 to 3, depending on probe type).
     * @param[in] bank: The electrode bank number to connect to (valid range: 0 to 11, depending on probe type, or 0xFF).
     */
    NP_EXPORT NP_ErrorCode selectElectrode(int slotID, int portID, int dockID, int channel, int shank, int bank);
    /**
     * \brief Maps multiple electrode banks to a channel.
     *
     * This is dependent on probe type, please check the manual for allowed combinations.
     *
     * setting the bank parameter to 0xFF clears all previous connections made.
     * 
     * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The channel number. (0 to 383)
     * @param[in] shank: The shank number (valid range 0 to 3, depending on probe type).
     * @param[in] bank_mask: Electrode bank numbers to connect. (BankA to BankL, depending on probe type)
     */
    NP_EXPORT NP_ErrorCode selectElectrodeMask(int slotID, int portID, int dockID, int channel, int shank, electrodebanks_t bank_mask);
    /**
     * \brief Set the reference input per probe
     *
     * These available reference lines are:
     *  - External reference electrode
     *  - Tip electrode
     *  - Internal reference line
     *  - Ground reference
     *  - No reference
     *
     * Please refer to the specific probe manuals for allowed combinations.
     *
     * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The channel number. (0 to 383)
     * @param[in] shank: The shank number (valid range 0 to 3, depending on probe type).
     * @param[in] reference: the selected reference input (valid range: 0 to 3, depending on probe type).
     * @param[in] int_ref_electrode_bank: the selected internal reference electrode (valid range: 0 to 3 or 0xFF, depending on probe type).
     */
    NP_EXPORT NP_ErrorCode setReference(int slotID, int portID, int dockID, int channel, int shank, channelreference_t reference, int int_ref_electrode_bank);
    /**
     * \brief Sets the high-pass corner frequency can be programmed for each AP channel individually.
     *
     * \note This function is not valid for Neuropixels 2.0 probes.
     *
     * The high-pass corner frequency setting of the probe is
     * programmed via the probes' base configuration register.
     * 
     * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The channel number. (0 to 383)
     * @param[in] disable_high_pass: true for disabling the 300Hz high-pass cut-off filter, false for enabling
     */
    NP_EXPORT NP_ErrorCode setAPCornerFrequency(int slotID, int portID, int dockID, int channel, bool disable_high_pass);

    /**
     * \brief set each channel individually in stand-by mode.
     *
     * In standby mode the channel amplifiers are disabled.
     * The channel is still output on the PSB data bus.
     *
      * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The channel number. (0 to 383)
     * @param[in] standby: True for stand-by.
     */
    NP_EXPORT NP_ErrorCode setStdb(int slotID, int portID, int dockID, int channel, bool standby);

    /**
     * \brief Enables/disables half the electrode columns on the shank.
     *
     * \note These functions are only valid for Neuropixels UHD2 probes.
     * 
     * Before selecting the connection between channel groups and electrodes,
     * the user must first set which column pattern of electrodes on the shank is activated.
     * If the function is called, all existing channel group to electrode group connections are removed.
     * 
     * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] pattern: The electrode column patterns to enable.
     */
    NP_EXPORT NP_ErrorCode selectColumnPattern(int slotID, int portID, int dockID, columnpattern_t pattern);

    /**
     * \brief Connect a single bank to a channel group.
     *
     * \note These functions are only valid for Neuropixels UHD2 probes.
     * 
     * The following function is used to connect a single bank to a channel group.
     * Calling this function disconnects any previously connected bank(s) to the selected channel group.
     * If the parameter 'bank' is set to 0xFF the channel_group is disconnected from all banks.
     * 
     * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel_group: Channel group to connect, can have value from 0 to 23.
     * @param[in] bank: Bank to connect, can have value from 0 to 15.
     */
    NP_EXPORT NP_ErrorCode selectElectrodeGroup(int slotID, int portID, int dockID, int channel_group, int bank);
    /**
     * \brief Connect a maximum of two banks to channel group.
     *
     * \note These functions are only valid for Neuropixels UHD2 probes.
     * 
     * In the case the EN_A or EN_B bit in the shank register is set low,
     * it is allowed to connect a channel group to maximum two banks.
     * 
     * It will not apply this change to the probe until '\ref writeProbeConfiguration' is called.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel_group: Channel group to connect, can have value from 0 to 23.
     * @param[in] mask: Banks to connect, combining maximum two values.
     */
    NP_EXPORT NP_ErrorCode selectElectrodeGroupMask(int slotID, int portID, int dockID, int channel_group, electrodebanks_t mask);

    /** Onebox AUXilary IO functions */

    // Onebox Waveplayer
    /**
     * \brief Write to the Waveplayer's sample buffer.
     * 
     * The function returns a warning via the message log if the maximum number of samples is exceeded
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] data: A buffer of length (len) of 16 bit signed data samples.
     * @param[in] len: Amount of samples in 'data'. If len is 0 the current sample data is invalidated and calling \ref waveplayer_arm will fail.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device. NODATA if data-length is 0.

     */
    NP_EXPORT NP_ErrorCode waveplayer_writeBuffer(int slotID, const int16_t* data, unsigned int len);

    /**
     * \brief Arm the Waveplayer.
     *
     * This prepares the output SMA channel to playback the waveform programmed with '\ref waveplayer_writeBuffer' by routing output DAC0 to the SMA output and re-setting it to 0 V,
     * even when no sample-data was set by '\ref waveplayer_writeBuffer'.
     * The Waveplayer playback is triggered by a signal in the switch matrix (see \ref switchmatrix_set) or by the software trigger.
     * By default, \ref SM_Input_SWTrigger2 is bound as the WavePlayer software trigger.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] single_shot: If true, the Waveplayer will play the programmed waveform once after trigger. if false, the waveform is repeat until rearmed in other mode or \ref waveplayer_stop is called.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device. NODATA if no sample data has been assigned using '\ref waveplayer_writeBuffer'.
     */
    NP_EXPORT NP_ErrorCode waveplayer_arm(int slotID, bool single_shot);

    /**
     * \brief Stop playback of Waveplayer
     *
     * Stops playback of Waveplayer, disarms waveplayer, disables the waveplayer-trigger and resets the output to 0 V.
     *
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode waveplayer_stop(int slotID);

    /**
     * \brief Set the actual Waveplayer's sampling frequency in Hz.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] frequency_Hz: The target sample frequency in Hz.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode waveplayer_setSampleFrequency(int slotID, double frequency_Hz);

    /**
     * \brief Get the actual Waveplayer's sampling frequency in Hz
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] frequency_Hz: The target sample frequency in Hz.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode waveplayer_getSampleFrequency(int slotID, double* frequency_Hz);

    /**
     * \brief Directly reads the voltage of a particular ADC channel.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] ADC_channel: The ADC channel to read the data from (valid range 0 to 11).
     * @param[out] voltage: Return voltage of the ADC Channel in volts.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode ADC_read(int slotID, int ADC_channel, double* voltage);

    /**
     * \brief Directly reads the ADC comparator output state.
     *
     * The low/high comparator threshold values can be set using #ADC_setComparatorThreshold.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] ADC_channel: The ADC channel to read the data from (valid range 0 to 11).
     * @param[out] state: Returns the comparator output state. True for high, false for low.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode ADC_readComparator(int slotID, int ADC_channel, bool* state);

    /**
     * \brief Directly reads the ADC comparator state of all ADC channels in a single output word.
     *
     * The low/high comparator threshold values can be set using #ADC_setComparatorThreshold.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] flags: A word containing the comparator state of each ADC channel (bit0 = ADCCH0, bit 1 = ADCCH1, etc...).
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode ADC_readComparators(int slotID, uint32_t* flags);

    /**
     * \brief Enable or disables the auxiliary ADC probe.
     *
     * Regardless of whether or not the ADC probe is enabled, the OneBox will actively sample the ADC.
     * When the ADC probe is enabled however, a stream of ADC and comparator data is sent to the PC together
     * with other data streams such as those from neural probes.
     * Data from the ADC and comparators can also be read using functions like #ADC_read and #ADC_readComparator,
     * even if the ADC probe is disabled.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] enable: True to enable, false to disable.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode ADC_enableProbe(int slotID, bool enable);

    /**
     * \brief Get the LSB to voltage conversion factor and bit depth for the ADC probe channel.
     *
     * This conversion changes with programmed ADC range (via #ADC_setVoltageRange).
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] lsb_to_voltage: Conversion factor to convert 16 bit signed value to voltage.
     * @param[out] bit_depth: Optional return value that indicates the number of bits in the ADC stream.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode ADC_getStreamConversionFactor(int slotID, double* lsb_to_voltage, int* bit_depth);

    /**
     * \brief Set the ADC comparator low/high threshold voltages per channel.
     * 
     * Default comparator levels are:
     * - Comparator Threshold Low: 0.799789 V
     * - Comparator Threshold High: 1.99974 V
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] ADC_channel: ADC channel (valid range 0 to 11).
     * @param[in] v_low: Low comparator threshold voltage. Comparator state will toggle to 0 if the input is below this value.
     * @param[in] v_high: High comparator threshold voltage. Comparator state will toggle to 1 if the input is above this value.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode ADC_setComparatorThreshold(int slotID, int ADC_channel, double v_low, double v_high);

    /**
     * \brief Get the programmed ADC comparator low/high threshold voltages.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] ADC_channel: ADC channel (valid range 0 to 11).
     * @param[out] v_low: Get the low comparator threshold voltage. Comparator state will toggle to 0 if the input is below this value.
     * @param[out] v_high: Get the high comparator threshold voltage. Comparator state will toggle to 1 if the input is above this value.
     * @returns SUCCESS if successful. NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode ADC_getComparatorThreshold(int slotID, int ADC_channel, double* v_low, double* v_high);

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
     * \brief Set the ADC Voltage range.
     *
     * This voltage range is used for all ADC channels.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] range: Programmed range will be [-range .. +range].
     * @return SUCCESS if successful.
     *         NOTSUPPORTED if this functionality is not supported by the device.
     *         PARAMETER_INVALID if the range is not supported by the device
     */
    NP_EXPORT NP_ErrorCode ADC_setVoltageRange(int slotID, ADCrange_t range);

    /**
     * \brief Get the programmed ADC Voltage range.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] range: Programmed range will be [-range .. +range].
     * @return SUCCESS if successful.
     *         NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode ADC_getVoltageRange(int slotID, ADCrange_t* range);

    /**
     * \brief Set a DAC channel to a fixed voltage.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] DAC_channel: The DAC channel to configure.
     * @param[in] voltage: The requested fixed output voltage in volts.
     * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode DAC_setVoltage(int slotID, int DAC_channel, double voltage);

    /**
     * \brief Set multiple DAC output's voltages.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] DAC_channel_mask: Bit mask indicating which channels to set voltage, lowest bit (0x01) is DAC0.
     * @param[in] voltages: Pointer to array with 12 voltage entries.
     * @returns SUCCESS if successful. PARAMETER_INVALID if a voltage setting is out of bounds [-5V, 5V], NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode DAC_setVoltages(int slotID, uint16_t DAC_channel_mask, double* voltages);


    /**
     * \brief Set a DAC channel in digital tracking mode, and program its low and high voltage.
     *
     * In this mode, the DAC channel acts as an output of the switch matrix (See #switchmatrix_set).
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] DAC_channel: The DAC channel to configure.
     * @param[in] v_high: DAC voltage for Digital 'H'. default is 5 volt.
     * @param[in] v_low: DAC voltage for Digital 'L'. default is 0 volt.
     * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode DAC_setDigitalLevels(int slotID, int DAC_channel, double v_high, double v_low);

    /**
     * \brief Enable DAC channel output on SDR connector.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] DAC_channel: The DAC channel to configure.
     * @param[in] state: True to enable output, false for high impedance.
     * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device
     */
    NP_EXPORT NP_ErrorCode DAC_enableOutput(int slotID, int DAC_channel, bool state);

    /**
     * \brief Set a DAC channel in probe sniffing mode.
     *
     * The output of the DAC will now track a programmed probe channel.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] DAC_channel: The DAC channel to configure.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in] channel: The probe's channel nr that will be tracked.
     * @param[in] source_type: Source stream. (Default AP)
     * @returns SUCCESS if successful. WRONG_DACCHANNEL if channel out of bound, NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode DAC_setProbeSniffer(int slotID, int DAC_channel, int portID, int dockID, int channel, streamsource_t source_type);

    /**
     * \brief Read multiple packets from the auxiliary ADC probe stream (non-blocking and thread-safe).
     *
     * The OneBox actively samples an onboard 12-channel ADC to which external signals can be connected using the SDR connector.
     * Each ADC channel is also fed into a comparator which can be configured through #ADC_setComparatorThreshold.
     * When enabled through #ADC_enableProbe, the OneBox will send a packet stream of ADC and comparator data to the computer,
     * which can be read using this function.
     *
     * Each packet of the stream contains 24 samples:
     * - the first 12 samples are the ADC values (counts) for each channel.
     * - the last 12 samples are the ADC comparator values for each ADC channel.
     *
     * It is possible to only read the first N samples from each packet by setting the \p channel_count parameter to a value lower than 24.
     *
     * The ADC counts, which are 16-bit signed integers, can be converted to voltages by multiplying them with a conversion factor.
     * This conversion factor can be retrieved using #ADC_getStreamConversionFactor.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] pck_info: Output data containing additional packet data: timestamp, stream status, and payload length.
     *                      size of this buffer is expected to be sizeof(struct PacketInfo)*packet_count.
     * @param[out] data: Unpacked 16 bit right aligned data. Size of this buffer is expected to be 'channel_count*packet_count*sizeof(int16_t)'.
     * @param[in] channel_count: Number of channels to read per packet. This value is also the data stride value in the result 'data' buffer. Onebox supports 12 ADC channels + 12 comparator channels.
     * @param[in] packet_count: Maximum number of packets to read per call.
     * @param[out] packets_read: Number of packets read from the fifo.
     * @returns SUCCESS if successful. Note that this function also returns SUCCESS if no data was available (samplesread returns ==0). NOTSUPPORTED if this functionality is not supported by the device.
     */
    NP_EXPORT NP_ErrorCode ADC_readPackets(int slotID, struct PacketInfo* pck_info, int16_t* data, int channel_count, int packet_count, int* packets_read);

    /**
     * \brief Get status (available packets and remaining capacity) of auxiliary ADC probe stream FIFO.
     *
     * This is a non-blocking and thread-safe function.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[out] packets_avaialble: number of packets available for read.
     * @param[out] headroom: remaining capacity of the FIFO.
     */
    NP_EXPORT NP_ErrorCode ADC_getPacketFifoStatus(int slotID, int* packets_avaialble, int* headroom);

    /******************** Built In Self Test ****************************/
    /**
     * \brief Basestation platform BIST (Built-In Self-Test).
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @returns SUCCESS if successful,
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     */
    NP_EXPORT NP_ErrorCode bistBS(int slotID);

    /**
     * \brief headstage heartbeat (HB) test
     *
     * The heartbeat signal generated by the PSB_SYNC signal of the probe. The PSB_SYNC signal starts when the probe is powered on,
     * the OP_MODE register in the probes' memory map set to 1, and the REC_NRESET signal set high.
     * The heartbeat signal is visible on the headstage (can be disabled by API functions) and on the BSC.
     * This is in the first place a visual check. In order to facilitate a software check of the BSC heartbeat signal,
     * the PSB_SYNC signal is also routed to the BS FPGA. A function is provided to check whether the PSB_SYNC signal contains a 0.5Hz clock.
     * The presence of a heartbeat signal acknowledges the functionality of the power supplies on the headstage for serializer and probe, the POR signal,
     * the presence of the master clock signal on the probe, the functionality of the clock divider on the probe, an basic communication over the serdes link.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @returns SUCCESS if successful, \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered. \n
     */
    NP_EXPORT NP_ErrorCode bistHB(int slotID, int portID, int dockID);

    /**
     * \brief Start Serdes PRBS test.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @returns SUCCESS if successful, \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered. \n
     */
    NP_EXPORT NP_ErrorCode bistStartPRBS(int slotID, int portID);

    /**
     * @brief Stop Serdes PRBS test.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[out] prbs_err: The number of prbs errors.
     * @returns SUCCESS if successful, \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered. \n
     */
    NP_EXPORT NP_ErrorCode bistStopPRBS(int slotID, int portID, int* prbs_err);

    /**
     * @brief Read intermediate Serdes PRBS test result
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[out] prbs_err: The number of PRBS errors.
     * @returns SUCCESS if successful, \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered. \n
     */
    NP_EXPORT NP_ErrorCode bistReadPRBS(int slotID, int portID, int* prbs_err);

    /**
     * \brief Test I2C memory map access.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @returns SUCCESS if successful, \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropixel card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          in case a port number outside the valid range is entered, \n
     *          NO_ACK in case no acknowledgment is received, \n
     *          READBACK_ERROR in case the written and read-back word are not the same. \n
     */
    NP_EXPORT NP_ErrorCode bistI2CMM(int slotID, int portID, int dockID);

    /**
     * \brief Test all EEPROMs (Flex, headstage, BSC). by verifying a write/readback cycle on an unused memory location.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @returns SUCCESS if successful, \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered, \n
     *          NO_ACK_FLEX in case no acknowledgment is received from the flex eeprom, \n
     *          READBACK_ERROR_FLEX in case the written and read-back word are not the same from the flex eeprom, \n
     *          NO_ACK_HS in case no acknowledgment is received from the HS eeprom, \n
     *          READBACK_ERROR_HS in case the written and read-back word are not the same from the HS eeprom, \n
     *          NO_ACK_BSC in case no acknowledgment is received from the BSC eeprom, \n
     *          READBACK_ERROR_BSC in case the written and read-back word are not the same from the BSC eeprom. \n
     */
    NP_EXPORT NP_ErrorCode bistEEPROM(int slotID, int portID);

    /**
     * \brief Test the shift registers
     *
     * This test verifies the functionality of the shank and base shift registers.
     * The function configures the shift register two or more times with the same code.
     * After the 2nd write cycle the SR_OUT_OK bit in the STATUS register is read.
     * If OK, the shift register configuration was successful.
     * The test is done for all applicable SR chain registers.
     * The configuration code used for the test is a dedicated code (to make sure the bits are not all 0 or 1).
     * Optional parameter \ref shanksOkMask can be used to determine which shanks have passed and which have failed.
     * Each bit in the returned mask stands for a shank starting with bit 0 for shank 1.
     * A set bit (1) means the shank passed the test.
     *
     * Note 1: this test overwrites and does not restore the data stored in the SR chains before the test.
     * Use writeProbeConfiguration to restore the configuration.
     *
     * Note 2: with 2C and other multi-shank probes the test will be executed for all SR chains of
     * shanks **unless** unexpected failures occur. This means that the value of the shanksOkMask
     * is valid if and only if the test returns SUCCESS or ERROR_SR_CHAIN. Should it return any
     * other error code, such as TIMEOUT, the test will have been aborted.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @param[in,out] shanksOkMask: If pointer is set (not NULL) it will return a bit-mask for each shank that passed
     * @returns SUCCESS if successful,\n
     *          NO_LINK if no datalink,\n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot,\n
     *          TIMEOUT when there are communication failures,\n
     *          WRONG_SLOT in case a slot number outside the valid range is entered,\n
     *          WRONG_PORT in case a port number outside the valid range is entered,\n
     *          ERROR_SR_CHAIN in case the SR_OUT_OK bit is not okay.\n
     */
    NP_EXPORT NP_ErrorCode bistSR(int slotID, int portID, int dockID, uint8_t* shanksOkMask = NULL);

    /**
     * \brief Test the PSB bus on the headstage.
     *
     * A test mode is implemented on the probe which enables the transmission of a known data pattern over the PSB bus.
     * The following function sets the probe in this test mode, records a small data set,
     * and verifies whether the acquired data matches the known data pattern.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @returns SUCCESS if successful, \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered. \n
     */
    NP_EXPORT NP_ErrorCode bistPSB(int slotID, int portID, int dockID);

    /**
     * \brief The probe is configured for noise analysis.
     *
     * Via the shank and base configuration registers and the memory map,
     * the electrode inputs are shorted to ground. The data signal is recorded and the noise level is calculated.
     * The function analyses if the probe performance falls inside a specified tolerance range (go/no-go test).
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @returns SUCCESS if successful, \n
     *          BIST_ERROR of test failed. \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered. \n
     */
    NP_EXPORT NP_ErrorCode bistNoise(int slotID, int portID, int dockID);
    /**
     * \brief The probe is configured for recording of a test signal which is generated on the HS.
     * This configuration is done via the shank and base configuration registers.
     * The AP data signal is recorded, and the frequency and amplitude of the recorded signal are extracted for each electrode.
     *
     * The probe passes the signal test if at least 90% of the electrodes show a signal within the frequency and amplitude
     * tolerance. The API function returns a pass/fail value.
     *
     * Do not load the ADC calibration parameters to the probe prior to calling the bistSignal function.
     * The tolerances on the amplitude test are set for an uncalibrated probe.
     * The test requires more than 30 seconds to complete.
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Id of the dock of the probe on the headstage. (1..2 (for NPM))
     * @returns Success if successful, BIST_ERROR if less than 90% of probes give good signal.
     */
    NP_EXPORT NP_ErrorCode bistSignal(int slotID, int portID, int dockID);

    /******************** Headstage tester API functions ****************************/
    /**
     * \brief Gets the headstage Tester's (HST) version.
     */
    NP_EXPORT NP_ErrorCode HST_GetVersion(int slotID, int portID, int* version_major, int* version_minor);
    /**
     * \brief Checks if the HS dock contains an analog 1.2 V supply signal within the required tolerance
     * 
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria
     */
    NP_EXPORT NP_ErrorCode HSTestVDDA1V2(int slotID, int portID);
    /**
     * \brief Checks if the HS dock contains an digital 1.2 V supply signal within the required tolerance
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria
     */
    NP_EXPORT NP_ErrorCode HSTestVDDD1V2(int slotID, int portID);
    /**
     * \brief Checks if the HS dock contains an analog 1.8 V supply signal within the required tolerance
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria
     */
    NP_EXPORT NP_ErrorCode HSTestVDDA1V8(int slotID, int portID);
    /**
     * \brief Checks if the HS dock contains an digital 1.8 V supply signal within the required tolerance
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria
     */
    NP_EXPORT NP_ErrorCode HSTestVDDD1V8(int slotID, int portID);
    /**
     * \brief Tests the functionality of the test signal generator.
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria
     */
    NP_EXPORT NP_ErrorCode HSTestOscillator(int slotID, int portID);
    /**
     * \brief Checks if the master clock (MCLK) signal on the headstage is present.
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria
     */
    NP_EXPORT NP_ErrorCode HSTestMCLK(int slotID, int portID);
    /**
     * \brief Tests the functionality of the probe clock pin (PCLK) on the headstage dock.
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria
     */
    NP_EXPORT NP_ErrorCode HSTestPCLK(int slotID, int portID);
    /**
     * \brief Tests the connectivity of the neural data bus.
     * 
     * if the test fails, It will print a message indicating which signal has failed.
     * 
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria. 
     */
    NP_EXPORT NP_ErrorCode HSTestPSB(int slotID, int portID);
    /**
     * \brief Tests the functionality of the I2C bus.
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria.
     */
    NP_EXPORT NP_ErrorCode HSTestI2C(int slotID, int portID);
    /**
     * \brief Tests the functionality of the reset signal (NRST) on the headstage.
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria.
     */
    NP_EXPORT NP_ErrorCode HSTestNRST(int slotID, int portID);
    /**
     * \brief Tests the functionality of the reset signal (REC_NRESET) on the headstage.
     *
     * @returns SUCCESS if the test passes, BIST_ERROR if the test doesn't pass the criteria.
     */
    NP_EXPORT NP_ErrorCode HSTestREC_NRESET(int slotID, int portID);

// Configuration

#pragma pack(push, 1)
    struct HardwareID {
        uint64_t SerialNumber;
        char ProductNumber[64]; /**< Must include null-termination */
        char version_Major[16]; /**< Must include null-termination */
        char version_Minor[16]; /**< Must include null-termination */
    };
#pragma pack(pop)

    /**
     * \brief Gets the basestation's driver ID.
     */
    NP_EXPORT NP_ErrorCode getBasestationDriverID(int slotID, char* name, size_t len);
    /**
     * \brief Gets the headstage's driver ID.
     */
    NP_EXPORT NP_ErrorCode getHeadstageDriverID(int slotID, int portID, char* name, size_t len);
    /**
     * \brief Gets the Flex's driver ID.
     */
    NP_EXPORT NP_ErrorCode getFlexDriverID(int slotID, int portID, int dockID, char* name, size_t len);
    /**
     * \brief Gets the Probe's driver ID.
     */
    NP_EXPORT NP_ErrorCode getProbeDriverID(int slotID, int portID, int dockID, char* name, size_t len);

     /**
     * \brief Gets the basestation Connect Board's hardware ID.
     */
    NP_EXPORT NP_ErrorCode getBSCHardwareID(int slotID, struct HardwareID* pHwid);
    /**
     * \brief Gets the headstage's hardware ID.
     */
    NP_EXPORT NP_ErrorCode getHeadstageHardwareID(int slotID, int portID, struct HardwareID* pHwid);
    /**
     * \brief Gets the Flex's hardware ID.
     */
    NP_EXPORT NP_ErrorCode getFlexHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
    /**
     * \brief Reading the probes' S/N and P/N using the structure HardwareID. 
     * 
     * The parameters version_Major and version_Minor are not applicable and remain 0.
     */
    NP_EXPORT NP_ErrorCode getProbeHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);

    /**
     * \brief Configure the headstage serializer error generator for testing error detection and correction.
     * This is a hardware feature of the MAX9271 serializer. Once configured the generator can be enabled
     * using \ref enableSerDesErrorGenerator
     * \param slotID Basestation slot ID
     * \param portID Port-ID to use
     * \param error_rate At which rate errors should be generated.
     * \param error_count How many errors should be generated.
     * \returns SUCCESS if sucessfull \n
     *          NO_LINK if no datalink, \n
     *          NO_SLOT if no Neuropix card is plugged in the selected PXI chassis slot, \n
     *          WRONG_SLOT in case a slot number outside the valid range is entered, \n
     *          WRONG_PORT in case a port number outside the valid range is entered. \n
     */
    NP_EXPORT NP_ErrorCode configureSerDesErrorGenerator(int slotID,
                                                         int portID,
                                                         serdes_error_rate_t error_rate,
                                                         serdes_error_count_t error_count);

    /**
     * \brief Enables or disables SerDes error generator
     * \param slotID Basestation slot ID
     * \param portID Port-ID to use
     * \param enable enable (true) or disable(false) SerDes error generator
     */
    NP_EXPORT NP_ErrorCode enableSerDesErrorGenerator(int slotID, int portID, bool enable);

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
        uint32_t last_error_packet;  /**< last packet number with an error */
        uint32_t err_badmagic;       /**< amount of packet header bad MAGIC markers */
        uint32_t err_badcrc;         /**< amount of packet header CRC errors */
        uint32_t err_count;          /**< Every psb frame has an incrementing count index. If the received frame count value is not as expected possible data loss has occured and this flag is raised. */
        uint32_t err_serdes;         /**< incremented if a deserializer error (hardware pin) occured during receiption of this frame this flag is raised */
        uint32_t err_lock;           /**< incremented if a deserializer loss of lock (hardware pin) occured during receiption of this frame this flag is raised */
        uint32_t err_pop;            /**< incremented whenever the next blocknummer round-robin FiFo is flagged empty during request of the next value (for debug purpose only, irrelevant for end-user software) */
        uint32_t err_sync;           /**< Front-end receivers are out of sync. => frame is invalid. */
    };

    struct np_sourcestats {
        uint32_t timestamp;          /**< last recorded timestamp */
        uint32_t packetcount;        /**< last recorded packet counter */
        uint32_t samplecount;        /**< last recorded sample counter */
        uint32_t fifooverflow;       /**< last recorded sample counter */
    };


    /****** Neuropixels OPTO specific **********************************************************************************************************************/
    typedef enum
    {
        wavelength_blue, // 450nm
        wavelength_red   // 638nm
    }wavelength_t;

    /**
     * \brief Program the optical switch calibration using a calibration csv file.
     * A OPTO headstage must be attached to SlotID/PortID.
     * 
     * Example csv file content: \n 
     * 21050005          \n
     * 0, 0,  0.0, 4.0   \n
     * 0, 1,  0.0, 4.0   \n
     * 0, 2,  0.0, 4.0   \n
     * 0, 3,  0.0, 4.0   \n
     * 0, 4,  0.0, 4.0   \n
     * 0, 5,  0.0, 4.0   \n
     * 0, 6,  0.0, 4.0   \n
     * 0, 7,  0.0, 4.0   \n
     * 0, 8,  0.0, 4.0   \n
     * 0, 9,  0.0, 4.0   \n
     * 0, 10, 0.0, 4.0   \n
     * 0, 11, 0.0, 4.0   \n
     * 0, 12, 0.0, 4.0   \n
     * 0, 13, 0.0, 4.0   \n
     * 0, 14, 0.0, 4.0   \n
     * 1, 0,  0.0, 4.0   \n
     * 1, 1,  0.0, 4.0   \n
     * 1, 2,  0.0, 4.0   \n
     * 1, 3,  0.0, 4.0   \n
     * 1, 4,  0.0, 4.0   \n
     * 1, 5,  0.0, 4.0   \n
     * 1, 6,  0.0, 4.0   \n
     * 1, 7,  0.0, 4.0   \n
     * 1, 8,  0.0, 4.0   \n
     * 1, 9,  0.0, 4.0   \n
     * 1, 10, 0.0, 4.0   \n
     * 1, 11, 0.0, 4.0   \n
     * 1, 12, 0.0, 4.0   \n
     * 1, 13, 0.0, 4.0   \n
     * 1, 14, 0.0, 4.0   \n
     *
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Ignored.
     * @param[in] filename: Comma separated text file (csv) containing following data:
     *       first line : probe serial number
     *       next lines: 'wavelengthindex', 'thermal_switch_index', 'off_mA', 'on_mA'
     *        wavelengthindex    : 0 = blue(450nm), 1 = red(638nm)
     *        thermal_switch_index :
     *                0    : 1_1
     *                1..2 : 2_1, 2_2
     *                3..8 : 3_1, 3_2, 3_3, 3_4
     *                9..14: 4_1, 4_2, 4_3, 4_4, 4_5, 4_6, 4_7, 4_8
     *        off_mA/on_mA : on/off current setting.
     */
    NP_EXPORT NP_ErrorCode setOpticalCalibration(int slotID, int portID, int dockID, const char* filename);
    /**
     * \brief Program calibration current for a single optical thermal switch.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Ignored.
     * @param[in] wavelength: Optical path selection.
     * @param[in] thermal_switch_index: Thermal switch calibration target.
     *                0    : 1_1
     *                1..2 : 2_1, 2_2
     *                3..8 : 3_1, 3_2, 3_3, 3_4
     *                9..14: 4_1, 4_2, 4_3, 4_4, 4_5, 4_6, 4_7, 4_8
     * @param[in] On_mA:  ON current for the target switch.
     * @param[in] Off_mA: OFF current for the target switch.
     */
    NP_EXPORT NP_ErrorCode setOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermal_switch_index, double On_mA, double Off_mA);
    /**
     * \brief Get the calibration currents for a single optical thermal switch.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Ignored.
     * @param[in] wavelength: Optical path selection.
     * @param[in] thermal_switch_index: Thermal switch calibration target.
     *                0    : 1_1
     *                1..2 : 2_1, 2_2
     *                3..8 : 3_1, 3_2, 3_3, 3_4
     *                9..14: 4_1, 4_2, 4_3, 4_4, 4_5, 4_6, 4_7, 4_8
     * @param[out] On_mA:  ON current for the target switch.
     * @param[out] Off_mA: OFF current for the target switch.
     */
    NP_EXPORT NP_ErrorCode getOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermal_switch_index, double* On_mA, double* Off_mA);
    /**
     * \brief Activate an optical emission site.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Ignored.
     * @param[in] wavelength: Optical path selection.
     * @param[in] site:   Emission site index (0..13) or -1 to disable the optical path.
     */
    NP_EXPORT NP_ErrorCode setEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int site);
    /**
     * \brief activate an optical emission site.
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Ignored.
     * @param[in] wavelength: Optical path selection.
     * @param[out] site: Get the active emission site index (0..13, or -1 if the path is disabled).
     */
    NP_EXPORT NP_ErrorCode getEmissionSite(int slotID, int portID, int dockID, wavelength_t wavelength, int* site);
    /**
     * \brief get the light power attenuation factor for an emission site
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Ignored.
     * @param[in] wavelength: Optical path selection.
     * @param[in] site:   Emission site index (0..13) or -1 to disable the optical path.
     * @param[out] attenuation: Get the laser power attenuation factor.
     */
    NP_EXPORT NP_ErrorCode getEmissionSiteAttenuation(int slotID, int portID, int dockID, wavelength_t wavelength, int site, double* attenuation);
    /**
     * \brief Disable an optical emission path.
     * 
     * Note: only the current to the optical thermal switches is disabled. Laser power is not affected
     * 
     * @param[in] slotID: Id of the slot in the PXIe chassis or the virtual slot for OneBox.
     * @param[in] portID: Id of the port on the basestation. (1..4)
     * @param[in] dockID: Ignored.
     * @param[in] wavelength: Optical path selection.
     */
    NP_EXPORT NP_ErrorCode disableEmissionPath(int slotID, int portID, int dockID, wavelength_t wavelength);

    /**** Neuropixels IMU ********************************************************************************************************************************/

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

    /***** Debug support functions ************************************************************************************************************************/

    /**
     * Set the logging level.
     *
     * @param level Highest level of reported messages
     */
    NP_EXPORT void         dbg_setlevel(int level);

    /**
     * Get the current logging level
     *
     * @return Current logging level
     */
    NP_EXPORT int          dbg_getlevel(void);

    /**
     * Configure a logging callback.
     *
     * Whenever a logging message is produced with a level at or below `minlevel`,
     * the provided callback function will be called with:
     * - the actual level of the message,
     * - a timestamp for the message,
     * - the module (function) in which the message was produced, and
     * - the message contents
     *
     * Only one callback can be used at a time, and a previously installed callback can be
     * removed by calling this function with nullptr.
     *
     * @param minlevel Highest level of reported messages
     * @param callback Callback (use nullptr to remove previously installed callback)
     */
    NP_EXPORT void         dbg_setlogcallback(int minlevel, void(*callback)(int level, time_t ts, const char* module, const char* msg));

    /**
     * Reset stream processor and source ID statistics of a slot.
     *
     * @param slotID Target slot
     */
    NP_EXPORT NP_ErrorCode dbg_stats_reset(int slotID);

    /**
     * Read stream processor statistics.
     *
     * @param slotID Target slot
     * @param stats Pointer to result
     */
    NP_EXPORT NP_ErrorCode dbg_diagstats_read(int slotID, struct np_diagstats* stats);

    /**
     * Read statics for given source ID.
     *
     * @param slotID Target slot
     * @param sourceID Source ID
     * @param stats Pointer to result
     */
    NP_EXPORT NP_ErrorCode dbg_sourcestats_read(int slotID, uint8_t sourceID, struct np_sourcestats* stats);

#define NP_APIC __stdcall
    extern "C" {
        //NeuropixAPI.h
        NP_EXPORT void         NP_APIC np_getAPIVersion(int* version_major, int* version_minor, int* version_patch);
        NP_EXPORT size_t       NP_APIC np_getAPIVersionFull(char* buffer, size_t size);
        NP_EXPORT void         NP_APIC np_getMinimumFtdiDriverVersion(int* version_major, int* version_minor, int* version_build);
        NP_EXPORT void         NP_APIC np_checkFtdiDriver(ftdi_driver_version_t* required,ftdi_driver_version_t* current,bool* is_driver_present,bool* is_version_ok);
        NP_EXPORT void         NP_APIC np_checkBasestationSupported(int slotID, firmware_Info* carrier, firmware_Info* bsc, bool* carrier_supported, bool* bsc_supported);
        NP_EXPORT size_t       NP_APIC np_getLastErrorMessage(char* buffer, size_t buffer_size);
        NP_EXPORT const char*  NP_APIC np_getErrorMessage(NP_ErrorCode code);
        NP_EXPORT int          NP_APIC np_getDeviceList(struct basestationID* list, int count);
        NP_EXPORT NP_ErrorCode NP_APIC np_getDeviceInfo(int slotID, struct basestationID* info);
        NP_EXPORT bool         NP_APIC np_tryGetSlotID(const basestationID* bsid, int* slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_scanBS(void);
        NP_EXPORT NP_ErrorCode NP_APIC np_mapBS(int serial_number, int slot);
        NP_EXPORT NP_ErrorCode NP_APIC np_unmapBS(int slot);
        NP_EXPORT NP_ErrorCode NP_APIC np_setParameter(np_parameter_t paramID, int value);
        NP_EXPORT NP_ErrorCode NP_APIC np_getParameter(np_parameter_t paramid, int* value);
        NP_EXPORT NP_ErrorCode NP_APIC np_setParameter_double(np_parameter_t paramid, double value);
        NP_EXPORT NP_ErrorCode NP_APIC np_getParameter_double(np_parameter_t paramid, double* value);
        NP_EXPORT char**       NP_APIC np_probeInfo_GetAllFieldNames(size_t* numFields);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfo_QueryPn(const char* queryStr, probeinfo_result_t* qResult);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfoResult_Count(probeinfo_result_t qResult, size_t* count);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfoResult_GetValueByField(probeinfo_result_t qResult, const char* fieldName, char** fieldValue);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfoResult_GetValueCpyByField(probeinfo_result_t qResult, const char* fieldName, char* fieldValue, const size_t maxStrLen);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfoResult_GetAllFieldsAndValues(probeinfo_result_t qResult, char*** fields, char*** values, size_t* num_fields);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfoResult_Free(probeinfo_result_t* qResult);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfoResult_NextRecord(probeinfo_result_t qResult, bool* success);
        NP_EXPORT NP_ErrorCode NP_APIC np_probeInfoResult_Reset(probeinfo_result_t qResult);
        NP_EXPORT NP_ErrorCode NP_APIC np_detectBS(int slotID, bool* detected);
        NP_EXPORT NP_ErrorCode NP_APIC np_openBS(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_closeBS(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_arm(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_setSWTrigger(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_setSWTriggerEx(int slotID, swtriggerflags_t trigger_flags);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_set(int slotID, switchmatrixoutput_t output, switchmatrixinput_t input, bool connect);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_get(int slotID, switchmatrixoutput_t output, switchmatrixinput_t input, bool* isconnected);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_clear(int slotID, switchmatrixoutput_t output);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_setInputInversion(int slotID, switchmatrixinput_t input, bool invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_getInputInversion(int slotID, switchmatrixinput_t input, bool* invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_setOutputInversion(int slotID, switchmatrixoutput_t output, bool invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_getOutputInversion(int slotID, switchmatrixoutput_t output, bool* invert);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_setOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t edge);
        NP_EXPORT NP_ErrorCode NP_APIC np_switchmatrix_getOutputTriggerEdge(int slotID, switchmatrixoutput_t output, triggeredge_t* edge);
        NP_EXPORT NP_ErrorCode NP_APIC np_setSyncClockFrequency(int slotID, double frequency);
        NP_EXPORT NP_ErrorCode NP_APIC np_getSyncClockFrequency(int slotID, double* frequency);
        NP_EXPORT NP_ErrorCode NP_APIC np_setSyncClockPeriod(int slotID, int period_ms);
        NP_EXPORT NP_ErrorCode NP_APIC np_getSyncClockPeriod(int slotID, int* period_ms);
        NP_EXPORT NP_ErrorCode NP_APIC np_setDataMode(int slotID, int portID, np_datamode_t mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_getDataMode(int slotID, int portID, np_datamode_t* mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_getTemperature(int slotID, double* temperature_degC);
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_getFirmwareInfo(int slotID, struct firmware_Info* info);
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_updateFirmware(int slotID, const char* filename, int(*callback)(size_t bytes_written));
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_resetFirmware(int slotID, int (*callback)(size_t bytes_written));
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_resetFirmware(int slotID, int (*callback)(size_t bytes_written));
        NP_EXPORT NP_ErrorCode NP_APIC np_bs_getFirmwareSize(int slotID, size_t* size);
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_getFirmwareSize(int slotID, size_t* size);
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_getTemperature(int slotID, double* temperature_degC);
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_getFirmwareInfo(int slotID, struct firmware_Info* info);
        NP_EXPORT NP_ErrorCode NP_APIC np_bsc_updateFirmware(int slotID, const char* filename, int(*callback)(size_t bytes_written));
        NP_EXPORT NP_ErrorCode NP_APIC np_ob_getFirmwareInfo(int slotID, struct firmware_Info* info);
        NP_EXPORT NP_ErrorCode NP_APIC np_hs_getFirmwareInfo(int slotID, int portID, firmware_Info* info);
        NP_EXPORT NP_ErrorCode NP_APIC np_hs_updateFirmware(int slotID, int portID, const char* filename, bool read_check);
        NP_EXPORT NP_ErrorCode NP_APIC np_getBSCSupportedPortCount(int slotID, int* count);
        NP_EXPORT NP_ErrorCode NP_APIC np_getHSSupportedProbeCount(int slotID, int portID, int* count);
        NP_EXPORT NP_ErrorCode NP_APIC np_openPort(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_closePort(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_detectHeadStage(int slotID, int portID, bool* detected);
        NP_EXPORT NP_ErrorCode NP_APIC np_detectFlex(int slotID, int portID, int dockID, bool* detected);
        NP_EXPORT NP_ErrorCode NP_APIC np_setHSLed(int slotID, int portID, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_openProbe(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_closeProbe(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_init(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_writeProbeConfiguration(int slotID, int portID, int dockID, bool read_check);
        NP_EXPORT NP_ErrorCode NP_APIC np_getCommittedProbeConfiguration(int slotID, int portID, int dockID, int channel_count, ProbeChannelConfiguration* configuration);
        NP_EXPORT NP_ErrorCode NP_APIC np_setADCCalibration(int slotID, int portID, const char* filename);
        NP_EXPORT NP_ErrorCode NP_APIC np_setGainCalibration(int slotID, int portID, int dockID, const char* filename);
        NP_EXPORT NP_ErrorCode NP_APIC np_readElectrodeData(int slotID, int portID, int dockID, struct electrodePacket* packets, int* actual_amount, int requested_amount);
        NP_EXPORT NP_ErrorCode NP_APIC np_getElectrodeDataFifoState(int slotID, int portID, int dockID, int* packets_available, int* headroom);
        NP_EXPORT NP_ErrorCode NP_APIC np_setTestSignal(int slotID, int portID, int dockID, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_setOPMODE(int slotID, int portID, int dockID, probe_opmode_t mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_setCALMODE(int slotID, int portID, int dockID, testinputmode_t mode);
        NP_EXPORT NP_ErrorCode NP_APIC np_setREC_NRESET(int slotID, int portID, bool state);
        NP_EXPORT NP_ErrorCode NP_APIC np_readPacket(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pck_info, int16_t* data, int requested_channel_count, int* actual_read);
        NP_EXPORT NP_ErrorCode NP_APIC np_readPackets(int slotID, int portID, int dockID, streamsource_t source, struct PacketInfo* pck_info, int16_t* data, int channel_count, int packet_count, int* packets_read);
        NP_EXPORT NP_ErrorCode NP_APIC np_getPacketFifoStatus(int slotID, int portID, int dockID, streamsource_t source, int* packets_available, int* headroom);
        NP_EXPORT NP_ErrorCode NP_APIC np_setGain(int slotID, int portID, int dockID, int channel, int ap_gain_select, int lfg_gain_select);
        NP_EXPORT NP_ErrorCode NP_APIC np_getGain(int slotID, int portID, int dockID, int channel, int* ap_gain_select, int* lfg_gain_select);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrode(int slotID, int portID, int dockID, int channel, int shank, int bank);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrodeMask(int slotID, int portID, int dockID, int channel, int shank, electrodebanks_t bank_mask);
        NP_EXPORT NP_ErrorCode NP_APIC np_setReference(int slotID, int portID, int dockID, int channel, int shank, channelreference_t reference, int int_ref_electrode_bank);
        NP_EXPORT NP_ErrorCode NP_APIC np_setAPCornerFrequency(int slotID, int portID, int dockID, int channel, bool disable_high_pass);
        NP_EXPORT NP_ErrorCode NP_APIC np_setStdb(int slotID, int portID, int dockID, int channel, bool standby);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectColumnPattern(int slotID, int portID, int dockID, columnpattern_t pattern);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrodeGroup(int slotID, int portID, int dockID, int channel_group, int bank);
        NP_EXPORT NP_ErrorCode NP_APIC np_selectElectrodeGroupMask(int slotID, int portID, int dockID, int channel_group, electrodebanks_t mask);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_writeBuffer(int slotID, const int16_t* data, int len);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_arm(int slotID, bool single_shot);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_stop(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_setSampleFrequency(int slotID, double frequency_Hz);
        NP_EXPORT NP_ErrorCode NP_APIC np_waveplayer_getSampleFrequency(int slotID, double* frequency_Hz);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_read(int slotID, int ADC_channel, double* voltage);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_readComparator(int slotID, int ADC_channel, bool* state);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_readComparators(int slotID, uint32_t* flags);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_enableProbe(int slotID, bool enable);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getStreamConversionFactor(int slotID, double* lsb_to_voltage, int* bit_depth);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_setComparatorThreshold(int slotID, int ADC_channel, double v_low, double v_high);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getComparatorThreshold(int slotID, int ADC_channel, double* v_low, double* v_high);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_setVoltageRange(int slotID, ADCrange_t range);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getVoltageRange(int slotID, ADCrange_t* range);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_setVoltage(int slotID, int DAC_channel, double voltage);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_setVoltages(int slotID, uint16_t DAC_channel_mask, double* voltages);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_enableOutput(int slotID, int DAC_channel, bool state);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_setDigitalLevels(int slotID, int DAC_channel, double v_high, double v_low);
        NP_EXPORT NP_ErrorCode NP_APIC np_DAC_setProbeSniffer(int slotID, int DAC_channel, int portID, int dockID, int channel, streamsource_t source_type);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_readPackets(int slotID, struct PacketInfo* pck_info, int16_t* data, int channel_count, int packet_count, int* packets_read);
        NP_EXPORT NP_ErrorCode NP_APIC np_ADC_getPacketFifoStatus(int slotID, int* packets_available, int* headroom);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistBS(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistHB(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistStartPRBS(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistStopPRBS(int slotID, int portID, int* prbs_err);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistReadPRBS(int slotID, int portID, int* prbs_err);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistI2CMM(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistEEPROM(int slotID, int portID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistSR(int slotID, int portID, int dockID, uint8_t* shanksOkMask=NULL);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistPSB(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistNoise(int slotID, int portID, int dockID);
        NP_EXPORT NP_ErrorCode NP_APIC np_bistSignal(int slotID, int portID, int dockID);

        NP_EXPORT NP_ErrorCode NP_APIC np_HST_GetVersion(int slotID, int portID, int* version_major, int* version_minor);
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

        //NeuropixAPI_configuration.h
        NP_EXPORT NP_ErrorCode NP_APIC np_getBasestationDriverID(int slotID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getHeadstageDriverID(int slotID, int portID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getFlexDriverID(int slotID, int portID, int dockID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getProbeDriverID(int slotID, int portID, int dockID, char* name, size_t len);
        NP_EXPORT NP_ErrorCode NP_APIC np_getBSCHardwareID(int slotID, struct HardwareID* pHwid);\
        NP_EXPORT NP_ErrorCode NP_APIC np_getHeadstageHardwareID(int slotID, int portID, struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_getFlexHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_getProbeHardwareID(int slotID, int portID, int dockID, struct HardwareID* pHwid);
        NP_EXPORT NP_ErrorCode NP_APIC np_configureSerDesErrorGenerator(int slotID, int portID, serdes_error_rate_t error_rate, serdes_error_count_t error_count);
        NP_EXPORT NP_ErrorCode NP_APIC np_enableSerDesErrorGenerator(int slotID, int portID, bool enable);

        //NeuropixAPI_debug.h
        NP_EXPORT void         NP_APIC np_dbg_setlevel(int level);
        NP_EXPORT int          NP_APIC np_dbg_getlevel(void);
        NP_EXPORT void         NP_APIC np_dbg_setlogcallback(int minlevel, void(*callback)(int level, time_t ts, const char* module, const char* msg));
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_stats_reset(int slotID);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_diagstats_read(int slotID, struct np_diagstats* stats);
        NP_EXPORT NP_ErrorCode NP_APIC np_dbg_sourcestats_read(int slotID, uint8_t sourceID, struct np_sourcestats* stats);

        // Opto specific
        NP_EXPORT NP_ErrorCode NP_APIC np_setOpticalCalibration(int slotID, int portID, int dockID, const char* filename);
        NP_EXPORT NP_ErrorCode NP_APIC np_setOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermal_switch_index, double On_mA, double Off_mA);
        NP_EXPORT NP_ErrorCode NP_APIC np_getOpticalSwitchCalibration(int slotID, int portID, int dockID, wavelength_t wavelength, int thermal_switch_index, double* On_mA, double* Off_mA);
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
