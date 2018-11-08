#ifndef AIQ_H
#define AIQ_H

/* ---------------------------------------------------------------- */
/* Includes ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#include "SGLTypes.h"

#include <QMutex>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class AIQ
{
/* ----- */
/* Types */
/* ----- */

public:
    // Callback filtering functor.
    // User owns fltbuf and presizes it to nmax.
    // AIQ populates fltbuf with one channel of
    // raw stream data. AIQ calls operator() to
    // have user apply filter to the nflt items
    // currently in fltbuf.
    struct T_AIQFilter {
        vec_i16 fltbuf;
        int     nmax,
                chan;
        virtual void operator()( int nflt ) = 0;
    };

/* ---- */
/* Data */
/* ---- */

private:
    const double    srate;
    const int       nchans,
                    bufmax;
    vec_i16         buf;
    mutable QMutex  QMtx;
    double          tzero;
    quint64         endCt;
    int             bufhead,
                    buflen;

/* ------- */
/* Methods */
/* ------- */

public:
    AIQ( double srate, int nchans, int capacitySecs );

    double sRate() const        {return srate;}
    double chanRate() const     {return nchans * srate;}
    int nChans() const          {return nchans;}

    void setTZero( double t0 )  {tzero = t0;}
    double tZero() const        {return tzero;}

    void enqueueZero( double t0, double tLim );

    void enqueue( const qint16 *src, int nCts );

    void enqueueProfile(
        double          &tLock,
        double          &tWork,
        const qint16    *src,
        int             nCts );

    quint64 qHeadCt() const;
    quint64 endCount() const;
    double endTime() const;
    int mapTime2Ct( quint64 &ct, double t ) const;
    int mapCt2Time( double &t, quint64 ct ) const;

    int getNScansFromCt(
        vec_i16         &dest,
        quint64         fromCt,
        int             nMax ) const;

    qint64 getNScansFromCtMono(
        qint16          *dst,
        quint64         fromCt,
        int             nScans,
        int             chan ) const;

    qint64 getNScansFromCtStereo(
        qint16          *dst,
        quint64         fromCt,
        int             nScans,
        int             chan1,
        int             chan2 ) const;

    qint64 getNewestNScansMono(
        qint16          *dst,
        int             nScans,
        int             chan ) const;

    qint64 getNewestNScansStereo(
        qint16          *dst,
        int             nScans,
        int             chan1,
        int             chan2 ) const;

    bool findRisingEdge(
        quint64         &outCt,
        quint64         fromCt,
        int             chan,
        qint16          T,
        int             inarow ) const;

    bool findFltRisingEdge(
        quint64         &outCt,
        quint64         fromCt,
        qint16          T,
        int             inarow,
        T_AIQFilter     &usrFlt ) const;

    bool findBitRisingEdge(
        quint64         &outCt,
        quint64         fromCt,
        int             chan,
        int             bit,
        int             inarow ) const;

    bool findFallingEdge(
        quint64         &outCt,
        quint64         fromCt,
        int             chan,
        qint16          T,
        int             inarow ) const;

    bool findFltFallingEdge(
        quint64         &outCt,
        quint64         fromCt,
        qint16          T,
        int             inarow,
        T_AIQFilter     &usrFlt ) const;

    bool findBitFallingEdge(
        quint64         &outCt,
        quint64         fromCt,
        int             chan,
        int             bit,
        int             inarow ) const;
};

#endif  // AIQ_H


