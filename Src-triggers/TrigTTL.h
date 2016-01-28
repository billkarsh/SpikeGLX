#ifndef TRIGTTL_H
#define TRIGTTL_H

#include "TrigBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigTTL : public TrigBase
{
private:
    const qint64    nCycMax,
                    hiCtMax,
                    marginCt,
                    refracCt;
    const int       maxFetch;
    quint64         nextCt;
    int             nH,
                    state;

public:
    TrigTTL(
        DAQ::Params     &p,
        GraphsWindow    *gw,
        const AIQ       *imQ,
        const AIQ       *niQ );

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void initState();

    void seekNextEdge(
        quint64 &edgeCt,
        quint64 &fallCt,
        qint64  &remCt );

    bool writePreMargin(
        int     &ig,
        int     &it,
        qint64  &remCt,
        quint64 edgeCt );

    bool writePostMargin( qint64 &remCt, quint64 fallCt );

    bool doSomeH(
        int     &ig,
        int     &it,
        qint64  &remCt,
        quint64 &fallCt,
        quint64 edgeCt );
};

#endif  // TRIGTTL_H


