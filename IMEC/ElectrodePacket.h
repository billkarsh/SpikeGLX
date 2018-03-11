#ifndef ElectrodePacket_h_
#define ElectrodePacket_h_

#include <stdint.h>
#include <iosfwd>

/**
 * An electrode superframe as represented by the HW,
 * contains 1 LFP frame and 12 AP frames
 */
struct ElectrodeSuperFrame
{
  union
    {
      uint32_t header;
      struct
        {
          unsigned aux_bits : 16;
          unsigned superframe_counter : 4;
          unsigned dummy1 : 4;
          unsigned dummy2 : 2;
          unsigned errorflag : 1;
          unsigned electrode_mode : 1;
          unsigned lfp_not_ap : 1;
          unsigned trigger : 1;
          unsigned eop : 1;
          unsigned sop : 1;
        };
    };
  uint32_t timestamp;
  // Sorted per ADC number ! You need superframe_counter from the header to
  // know which samples these are.
  int16_t lfpSamples[32];
  // Sorted per channel number
  int16_t apSamples[12*32];
};

/**
 * An electrode packet as represented by the HW, contains 12 superframes
 */
struct ElectrodePacketHW
{
  ElectrodeSuperFrame frames[12];

  void writeCsv(std::ostream & output) const;
};

/**
 * Electrode packet with LFP ordered
 */
struct ElectrodePacket
{
  /// Fixed point samples, centered around 0.
  /// Conversion to millivolts : (X+512)/1024.0*1.2
  int16_t apData[12][384];
  uint32_t timestamp[12];
  int16_t lfpData[384];
  /// 1 startTrigger bit per superframe
  uint16_t startTrigger; // only lower 12 bits used
  /// 16 AUX IO pins in each superframe
  uint16_t aux[12];
};

#endif
