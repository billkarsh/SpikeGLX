
#include "Sync.h"
#include "DAQ.h"


/* ---------------------------------------------------------------- */
/* SyncStream ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SyncStream::init( const AIQ *Q, int ip, const DAQ::Params &p )
{
    this->Q     = Q;
    this->ip    = ip;

    if( !Q || ip < -1 )
        return;

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
            bit  = p.sync.niChan % 16;
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
    quint64 stepBack = 1.5 * TRel2Ct( p.sync.sourcePeriod );

    fromCt -= (fromCt >= stepBack ? stepBack : fromCt);

    if( bit < 0 )
        return Q->findRisingEdge( outCt, fromCt, chan, thresh, 100 );
    else
        return Q->findBitRisingEdge( outCt, fromCt, chan, bit, 100 );
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

    if( p.sync.sourceIdx == DAQ::eSyncSourceNone
        || !src->findEdge( srcEdge, srcCt, p )
        || !dst->findEdge( dstEdge, dst->TAbs2Ct( srcTAbs ), p ) ) {

        dst->tAbs   = srcTAbs;
        dst->bySync = false;

        return srcTAbs;
    }

    double dstTAbs, halfPer;

    if( srcEdge > srcCt ) {
        quint64 perCt = src->TRel2Ct( p.sync.sourcePeriod );
        do {
            srcEdge -= perCt;
        } while( srcEdge > srcCt );
    }

    dstTAbs = dst->Ct2TAbs( dstEdge ) + src->Ct2TRel( srcCt - srcEdge );
    halfPer = 0.5 * p.sync.sourcePeriod;

    if( dstTAbs - srcTAbs > halfPer ) {
        do {
            dstTAbs -= p.sync.sourcePeriod;
        } while( dstTAbs - srcTAbs > halfPer );
    }
    else if( srcTAbs - dstTAbs > halfPer ) {
        do {
            dstTAbs += p.sync.sourcePeriod;
        } while( srcTAbs - dstTAbs > halfPer );
    }

    dst->tAbs   = dstTAbs;
    dst->bySync = true;

    return dstTAbs;
}


