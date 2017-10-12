#ifndef ADCPacket_h_
#define ADCPacket_h_

#include <stdint.h>

/**
 * An ADC packet, contains 1 ADC frames of 32 samples.
 */
struct ADCPacket
{
  union
    {
      uint32_t header;
      struct
        {
          unsigned superframe_counter : 4;
          unsigned dummy1 : 12;
          unsigned dummy2 : 8;
          unsigned dummy3 : 2;
          unsigned errorflag : 1;
          unsigned electrode_mode : 1;
          unsigned lfp_not_ap : 1;
          unsigned trigger : 1;
          unsigned eop : 1;
          unsigned sop : 1;
        };
    };
  uint32_t timestamp;
  int16_t samples[32];
};

#endif
