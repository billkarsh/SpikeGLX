#ifndef AODEVSIM_H
#define AODEVSIM_H

#include "AODevBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Simulated audio output
//
class AODevSim : public AODevBase
{
public:
    AODevSim( AOCtl *aoC, const DAQ::Params &p )
    :   AODevBase(aoC, p)               {}

    // Development tests
    virtual void test1()                {}
    virtual void test2()                {}
    virtual void test3()                {}

    // Device api
    virtual int getOutChanCount( QString &err )
    {
        err = "This build does not support audio output.";
        return 0;
    }

    virtual bool doAutoStart()          {return false;}
    virtual bool readyForScans() const  {return false;}
    virtual bool devStart(
        const QVector<AIQ*> &,
        const QVector<AIQ*> &,
        const QVector<AIQ*> &,
        const AIQ           * )
                                        {return false;}
    virtual void devStop()              {}
};

#endif  // AODEVSIM_H


