#ifndef ShiftRegister_h_
#define ShiftRegister_h_

#include "dll_import_export.h"
#include "NP_ErrorCode.h"

#include <vector>

class DLL_IMPORT_EXPORT ShiftRegister
{
public:
  explicit ShiftRegister();
  explicit ShiftRegister(unsigned int bits);
  ~ShiftRegister();

  /**
   * @brief This function returns the size of the vector of shift register bits.
   *
   * @return the size of the shiftRegisterBits vector
   */
  unsigned int getSize();

  /**
   * @brief This function handles writing of 1 bit.
   *
   * @param bit: the bit to write
   *
   * @return SUCCESS if successful,
   *         SR_WRITE_POINTER_OUT_OF_RANGE if write pointer is out of range for
   *         this write
   */
  NP_ErrorCode writeBit(bool bit);

  /**
   * @brief This function handles writing of an array of n bits.
   *
   * @param bits: the bits to write
   *
   * @return SUCCESS if successful,
   *         SR_WRITE_POINTER_OUT_OF_RANGE if write pointer is out of range for
   *         this write
   */
  NP_ErrorCode writeBits(std::vector<bool> const & bits);

  /**
   * @brief This function handles writing of a number of bits.
   *
   * @param bits: the bits to write
   * @param numberOfBits: the number of bits to write
   *
   * @return SUCCESS if successful,
   *         SR_WRITE_POINTER_OUT_OF_RANGE if write pointer is out of range for
   *         this write
   */
  NP_ErrorCode writeBits(unsigned char bits, unsigned char numberOfBits);

  /**
   * @brief This function handles writing of zeroes.
   *
   * @param numberOfZeroes: the amount of zeroes to write
   *
   * @return SUCCESS if successful,
   *         SR_WRITE_POINTER_OUT_OF_RANGE if write pointer is out of range for
   *         this write
   */
  NP_ErrorCode writeZeroes(unsigned int numberOfZeroes);

  /**
   * @brief This function handles reading of 1 bit.
   *
   * @param bit: the bit read
   *
   * @return SUCCESS if successful,
   *         SR_READ_POINTER_OUT_OF_RANGE if read pointer is out of range for
   *         this read
   */
  NP_ErrorCode readBit(bool & bit);

  /**
   * @brief This function handles reading of n bits.
   *
   * @param bits: the bits read (in a std::array)
   *
   * @return SUCCESS if successful,
   *         SR_READ_POINTER_OUT_OF_RANGE if read pointer is out of range for
   *         this read
   */
  NP_ErrorCode readBits(std::vector<bool> & bits);

  /**
   * @brief This function handles reading of a number of bits.
   *
   * @param bits: the bits read
   * @param numberOfBits: The number of bits to read
   *
   * @return SUCCESS if successful,
   *         SR_READ_POINTER_OUT_OF_RANGE if read pointer is out of range for
   *         this read
   */
  NP_ErrorCode readBits(unsigned char & bits, unsigned int numberOfBits);

  /**
   * \brief Reset the read and write pointer so you can use readBit and writeBit
   * once more
   */
  void resetPointers();

  void print() const;

private:
  void resize(unsigned int numberOfBits);

  std::vector<bool> shiftRegisterBits_;
  unsigned int readBitPointer_;
  unsigned int writeBitPointer_;

};

#endif
