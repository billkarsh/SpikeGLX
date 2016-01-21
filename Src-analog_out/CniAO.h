#ifndef CNIAO_H
#define CNIAO_H

#include "SGLTypes.h"

namespace DAQ {
struct Params;
}

class AOCtl;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Base class for analog output
//
class CniAO
{
protected:
    AOCtl               *owner;
    const DAQ::Params   &p;

public:
    CniAO( AOCtl *owner, const DAQ::Params &p )
    : owner(owner), p(p)    {}
    virtual ~CniAO()        {}

    virtual void clearDerived() = 0;

    virtual bool doAutoStart() = 0;
    virtual bool readyForScans() = 0;
    virtual bool devStart() = 0;
    virtual void devStop() = 0;
    virtual void putScans( vec_i16 &aiData ) = 0;
};

#endif  // CNIAO_H


