
#include "Sync.h"
#include "DAQ.h"


/* ---------------------------------------------------------------- */
/* SyncStream ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SyncStream::init( const AIQ *Q, int js, int ip, const DAQ::Params &p )
{
    this->Q     = Q;
    this->js    = js;
    this->ip    = ip;

    if( !Q )
        return;

    switch( js ) {
        case jsNI:
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
            break;
        case jsOB:
            chan = p.im.get_iStrOneBox( ip ).obCumTypCnt[CimCfg::obSumData];
            bit  = 6;   // Sync signal always at bit 6 of AUX word
            break;
        case jsIM:
            chan = p.im.prbj[ip].imCumTypCnt[CimCfg::imSumNeural];
            bit  = 6;   // Sync signal always at bit 6 of AUX word
            break;
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

// Map an event (at given sample count) from src to dst stream.
// Set and return the absolute dst time of the event.
//
// If mapping was done via matching edges, set dst.bySync true.
// Else, dst absolute time is set to src absolute time.
//
double syncDstTAbs(
    quint64             srcCt,
    const SyncStream    *src,
    const SyncStream    *dst,
    const DAQ::Params   &p )
{
    double  srcTAbs = src->Ct2TAbs( srcCt );
    quint64 srcEdge,
            dstEdge;

    src->tAbs   = srcTAbs;
    src->bySync = false;
    dst->bySync = false;

    if( p.sync.sourceIdx == DAQ::eSyncSourceNone
        || !(src->bySync = src->findEdge( srcEdge, srcCt, p ))
        || !(dst->bySync = dst->findEdge( dstEdge, dst->TAbs2Ct( srcTAbs ), p )) ) {

        dst->tAbs = srcTAbs;

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


// Map an event (at given sample count) from src to each dst stream.
// Set each dst stream's absolute event time.
//
// Stream by stream...
// If mapping was done via matching edges, set dst.bySync true.
// Else, dst absolute time is set to src absolute time.
//
void syncDstTAbsMult(
    quint64                         srcCt,
    int                             iSrc,
    const std::vector<SyncStream>   &vS,
    const DAQ::Params               &p )
{
    const SyncStream    &src = vS[iSrc];

    src.tAbs = src.Ct2TAbs( srcCt );

    quint64 srcEdge;
    int     nS = vS.size();

    for( int is = 0; is < nS; ++is )
        vS[is].bySync = false;

    if( nS == 1 )
        return;

    if( p.sync.sourceIdx == DAQ::eSyncSourceNone
        || !(src.bySync = src.findEdge( srcEdge, srcCt, p )) ) {

        for( int is = 0; is < nS; ++is ) {

            if( is != iSrc )
                vS[is].tAbs = src.tAbs;
        }

        return;
    }

    for( int is = 0; is < nS; ++is ) {

        if( is == iSrc )
            continue;

        const SyncStream    &dst = vS[is];

        quint64 dstEdge;

        if( !dst.findEdge( dstEdge, dst.TAbs2Ct( src.tAbs ), p ) )
            dst.tAbs = src.tAbs;
        else {

            double dstTAbs, halfPer;

            if( srcEdge > srcCt ) {
                quint64 perCt = src.TRel2Ct( p.sync.sourcePeriod );
                do {
                    srcEdge -= perCt;
                } while( srcEdge > srcCt );
            }

            dstTAbs = dst.Ct2TAbs( dstEdge )
                        + src.Ct2TRel( srcCt - srcEdge );
            halfPer = 0.5 * p.sync.sourcePeriod;

            if( dstTAbs - src.tAbs > halfPer ) {
                do {
                    dstTAbs -= p.sync.sourcePeriod;
                } while( dstTAbs - src.tAbs > halfPer );
            }
            else if( src.tAbs - dstTAbs > halfPer ) {
                do {
                    dstTAbs += p.sync.sourcePeriod;
                } while( src.tAbs - dstTAbs > halfPer );
            }

            dst.tAbs    = dstTAbs;
            dst.bySync  = true;
        }
    }
}


