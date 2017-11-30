
#include "Sync.h"
#include "DAQ.h"
#include "AIQ.h"


/* ---------------------------------------------------------------- */
/* SyncStream ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SyncStream::init( const AIQ *Q, int ip, const DAQ::Params &p )
{
    this->Q     = Q;
    this->ip    = ip;

    if( !Q || ip < -1 )
        return;

    srate = Q->sRate();
    tZero = Q->getTZero();

    if( ip >= 0 ) {

        if( p.sync.imChanType == 0 ) {
            chan = p.im.imCumTypCnt[CimCfg::imSumNeural];
            bit  = p.sync.imChan;
        }
        else {
            chan    = p.sync.imChan;
            bit     = -1;
            thresh  = p.im.vToInt10( p.sync.imThresh, chan );
        }
    }
    else {

        if( p.sync.niChanType == 0 ) {
            chan = p.ni.niCumTypCnt[CniCfg::niSumAnalog]
                    + p.sync.niChan/16;
            bit  = p.sync.niChan;
        }
        else {
            chan    = p.sync.niChan;
            bit     = -1;
            thresh  = p.ni.vToInt16( p.sync.niThresh, chan );
        }
    }
}


bool SyncStream::findEdge(
    quint64             &outCt,
    quint64             fromCt,
    const DAQ::Params   &p ) const
{
    fromCt -= 1.5 * p.sync.sourcePeriod * srate;

    if( bit < 0 )
        return Q->findRisingEdge( outCt, fromCt, chan, thresh, 1 );
    else
        return Q->findBitRisingEdge( outCt, fromCt, chan, bit, 1 );
}

/* ---------------------------------------------------------------- */
/* Functions ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

double syncDstTAbs(
    quint64             srcCt,
    const SyncStream    *src,
    const SyncStream    *dst,
    const DAQ::Params   &p )
{
    double  srcTAbs = src->Ct2TAbs( srcCt );
    quint64 srcEdge,
            dstEdge;

    src->tAbs = srcTAbs;

    if( p.sync.sourceIdx == 0
        || !src->findEdge( srcEdge, srcCt, p )
        || !dst->findEdge( dstEdge, dst->TAbs2Ct( srcTAbs ), p ) ) {

        dst->tAbs   = srcTAbs;
        dst->bySync = false;

        return srcTAbs;
    }

    double dstTAbs = dst->Ct2TAbs( dstEdge )
                        + src->Ct2TRel( srcCt - srcEdge );

    if( dstTAbs >= srcTAbs + p.sync.sourcePeriod / 2 )
        dstTAbs -= p.sync.sourcePeriod;
    else if( dstTAbs < srcTAbs - p.sync.sourcePeriod / 2 )
        dstTAbs += p.sync.sourcePeriod;

    dst->tAbs   = dstTAbs;
    dst->bySync = true;

    return dstTAbs;
}


