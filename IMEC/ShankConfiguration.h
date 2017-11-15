#ifndef ShankConfiguration_h_
#define ShankConfiguration_h_

#include "NP_ErrorCode.h"
#include "ShiftRegister.h"

#include <array>

class DLL_IMPORT_EXPORT ShankConfiguration
{
public:
  ShankConfiguration();
  ~ShankConfiguration();

  /**
   * @brief Set all members to their default values.
   */
  void reset();

  /**
   * @brief Translate the shiftregister chain to shank configuration members.
   *
   * @param chain : the chain to translate
   *
   * @return SUCCESS if successful,
   *         SR_READ_POINTER_OUT_OF_RANGE if Shift Register read pointer error,
   *         ILLEGAL_CHAIN_VALUE if multiple electrodes for one channel enabled.
   */
  NP_ErrorCode getShankConfigFromChain(ShiftRegister & chain);

  /**
   * @brief Combine all members to the chain of bools for the shift register.
   *
   * @param chain : the resulting chain
   *
   * @return SUCCESS if successful,
   *         SR_WRITE_POINTER_OUT_OF_RANGE if Shift Register write pointer error
   */
  NP_ErrorCode getChain(ShiftRegister & chain) const;

  /**
   * @brief Get the external electrode value.
   *
   * @param evenNotOdd : true for even, false for odd tip electrode. External
   *                     electrode 1 is odd, external electrode 2 is even.
   * @param externalElectrode : return true if external electrode is enabled
   *
   * @return SUCCESS
   */
  NP_ErrorCode getExternalElectrode(bool evenNotOdd,
                                    bool & externalElectrode) const;

  /**
   * @brief Set the external electrode.
   *
   * @param evenNotOdd : true for even, false for odd tip electrode. External
   *                     electrode 1 is odd, external electrode 2 is even.
   * @param externalElectrode : enable or disable the externel electrode
   *
   * @return SUCCESS
   */
  NP_ErrorCode setExternalElectrode(bool evenNotOdd, bool externalElectrode);

  /**
   * @brief Get the tip electrode value.
   *
   * @param evenNotOdd : true for even, false for odd tip electrode. Tip
   *                     electrode 1 is odd, tip electrode 2 is even.
   * @param tipElectrode : return true if tip electrode is enabled
   *
   * @return SUCCESS
   */
  NP_ErrorCode getTipElectrode(bool evenNotOdd, bool & tipElectrode) const;

  /**
   * @brief Set the tip electrode.
   *
   * @param evenNotOdd : true for even, false for odd tip electrode. Tip
   *                     electrode 1 is odd, tip electrode 2 is even.
   * @param tipElectrode : enable or disable the tip electrode
   *
   * @return SUCCESS
   */
  NP_ErrorCode setTipElectrode(bool evenNotOdd, bool tipElectrode);

  /**
   * @brief Get the electrode connection.
   *
   * @param channelnumber : the channel number of which the electrode connection
   *                        is wanted (valid range: 0 to 383)
   * @param electrodeConnection : the electrode connection to return
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range.
   */
  NP_ErrorCode getElectrodeConnection(unsigned int channelnumber,
                                      unsigned char &electrodeConnection) const;

  /**
   * @brief Set the electrode connection.
   *
   * @param channelnumber : the channel number of which to set the electrode
   *                        connection (valid range: 0 to 383)
   * @param electrodeConnection : connect an electrode to a channel
   *                            - 0 : connect electrode nr channelnumber
   *                            - 1 : connect electrode nr channelnumber + 384
   *                            - 2 : connect electrode nr channelnumber + 768
   *                            - 0xff : no electrode is connected.
   *
   * @return SUCCESS if successful,
   *         CHANNEL_OUT_OF_RANGE if channel number out of range,
   *         ILLEGAL_WRITE_VALUE if electrode connection number invalid
   */
  NP_ErrorCode setElectrodeConnection(unsigned int channelnumber,
                                      unsigned char electrodeConnection);

private:
  std::array<unsigned char, 384> electrodeConnections_;
  std::array<bool, 2> externalElectrodes_;
  std::array<bool, 2> tipElectrodes_;
};

#endif
