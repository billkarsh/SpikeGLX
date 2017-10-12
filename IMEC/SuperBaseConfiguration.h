#ifndef SuperBaseConfiguration_h_
#define SuperBaseConfiguration_h_

#include "BaseConfiguration.h"
#include "NP_ErrorCode.h"

#include <array>

class DLL_IMPORT_EXPORT SuperBaseConfiguration
{
public:
  SuperBaseConfiguration();
  ~SuperBaseConfiguration();

  friend bool operator==(const SuperBaseConfiguration & lhs,
                         const SuperBaseConfiguration & rhs);

  /**
   * @brief Set all base configurations to their default values.
   */
  void reset(bool omitCalibrated=true);

  /**
   *
   * @brief Translate the shiftregister chains to the base configurations.
   *
   * @param chains : the chains to translate
   *
   * @return SUCCESS if successful,
   *         SR_READ_POINTER_OUT_OF_RANGE if Shift Register read pointer error,
   *         ILLEGAL_CHAIN_VALUE if illegal reference combination
   */
  NP_ErrorCode getBaseConfigsFromChains(std::array<ShiftRegister, 2> & chains);

  /**
   * @brief Combine all members of all base configurations to the chains.
   *
   * @param chains : the resulting chains
   *
   * @return SUCCESS if successful,
   *         SR_WRITE_POINTER_OUT_OF_RANGE if Shift Register write pointer error
   */
  NP_ErrorCode getChains(std::array<ShiftRegister, 2> & chains) const;

  /**
   * @brief Get the channel reference of the given channel number.
   *
   * @param channelnumber : the channel number of which the channel reference is
   *                        wanted (valid range: 0 to 383)
   * @param channelreference : the channel reference to return
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range
   */
  NP_ErrorCode getChannelReference(unsigned int channelnumber,
                                   ChannelReference & channelreference) const;

  /**
   * @brief Set the channel reference of the given channel number.
   *
   * @param channelnumber : the channel number of which to set the channel
   *                        reference
   * @param channelreference : the value of the channel reference to set
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range,
   *         ILLEGAL_WRITE_VALUE if channel reference value invalid
   */
  NP_ErrorCode setChannelReference(unsigned int channelnumber,
                                   ChannelReference channelreference);

  /**
   * @brief Get the AP gain of the given channel number.
   *
   * @param channelnumber : the channel number of which the AP gain is wanted
   *                        (valid range: 0 to 383)
   * @param gAp           : the AP gain to return
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode getApGain(unsigned int channelnumber,
                         unsigned char & gAp) const;

  /**
   * @brief Set the AP gain of the given channel number.
   *
   * @param channelnumber : the channel number of which to set the AP gain
   * @param gAp : the value of the AP gain to set (valid range: 0 to 7)
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range,
   *         ILLEGAL_WRITE_VALUE if AP gain value invalid
   */
  NP_ErrorCode setApGain(unsigned int channelnumber, unsigned char gAp);

  /**
   * @brief Get the LFP gain of the given channel number.
   *
   * @param channelnumber : the channel number of which the LFP gain is wanted
   *                        (valid range: 0 to 383)
   * @param gLfp           : the LFP gain to return
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode getLfpGain(unsigned int channelnumber,
                          unsigned char & gLfp) const;

  /**
   * @brief Set the LFP gain of the given channel number.
   *
   * @param channelnumber : the channel number of which to set the LFP gain
   * @param gLfp : the value of the LFP gain to set (valid range: 0 to 7)
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range,
   *         ILLEGAL_WRITE_VALUE if LFP gain value invalid
   */
  NP_ErrorCode setLfpGain(unsigned int channelnumber, unsigned char gLfp);

  /**
   * @brief Get the STDB value of the given channel number.
   *
   * @param channelnumber : the channel number of which the STDB is wanted
   * @param stdb          : the STDB value to return
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode getStdb(unsigned int channelnumber, bool & stdb) const;

  /**
   * @brief Set the STDB value of the given channel number.
   *
   * @param channelnumber : the channel number of which to set the STDB
   * @param stdb          : the value of the STDB to set
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode setStdb(unsigned int channelnumber, bool stdb);

  /**
   * @brief Get the FULL value of the given channel number.
   *
   * @param channelnumber : the channel number of which the FULL is wanted
   * @param full          : the FULL value to return
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode getFull(unsigned int channelnumber, bool & full) const;

  /**
   * @brief Set the FULL value of the given channel number.
   *
   * @param channelnumber : the channel number of which to set the FULL
   * @param full          : the value of the FULL to set
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode setFull(unsigned int channelnumber, bool full);

  /**
   * @brief Get the Comparator offset P value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which the CompP is wanted
   * @param compP     : the compP value to return
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range.
   */
  NP_ErrorCode getCompP(unsigned int adcnumber, unsigned char & compP) const;

  /**
   * @brief Set the Comparator offset P value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which to set the CompP
   * @param compP     : the value of the CompP to set
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range,
   *         ILLEGAL_WRITE_VALUE if compP value out of range.
   */
  NP_ErrorCode setCompP(unsigned int adcnumber, unsigned char compP);

  /**
   * @brief Get the Comparator offset N value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which the CompN is wanted
   * @param compN     : the compN value to return
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range.
   */
  NP_ErrorCode getCompN(unsigned int adcnumber, unsigned char & compN) const;

  /**
   * @brief Set the Comparator offset N value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which to set the CompN
   * @param compN     : the value of the CompN to set
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range,
   *         ILLEGAL_WRITE_VALUE if compN value out of range.
   */
  NP_ErrorCode setCompN(unsigned int adcnumber, unsigned char compN);

  /**
   * @brief Get the Slope value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which the Slope is wanted
   * @param slope     : the slope value to return
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range.
   */
  NP_ErrorCode getSlope(unsigned int adcnumber, unsigned char & slope) const;

  /**
   * @brief Set the Slope value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which to set the Slope
   * @param slope     : the value of the slope to set
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range,
   *         ILLEGAL_WRITE_VALUE if slope value out of range.
   */
  NP_ErrorCode setSlope(unsigned int adcnumber, unsigned char slope);

  /**
   * @brief Get the Fine value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which the Fine is wanted
   * @param fine      : the fine value to return
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range.
   */
  NP_ErrorCode getFine(unsigned int adcnumber, unsigned char & fine) const;

  /**
   * @brief Set the Fine value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which to set the Fine
   * @param fine      : the value of the fine to set
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range,
   *         ILLEGAL_WRITE_VALUE if fine value out of range.
   */
  NP_ErrorCode setFine(unsigned int adcnumber, unsigned char fine);

  /**
   * @brief Get the Coarse value of the given ADC number.
   *
   * @param adcnumber   : the ADC number of which the Coarse is wanted
   * @param coarse      : the coarse value to return
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range.
   */
  NP_ErrorCode getCoarse(unsigned int adcnumber, unsigned char & coarse) const;

  /**
   * @brief Set the Coarse value of the given ADC number.
   *
   * @param adcnumber   : the ADC number of which to set the Coarse
   * @param coarse      : the value of the coarse to set
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range,
   *         ILLEGAL_WRITE_VALUE if coarse value out of range.
   */
  NP_ErrorCode setCoarse(unsigned int adcnumber, unsigned char coarse);

  /**
   * @brief Get the Cfix value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which the Cfix is wanted
   * @param cfix      : the cfix value to return
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range.
   */
  NP_ErrorCode getCfix(unsigned int adcnumber, unsigned char & cfix) const;

  /**
   * @brief Set the Cfix value of the given ADC number.
   *
   * @param adcnumber : the ADC number of which to set the Cfix
   * @param cfix      : the value of the cfix to set
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range,
   *         ILLEGAL_WRITE_VALUE if cfix value out of range.
   */
  NP_ErrorCode setCfix(unsigned int adcnumber, unsigned char cfix);

  /**
   * @brief Get the ADC settings values of the given ADC number.
   *
   * @param adcnumber   : the ADC number of which the ADC settings are wanted
   * @param adcsettings : the ADC settings to return
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range.
   */
  NP_ErrorCode getAdcSettings(unsigned int adcnumber,
                              AdcSettings & adcsettings) const;

  /**
   * @brief Set the ADC settings values of the given ADC number.
   *
   * @param adcnumber   : the ADC number of which to set the ADC settings
   * @param adcsettings : the value of the ADC settings to set
   *
   * @return SUCCESS if successful,
   *         ADC_OUT_OF_RANGE if ADC number out of range,
   *         ILLEGAL_WRITE_VALUE if cfix value out of range.
   */
  NP_ErrorCode setAdcSettings(unsigned int adcnumber,
                              AdcSettings & adcsettings);

  /**
   * @brief Get the channel settings values of the given channel number.
   *
   * @param channelnumber   : the channel number of which the channel settings
   *                          are wanted
   * @param channelsettings : the channel settings to return
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode getChannelSettings(unsigned int channelnumber,
                                  ChannelSettings & channelsettings) const;

  /**
   * @brief Set the channel settings values of the given channel number.
   *
   * @param channelnumber   : the channel number of which to set the channel
   *                          settings
   * @param channelsettings : the value of the channel settings to set
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range,
   *         ILLEGAL_WRITE_VALUE if channel settings value out of range.
   */
  NP_ErrorCode setChannelSettings(unsigned int channelnumber,
                                  ChannelSettings & channelsettings);

  /**
   * @brief Check whether one of the even / odd channels uses the external
   * reference.
   *
   * @param evenNotOdd: true for even channels, false for odd channels
   *
   * @return true if the external reference is used for at least one channel,
   *         false otherwise
   */
  bool externalReferenceUsed(bool evenNotOdd) const;

  /**
   * @brief Check whether one of the even / odd channels uses the tip reference.
   *
   * @param evenNotOdd: true for even channels, false for odd channels
   *
   * @return true if the tip reference is used for at least one channel,
   *         false otherwise
   */
  bool tipReferenceUsed(bool evenNotOdd) const;

  /**
   * @brief Check whether one of the even / odd channels uses the internal
   * reference.
   *
   * @param evenNotOdd: true for even channels, false for odd channels
   *
   * @return true if the internal reference is used for at least one channel,
   *         false otherwise
   */
  bool internalReferenceUsed(bool evenNotOdd) const;

private:
  std::array<BaseConfiguration, 2> baseConfigs_;
};

#endif
