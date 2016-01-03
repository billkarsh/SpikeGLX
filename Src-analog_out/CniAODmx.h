#ifndef CNIAODMX_H
#define CNIAODMX_H

#ifdef HAVE_NIDAQmx

#include "CniAO.h"
#include "Biquad.h"
#include "NI/NIDAQmx.h"

#include <QString>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Dmx analog output
//
class CniAODmx : public CniAO
{
private:
    struct Derived {
        QString         niChanStr;
        QVector<uint>   aiIndices;
        double          volume;
        int             kmux;

        void clear();
        bool usr2drv( AOCtl *aoC );

        qint16 vol( qint16 val );
    };

private:
    Derived     drv;
    Biquad      hipass,
                lopass;
    double      taskRunT;
    TaskHandle  taskHandle;
    int         errCnt;

public:
    CniAODmx( AOCtl *owner, const DAQ::Params &p );
    virtual ~CniAODmx();

    void clearDerived()     {drv.clear();}

    bool doAutoStart();
    bool readyForScans();
    bool devStart();
    void devStop();
    void putScans( vec_i16 &aiData );

private:
    int extractAOChans( vec_i16 &aoData, vec_i16 &aiData );
};

#endif  // HAVE_NIDAQmx

#endif  // CNIAODMX_H


