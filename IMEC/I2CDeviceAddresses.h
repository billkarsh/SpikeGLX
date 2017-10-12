#ifndef I2CDeviceAddresses_h_
#define I2CDeviceAddresses_h_

// Serializer I2C bus
static const unsigned char DESERIALIZER_DEVICE_ADDR = 0x90;
static const unsigned char SERIALIZER_DEVICE_ADDR = 0x80;
static const unsigned char PROBE_DEVICE_ADDR = 0xE0;
static const unsigned char HS_EEPROM_ADDR = 0xA4;
static const unsigned char FLEX_EEPROM_ADDR = 0xA0;

// FMC I2C bus
static const unsigned char BSC_EEPROM_ADDR = 0x50;
static const unsigned char I2C_MUX_ADDR = 0x74;

#endif
