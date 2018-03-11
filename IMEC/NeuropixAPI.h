#ifndef NeuropixAPI_h_
#define NeuropixAPI_h_

#include "dll_import_export.h"
#include "NP_ErrorCode.h"
#include "SuperBaseConfiguration.h"
#include "ShankConfiguration.h"
#include "CsvFile.h"

#include <string>
#include <vector>
#include <cstdint>

class neuropix_basestation;
class VciInterface;
class field_t_vci2uart_command;
class TcpConnectionLink;
class ADCPacket;
class ElectrodePacket;
class ElectrodePacketHW;
class NeuropixDataLinkIntf;
class IicDriver;
class t_bsc_fpga;

class DLL_IMPORT_EXPORT NeuropixAPI
{
public:
	static const unsigned int api_version_minor = 0;
	static const unsigned int api_version_major = 5;

	/**
	* Operating mode of the probe
	*/
	enum OperatingMode {
		RECORDING = 0, /**< Recording mode: (default) pixels connected to
					   channels */
		CALIBRATION = 1, /**< Calibration mode: test signal input connected to
						pixel, channel or ADC input */
		DIGITAL_TEST = 2, /**< Digital test mode: data transmitted over the PSB bus
						is a fixed data pattern */
	};

	/**
	* Test input mode
	*/
	enum TestInputMode {
		PIXEL_MODE = 0, /**< HS test signal is connected to the pixel inputs */
		CHANNEL_MODE = 1, /**< HS test signal is connected to channel inputs */
		NO_TEST_MODE = 2, /**< no test mode */
		ADC_MODE = 3, /**< HS test signal is connected to the ADC inputs */
	};

	NeuropixAPI();
	virtual ~NeuropixAPI();

	/**
	* \brief Initialize BS FPGA and BSC FPGA
	*
	* - Setup configuration link over TCP/IP
	* - BS FPGA reset
	* - Check hardware and software versions : HS, BS FPGA, API
	*
	* @param ipAddress : The IP address of the board to connect to
	*
	* @return SUCCESS, ALREADY_OPEN or CONNECT_FAILED
	*/
	NP_ErrorCode openBS(const char* ipAddress);

	/**
	* \brief Configure serializer and deserializer and power up probe.
	*/
	NP_ErrorCode openProbe(unsigned char slotID, signed char port);

	/**
	* \brief Reset the connected probe to the default settings.
	*
	* - set memory map & test configuration register to default values
	* - set shank configuration to default values
	* - set base configuration to default values
	* - set REC bit in OP_MODE register to 1, other bits 0
	* - set PSB_F bit in REC_MOD register to 1
	* - set GPIO1 signal from the serializer low
	*/
	NP_ErrorCode init(unsigned char slotID, signed char port);

	/**
	* \brief Close the connection to the probe and the basestation.
	*/
	NP_ErrorCode close(unsigned char slotID, signed char port);


	/**
	* \brief Write all shift registers.
	*
	* \param readCheck : if enabled, read the configuraiton shift registers back
	* to check
	*/
	NP_ErrorCode writeProbeConfiguration(unsigned char slotID, signed char port,
		bool readCheck);


	/**
	* \brief Re-arm the data capture trigger
	*
	* In arm mode, the system waits for a trigger to start storing data in the
	* DRAM.
	*/
	NP_ErrorCode arm(unsigned char slotID) const;

	/**
	* \brief Select the source of the trigger
	*
	* \param source :
	* 0. Software trigger
	* 1. BSC SMA start trigger
	* 2. AUX_IO(0)
	* 3. AUX_IO(1)
	* 4. AUX_IO(2)
	* 5. AUX_IO(3)
	* 6. AUX_IO(4)
	* 7. AUX_IO(5)
	* 8. AUX_IO(6)
	* 9. AUX_IO(7)
	* 10. AUX_IO(8)
	* 11. AUX_IO(9)
	* 12. AUX_IO(10)
	* 13. AUX_IO(11)
	* 14. AUX_IO(12)
	* 15. AUX_IO(13)
	* 16. AUX_IO(14)
	* 17. AUX_IO(15)
	* 18. PXIe Trigger (not implemented)
	*/
	NP_ErrorCode setTriggerSource(unsigned char slotID, char source) const;

	/**
	* \brief Select the edge to trigger on for hardware based triggers
	*
	* This also sets the polarity of the BSC SMA start trigger if used as output
	*
	* \param rising :
	* - false : falling edge trigger.  Output trigger pulse is active low.
	* - true  : rising edge trigger. Output trigger pulse is active high.
	*/
	NP_ErrorCode setTriggerEdge(unsigned char slotID, bool rising) const;

	/**
	* \brief Generate a software trigger
	*/
	NP_ErrorCode setSWTrigger(unsigned char slotID) const;

	/**
	* \brief Replace samples with zeros on error detection
	*/
	NP_ErrorCode setDataZeroing(unsigned char slotID, signed char port,
		bool enable) const;

	/**
	* \brief Look at lock and |err ports of deserializer to determine if samples
	* are errored
	*/
	NP_ErrorCode setLockChecking(unsigned char slotID, signed char port,
		bool enable) const;

	/**
	* \brief Set the operating mode of the probe
	*
	* \param mode: the selected probe operation mode.
	*/
	NP_ErrorCode setOPMODE(unsigned char slotID, signed char port,
		OperatingMode mode) const;

	/**
	* \brief Set the test input mode of the probe
	*
	* \param mode: the selected test signal mode
	*/
	NP_ErrorCode setCALMODE(unsigned char slotID, signed char port,
		TestInputMode mode) const;

	/**
	* \brief Read Electrode packets
	*
	* \param result C array of \arg amount ElectrodePacket
	* \param amount packets to read, maximum is 250
	*
	* Samples are NOT requested from DRAM.  It is assumed infinite streaming is
	* active (\see startInfiniteStream) so samples are continously coming from
	* the FPGA.
	*/
	NP_ErrorCode readElectrodeData(unsigned char slotID, signed char port,
		ElectrodePacket * result,
		unsigned int & actualAmount,
		unsigned int amount = 1) const;

	/**
	* \brief Setup the BS FPGA to continously stream packets
	*
	* This function should NOT be used in combination with readADCData.
	* It should be used with readRawData or readElectrodeData
	*
	* Typical use case : readRawData in a separate thread and starting and
	* stopping of the streaming in the main thread.
	*/
	NP_ErrorCode startInfiniteStream(unsigned char slotID) const;

	/**
	* \brief Stop the continous streaming of packets
	*/
	NP_ErrorCode stopInfiniteStream(unsigned char slotID) const;

	/**
	* \brief Read the temperator of the BS FPGA
	*
	* The temperature is in degrees Celcius.
	*/
	NP_ErrorCode getTemperature(unsigned char slotID, float & temperature) const;

	/**
	* \brief Get the filling of a sample fifo
	*
	* \param fifo : Select which fifo to read the filling from.  Fifo numbers
	* are 4 bit numbers.  The lower 2 bits are the shank number, upper 2 bits are
	* the headstage number
	* \param filling : The filling in percentage (0-100)
	*/
	NP_ErrorCode fifoFilling(unsigned char slotID, signed char port,
		unsigned char fifo, unsigned char & filling) const;

	/**
	* \brief Set the ADC calibration values (compP and compN, Slope, Coarse,
	* Fine, Cfix).
	*
	* Check if the selected calibration file is for the connected probe (by
	* checking the S/N of the connected probe with the S/N in the calibration csv
	* file). Read the csv file containing the ADC calibration data,
	* update the base configuration register variable in the API, and write to probe.
	*
	* \param filename : the filename to read from (should be .csv)
	*
	* \return SUCCESS if successful,
	*         ADC_OUT_OF_RANGE if ADC number out of range,
	*         ILLEGAL_WRITE_VALUE if compP or compN value out of range,
	*         CSV_READ_ERROR if reading the csv file caused an error
	*/
	NP_ErrorCode setADCCalibration(unsigned char slotID, signed char port,
		const char* filename);

	/**
	* \brief Read the gain correction parameters from the given file.
	*
	* Check if the selected calibration file is for the connected probe. Read the
	* CSV file containing the gain correction parameters and validate whether the
	* resulting values are in the valid range.
	*
	* \param filename : the filename to read from (should be .csv)
	*
	* \return SUCCESS if successful,
	*         CHANNEL_OUT_OF_RANGE if channel number out of range,
	*         ILLEGAL_WRITE_VALUE if gain correction value out of range,
	*         CSV_READ_ERROR if reading the csv file caused an error
	*/
	NP_ErrorCode setGainCalibration(unsigned char slotID, signed char port,
		const char* filename);

	/**
	* \brief Check if there has been an overflow
	*/
	NP_ErrorCode hadOverflow(bool & overflow) const;

	/**
	* \brief clear the overflow flag
	*
	* Overflow can happen when the system is capturing samples in DRAM, but they
	* are not read out fast enough.  To clear the overflow, the overflow
	* condition must be removed. This can be done by re-arming the system, which
	* will drop all samples until a trigger is encountered.  Or by reading out
	* samples from the DRAM at a fast enough rate.  After the overflow condition
	* is cleared, the overflow flag can be cleared.
	*/
	NP_ErrorCode clearOverflow() const;

	/**
	* \brief Enable or disable the logging of commands and status to file.
	*
	* The information is logged to a text file 'api_log.txt'.
	*
	* \param enable: enable or disable the logging
	*/
	NP_ErrorCode setLog(unsigned char slotID, signed char port, bool enable);


	/**
	* \brief Set the GPIO1 signal of the serializer
	*
	* \param enable: enable (true) or disable (false) HS heartbeat LED
	*/
	NP_ErrorCode setHSLed(unsigned char slotID, signed char port,
		bool enable) const;

	/**
	* \brief Get the API software version
	*
	* \param version_major: the API version number
	* \param version_minor: the API revision number
	*/
	NP_ErrorCode getAPIVersion(unsigned char & version_major,
		unsigned char & version_minor) const;

	/**
	* \brief Get the BSC FPGA boot code version
	*
	* \param version_major: the BSC FPGA boot code version number
	* \param version_minor: the BSC FPGA boot code revision number
	*/
	NP_ErrorCode getBSCBootVersion(unsigned char slotID,
		unsigned char & version_major,
		unsigned char & version_minor) const;
	/**
	* \brief Get the BS FPGA boot code version
	*
	* \param version_major: the BS FPGA boot code version number
	* \param version_minor: the BS FPGA boot code revision number
	*/
	NP_ErrorCode getBSBootVersion(unsigned char slotID,
		unsigned char & version_major,
		unsigned char & version_minor) const;

	/**
	* \brief Get the BSC version
	*/
	NP_ErrorCode getBSCVersion(unsigned char slotID,
		unsigned char & version_major,
		unsigned char & version_minor) const;

	/**
	* \brief Get the BSC serial number
	*/
	NP_ErrorCode readBSCSN(unsigned char slotID, uint64_t & sn) const;

	/**
	* \brief Read the BSC part number
	*/
	NP_ErrorCode readBSCPN(unsigned char slotID, char * pn) const;

	/**
	* \brief Get the HS version
	*/
	NP_ErrorCode getHSVersion(unsigned char slotID, signed char port,
		unsigned char & version_major,
		unsigned char & version_minor) const;

	/**
	* \brief Get probe ID
	*/
	NP_ErrorCode readId(unsigned char slotID, signed char port, uint64_t & id) const;


	/**
	* \brief Read the probe part number
	*/
	NP_ErrorCode readProbePN(unsigned char slotID, signed char port, char * pn) const;


	/**
	* \brief Get the Flex version
	*/
	NP_ErrorCode getFlexVersion(unsigned char slotID, signed char port,
		unsigned char & version_major,
		unsigned char & version_minor) const;

	/**
	* \brief Get the headstage serial number
	*/
	NP_ErrorCode readHSSN(unsigned char slotID, signed char port, uint64_t & sn) const;


	/**
	* \brief Read the HS part number
	*/
	NP_ErrorCode readHSPN(unsigned char slotID, signed char port, char * pn) const;


	/**
	* \brief Set which electrode is connected to a channel.
	*
	* \param channel: the channel number (valid range: 0 to 383, excluding 191)
	* \param electrode_bank: the electrode bank number to connect to
	*                        (valid range: 0 to 2 or 0xFF)
	*/
	NP_ErrorCode selectElectrode(unsigned char slotID, signed char port,
		unsigned int channel,
		unsigned int electrode_bank);

	/**
	* \brief Set the channel reference of the given channel.
	*
	* \param channel: the channel number (valid range: 0 to 383)
	* \param reference: the sleected reference input (valid range: 0 to 2)
	* \param intRefElectrodeBank: the selected internal reference electrode
	*                             (valid range: 0 to 2)
	*/
	NP_ErrorCode setReference(unsigned char slotID, signed char port,
		unsigned int channel, ChannelReference reference,
		unsigned char intRefElectrodeBank);

	/**
	* \brief Set the AP and LFP gain of the given channel.
	*
	* \param channel: the channel number (valid range: 0 to 383)
	* \param ap_gain: the AP gain value (valid range: 0 to 7)
	* \param lfp_gain: the LFP gain value (valid range: 0 to 7)
	*/
	NP_ErrorCode setGain(unsigned char slotID, signed char port,
		unsigned int channel, unsigned char ap_gain,
		unsigned char lfp_gain);

	/**
	* \brief Set the high-pass corner frequency for the given AP channel.
	*
	* \param channel: the channel number (valid range: 0 to 383)
	* \param hipassCorner: the highpass cut-off frequency of the AP channels
	*/
	NP_ErrorCode setAPCornerFrequency(unsigned char slotID, signed char port,
		unsigned int channel,
		bool hipassCorner);

	/**
	* \brief Set the given channel in stand-by mode.
	*
	* \param channel: the channel number (valid range: 0 to 383)
	* \param standby: the standby value to write
	*/
	NP_ErrorCode setStdb(unsigned char slotID, signed char port,
		unsigned int channel, bool standby);


protected:

	VciInterface * vciIntf_;
	neuropix_basestation * regs_;
	t_bsc_fpga * bsc_regs_;
	NeuropixDataLinkIntf * dataLink_;

private:

	/**
	* Select shift register.  The integer values are chosen to be the I2C
	* address of the shift register.
	*/
	enum ShiftRegisterSelect {
		BASECONFIG_ODD = 0xd,
		BASECONFIG_EVEN = 0xc,
		SHANKCONFIG = 0xe,
	};

	
	/**
	* \brief Set the BSC part number
	*/
	NP_ErrorCode writeBSCPN(unsigned char slotID, const char * pn);

	
	/**
	* \brief Set the probe part number
	*/
	NP_ErrorCode writeProbePN(unsigned char slotID, signed char port, const char * pn);

	/**
	* \brief Set the Flex version
	*/
	NP_ErrorCode setFlexVersion(unsigned char slotID, signed char port,
		unsigned char version_major,
		unsigned char version_minor) const;


	/**
	* \brief Set the HS part number
	*/
	NP_ErrorCode writeHSPN(unsigned char slotID, signed char port, const char * pn);


	SuperBaseConfiguration superBaseConfiguration;
	ShankConfiguration shankConfiguration;
	/**
	* \brief Basestation platform interconnection test
	*/
	NP_ErrorCode bistBS(unsigned char slotID) const;

	/**
	* \brief Heartbeat test
	*/
	NP_ErrorCode bistHB(unsigned char slotID, signed char port) const;

	/**
	* \brief Start Serdes PRBS test
	*/
	NP_ErrorCode bistStartPRBS(unsigned char slotID, signed char port) const;

	/**
	* \brief Stop Serdes PRBS test
	*/
	NP_ErrorCode bistStopPRBS(unsigned char slotID, signed char port,
		unsigned char & prbs_err) const;

	/**
	* \brief Test I2C memory map access
	*/
	NP_ErrorCode bistI2CMM(unsigned char slotID, signed char port) const;

	/**
	* \brief Test all EEPROMs (Flex, headstage, BSC).
	*/
	NP_ErrorCode bistEEPROM(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the shift registers
	*/
	NP_ErrorCode bistSR(unsigned char slotID, signed char port);

	/**
	* \brief Test the PSB bus on the headstage
	*/
	NP_ErrorCode bistPSB(unsigned char slotID, signed char port);

	/**
	* \brief Test the pixel signal path
	*/
	NP_ErrorCode bistSignal(unsigned char slotID, signed char port);

	/**
	* \brief The the noise level of the probe
	*/
	NP_ErrorCode bistNoise(unsigned char slotID, signed char port);

	/**
	* \brief Test the headstage VDDA 1V2
	*/
	NP_ErrorCode HSTestVDDA1V2(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage VDDD 1V2
	*/
	NP_ErrorCode HSTestVDDD1V2(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage VDDA 1V8
	*/
	NP_ErrorCode HSTestVDDA1V8(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage VDDD 1V8
	*/
	NP_ErrorCode HSTestVDDD1V8(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage oscillator
	*/
	NP_ErrorCode HSTestOscillator(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage MCLK amplitude and frequency
	*/
	NP_ErrorCode HSTestMCLK(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage PCLK lock
	*/
	NP_ErrorCode HSTestPCLK(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage PSB bus
	*
	* On error, BIST_ERROR is returned and errormask contains a 1 for the PSB but
	* bits with errors.
	*/
	NP_ErrorCode HSTestPSB(unsigned char slotID, signed char port,
		unsigned int & errormask) const;

	/**
	* \brief Test the headstage I2C interface
	*/
	NP_ErrorCode HSTestI2C(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage POR
	*/
	NP_ErrorCode HSTestNRST(unsigned char slotID, signed char port) const;

	/**
	* \brief Test the headstage REC_NRESET
	*/
	NP_ErrorCode HSTestREC_NRESET(unsigned char slotID, signed char port) const;

	// Calculate channel number based on position in frame
	// deser_index is row, word_counter is column
	// Channel number starts from 0, not 1 like in all the other API functions
	static int channelNumber(unsigned int deser_index,
		unsigned int word_counter,
		unsigned int frame_counter,
		unsigned int superframe_counter);
	// Version for LFP frames
	// Channel number starts from 0, not 1 like in all the other API functions
	static unsigned int channelNumberLFP(unsigned int adcNumber,
		unsigned int superframe);



	virtual NP_ErrorCode setupConfigLink(std::string const & ipAddress);

	virtual NP_ErrorCode setupDataLink(std::string const & ipAddress);

	NP_ErrorCode configureDeserializer();
	NP_ErrorCode configureSerializer();
	NP_ErrorCode serializerReadRegister(unsigned char address,
		unsigned char & value) const;
	NP_ErrorCode serializerWriteRegister(unsigned char address,
		unsigned char value) const;
	NP_ErrorCode deserializerReadRegister(unsigned char address,
		unsigned char & value) const;
	NP_ErrorCode deserializerWriteRegister(unsigned char address,
		unsigned char value) const;
	NP_ErrorCode writeUart(unsigned char device,
		unsigned char address,
		unsigned char data) const;
	NP_ErrorCode readUart(unsigned char device,
		unsigned char address,
		unsigned char & data) const;
	NP_ErrorCode uartBurst(std::vector<field_t_vci2uart_command> & commands,
		std::vector<unsigned char> & readdata) const;
	virtual NP_ErrorCode programI2CMux() const;

	bool BistCheckHelper_EQ(const char * lhsName, const char * rhsName,
		unsigned int lhs, unsigned int rhs,
		const char * file, unsigned int line) const;
	void msleep(unsigned int milliseconds) const;

	static int16_t standardDeviation(int16_t * samples, unsigned int size);
	static int16_t mean(int16_t * samples, unsigned int size);
	static int32_t mean(int32_t * samples, unsigned int size);

	TcpConnectionLink * tcpConfigLink_;
	bool probeOpened_;
	bool electrodeMode_;
	CsvFile gainCorrectionFile_;
	mutable std::ofstream logFile_;
	IicDriver * iicDriver_;



	// static lookup table for LFP positions (LFP is sorted per ADC, not channel)
	struct LfpPosition
	{
		unsigned char superframe;
		unsigned short adcNumber;
	};
	LfpPosition lfpPositions_[384];

	/**
	* \brief Write to a shift register.
	*
	* The write is done twice.  After the second time, the SR_OUT_OK register
	* inside the probe is checked.
	*
	* @param sr : Select which shift register to write
	* @param chain : The bits to write.  chain[0] is the LSB ( = first bit
	* written)
	* @param readCheck : if enabled, write twice and read back SR_OUT_OK
	*/
	NP_ErrorCode writeShiftRegister(ShiftRegisterSelect sr,
		ShiftRegister & chain,
		bool readCheck) const;
	/**
	* \brief Read out shift register.
	*
	* This function relies on the read-out capability of the ASIC where read data
	* is shifted back into the shift register.  As such, the read here is only a
	* read and not a write back to get the original value back in the shift
	* registers, as that is already done by the ASIC.
	*
	* @param sr : Select which shift register to read
	* @param chain : The bits read.  chain[0] is the LSB
	*/
	NP_ErrorCode readShiftRegister(ShiftRegisterSelect sr,
		ShiftRegister & chain) const;

	/**
	* \brief Read all shift registers.
	*/
	NP_ErrorCode readProbeConfiguration();

	/**
	* \brief Read Electrode packets without reordering the LFP samples
	*
	* \param result C array of \arg amount ElectrodePacketHW
	* \param amount packets to read, maximum is 250
	*/
	NP_ErrorCode readElectrodeDataUnordered(unsigned char slotID,
		signed char port,
		ElectrodePacketHW * result,
		unsigned int amount = 1,
		bool nonblocking = false,
		unsigned int * actualBlocks = 0) const;

	/**
	* \brief Read RAW data from the data link
	*
	* \param buffer preallocated buffer to store the data into
	* \param size   amount of bytes to store in the buffer
	*
	* No streaming is started in the BS FPGA.  It should be started by other
	* functions.  It can be started before or after calling this function.
	* It is up to the user to interpret the data correctly, either as ADCPacket
	* in ADC mode or as ElectrodePacket in Electrode mode.
	*/
	NP_ErrorCode readRawData(char * buffer, unsigned int size) const;

	/**
	* \brief Control the reset pin of the probe
	*
	* \param value : The logic value to put on the reset pin.  The reset is
	* active low, which means setting it to false, puts the probe in reset.
	*/
	NP_ErrorCode setREC_NRESET(unsigned char slotID, signed char port,
		bool value) const;

	/**
	* \brief Reset the datapath in the BS FPGA
	*/
	NP_ErrorCode datapathReset(unsigned char slotID) const;

	/**
	* \brief Set all gain values to 1 in the FPGA, both AP and LPF, all
	* electrodes
	*/
	NP_ErrorCode resetFpgaGain() const;

	/**
	* \brief Set the length of the moving average window.
	*
	* \param length : The size of the window in number of cycles.  Setting it to
	* 0 disables the moving average compensation in the scale module.  Maximum
	* value is 2^18-1
	*/
	NP_ErrorCode setMovingAverageWindowSize(unsigned int length) const;

	/**
	* \brief Get the vci interface
	*/
	VciInterface * getVciInterface() const;

	/**
	* \brief Read from the headstage.
	*
	* \param address: the address to read from
	* \param value: the value that was read
	*/
	NP_ErrorCode headstageReadRegister(unsigned char address,
		unsigned char & value) const;

	/**
	* \brief Write to the headstage.
	*
	* \param address: the address to write to
	* \param value: the value to write
	*/
	NP_ErrorCode headstageWriteRegister(unsigned char address,
		unsigned char value) const;

	/**
	* \brief Write the gain correction parameters to the basestation.
	*
	* Check the gain parameters of the superBaseConfiguration member. Use these
	* to select the right sets of gain correction parameters, that are written to
	* the basestation.
	*
	* \return SUCCESS if successful,
	*         NOT_OPEN if TCP link is not open
	*/
	NP_ErrorCode writeGainCorrection() const;

	/**
	* \brief Write 1 data byte to the BSC FPGA memory map.
	*
	* \param address: the register address to write to
	* \param data: the data byte to write
	*
	* \return SUCCESS if successful, NOT_OPEN if config link is not open
	*/
	NP_ErrorCode writeBSCMM(unsigned char slotID,
		unsigned int address, unsigned char data) const;

	/**
	* \brief Read 1 data byte from the BSC FPGA memory map.
	*
	* \param address: the register address to read from
	* \param data: the data byte read
	*
	* \return SUCCESS if successful, NOT_OPEN if config link is not open
	*/
	NP_ErrorCode readBSCMM(unsigned char slotID,
		unsigned int address, unsigned char & data) const;

	/**
	* \brief Write N data bytes to a specified address on a slave device.
	*
	* \param device: the device address (slave) to write to
	* \param address: the register address to write to
	* \param data: the data to write
	*
	* \return SUCCESS if successful, UART error when an error occurred over UART.
	*/
	NP_ErrorCode writeI2C(unsigned char slotID, signed char port,
		unsigned char device, unsigned char address,
		std::vector<unsigned char> & data) const;

	/**
	* \brief Read N data bytes from a specified address on a slave device.
	*
	* \param device: the device address (slave) to read from
	* \param address: the register address to read from
	* \param number: the number of bytes to read
	* \param data: vector containing the data read over the I2C bus
	*
	* \return SUCCESS if successful, UART error when an error occurred over UART.
	*/
	NP_ErrorCode readI2C(unsigned char slotID, signed char port,
		unsigned char device, unsigned char address,
		unsigned int number,
		std::vector<unsigned char> & data) const;

	/**
	* \brief Set the OSC_STDB bit in the CAL_MOD register of the probe memory map
	*
	* \param enable: enable (true) or disable (false) the test signal.
	*/
	NP_ErrorCode setTestSignal(unsigned char slotID, signed char port,
		bool enable) const;


	/**
	* \brief Set the BSC version
	*/
	NP_ErrorCode setBSCVersion(unsigned char slotID,
		unsigned char version_major,
		unsigned char version_minor) const;

	/**
	* \brief Set the BSC serial number
	*/
	NP_ErrorCode writeBSCSN(unsigned char slotID, uint64_t sn) const;

	/**
	* \brief Set the HS version
	*/
	NP_ErrorCode setHSVersion(unsigned char slotID, signed char port,
		unsigned char version_major,
		unsigned char version_minor) const;

	/**
	* \brief Set probe ID
	*/
	NP_ErrorCode writeId(unsigned char slotID, signed char port, uint64_t id) const;

	/**
	* \brief Set the headstage serial number
	*/
	NP_ErrorCode writeHSSN(unsigned char slotID, signed char port, uint64_t sn) const;


	/**
	* \brief Select the source of the data.
	*
	* \param source:
	* 0. Neuropix
	* 1. DataGenerator ADC mode
	* 2. DataGenerator Channel mode
	* 3. DataGenerator Incremental mode
	*/
	NP_ErrorCode selectDataSource(unsigned char slotID, signed char port,
		unsigned char source) const;
	/**
	* \brief Set the operating mode
	*
	* \param mode : false = ADC mode, true = electrode mode
	*/
	NP_ErrorCode setDatamode(unsigned char slotID, signed char port, bool mode);

	/**
	* \brief Get the current operating mode
	*
	* \returns false = ADC mode, true = electrode mode
	*/
	bool getDatamode() const;


	/**
	* \brief Read ADC packets
	*
	* \param result : an C-style array of 13 ADCPackets
	*/
	NP_ErrorCode readADCData(unsigned char slotID, signed char port,
		ADCPacket * result) const;

};

#endif
