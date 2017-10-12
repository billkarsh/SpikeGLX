#ifndef VciInterface_h_
#define VciInterface_h_

#include <vector>
#include <stdexcept>

class VciInterface
{
public:
  // increments are expressed in word addresses.
  // So for INCR_1, the byte address increment is equal to vciDataWidth/8
  enum IncrementAddress
  {
    INCR_0 = 0,
    INCR_1 = 1
  };

  struct Exception : public std::runtime_error
    {
      Exception(const std::string & message);
    };

  VciInterface();
  virtual ~VciInterface();

  template<class T> void write(T & byteAddress, unsigned int value);
  template<class T> unsigned int read(T & byteAddress);

  virtual void vciWrite(unsigned int byteAddress, unsigned int value) = 0;
  virtual unsigned int vciRead(unsigned int byteAddress) = 0;
  virtual unsigned int getVciDataWidth() const = 0;

  virtual void vciBurstWrite(unsigned int byteAddress,
                             const std::vector<unsigned int> & values,
                             IncrementAddress = INCR_1);
  virtual std::vector<unsigned int> vciBurstRead(unsigned int byteAddress,
                                                 unsigned int numWords,
                                                 IncrementAddress = INCR_1);
};

template<class T>
void VciInterface::write(T & byteAddress, unsigned int value)
  {
    vciWrite((unsigned long int)(&byteAddress), value);
  }
template<class T>
unsigned int VciInterface::read(T & byteAddress)
  {
    return vciRead((unsigned long int)(void*)(&byteAddress));
  }

#endif

