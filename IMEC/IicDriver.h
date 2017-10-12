#ifndef IicDriver_h_
#define IicDriver_h_

class t_iic_master;

/**
 * Driver for Easics AXI IIC IP core
 */
class IicDriver
{
public:
  IicDriver(t_iic_master * iic_regs);
  ~IicDriver();

  void iicWrite(unsigned int deviceAddr, unsigned int regAddr,
                unsigned int numRegBytes, unsigned int value,
                unsigned int numValueBytes, bool no_stop=false);
  unsigned int iicRead(unsigned int deviceAddr, unsigned int regAddr,
                       unsigned int numRegBytes, unsigned int numValueBytes);

private:

  t_iic_master * iic_regs_;
};

#endif
