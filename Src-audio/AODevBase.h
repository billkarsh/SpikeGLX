#ifndef AODEVBASE_H
#define AODEVBASE_H

#include <QString>

namespace DAQ {
struct Params;
}

class AOCtl;
class AIQ;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Base class for audio output (AO)
//
class AODevBase
{
protected:
    AOCtl               *aoC;
    const DAQ::Params   &p;
    const AIQ           *aiQ;

public:
    AODevBase( AOCtl *aoC, const DAQ::Params &p )
    :   aoC(aoC), p(p)      {}
    virtual ~AODevBase()    {}

    // Development tests
    virtual void test1() = 0;
    virtual void test2() = 0;
    virtual void test3() = 0;

    // Device api
    virtual int  getOutChanCount( QString &err ) = 0;
    virtual bool doAutoStart() = 0;
    virtual bool readyForScans() const = 0;
    virtual bool devStart(
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &imQf,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ ) = 0;
    virtual void devStop() = 0;
};

#endif  // AODEVBASE_H


