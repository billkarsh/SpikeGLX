#ifndef NP_ErrorCode_h_
#define NP_ErrorCode_h_

#include "dll_import_export.h"
#include <string>

enum NP_ErrorCode {
  SUCCESS = 0,          /**< */
  ALREADY_OPEN     = 1, /**< open called twice */
  NOT_OPEN         = 2, /**< TCP link is not open */
  PROBE_NOT_OPEN   = 3, /**< openProbe was not called */
  CONNECT_FAILED   = 4, /**< TCP Connection failed */
  VERSION_MISMATCH = 5, /**< hardware and software versions don't match */
  PARAMETER_INVALID = 6, /**< one of the function parameters is invalid */
  UART_RX_OVERFLOW_ERROR = 7,
  UART_FRAME_ERROR       = 8,
  UART_PARITY_ERROR      = 9,
  UART_ACK_ERROR         = 10,
  UART_TIMEOUT           = 11,
  UART_UNDERFLOW_ERROR   = 12,
  UART_WRITE_CHECK_ERROR = 13, /**< Write check (SR_OUT_OK) failed in headstage
                                 */
  MODE_INVALID = 14, /**< Mixing ADC mode commands/settings and Electrode mode
                       commands/settings */
  DATA_READ_FAILED = 15, /**< Reading from the TCP data link failed */
  SR_READ_POINTER_OUT_OF_RANGE  = 16, /**< shift register read pointer out of
                                        range */
  SR_WRITE_POINTER_OUT_OF_RANGE = 17, /**< shift register write pointer out of
                                        range */
  CHANNEL_OUT_OF_RANGE      = 18, /**< channel number out of range */
  ILLEGAL_WRITE_VALUE       = 19, /**< illegal write value */
  ADC_OUT_OF_RANGE          = 20, /**< ADC number out of range */
  ILLEGAL_CHAIN_VALUE       = 21, /**< illegal shift register chain value */
  CSV_READ_ERROR            = 22, /**< error while reading csv file */
  IIC_ERROR                 = 23, /**< error while reading BSC EEPROM */
  BIST_ERROR                = 24, /**< error in BIST */
  FILE_OPEN_ERROR           = 25, /**< Unable to open file */
  CSV_PROBE_ID_ERROR        = 26, /**< Wrong probe ID in csv file */
};

DLL_IMPORT_EXPORT std::string toString(NP_ErrorCode code);

#endif
