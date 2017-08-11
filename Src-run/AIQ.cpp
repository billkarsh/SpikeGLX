
#include "AIQ.h"
#include "Util.h"


/* ---------------------------------------------------------------- */
/* AIQBlock ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

AIQ::AIQBlock::AIQBlock( vec_i16 &src, int len, quint64 headCt, double tailT )
    :   headCt(headCt), tailT(tailT)
{
    data.insert( data.begin(), src.begin(), src.begin() + len );
}


AIQ::AIQBlock::AIQBlock(
    vec_i16 &src,
    int     offset,
    int     len,
    quint64 headCt,
    double  tailT )
    :   headCt(headCt), tailT(tailT)
{
    data.insert(
        data.begin(),
        src.begin() + offset,
        src.begin() + offset + len );
}

/* ---------------------------------------------------------------- */
/* VBWalker ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Utility class assists edge detection across block boundaries.

class VBWalker {
private:
    std::deque<AIQ::AIQBlock>::const_iterator
                    it,
                    end;
    const qint16    *lim;
    int             nchans,
                    chan,
                    siz;
public:
    const qint16    *cur;
public:
    VBWalker( const std::deque<AIQ::AIQBlock> &Q, int nchans, int chan )
    :   it(Q.begin()), end(Q.end()), lim(0),
        nchans(nchans), chan(chan) {}

    bool SetStart( quint64 fromCt );
    bool next();
    quint64 curCt() {return it->headCt + (cur - &it->data[0])/nchans;}
    quint64 endCt() {return (lim ? (--end)->headCt + siz/nchans : 0);}
private:
    void nextBlock();
};


bool VBWalker::SetStart( quint64 fromCt )
{
    for( ; it != end; ++it ) {

        siz = (int)it->data.size();

        if( it->headCt + siz/nchans > fromCt ) {

            if( fromCt > it->headCt )
                cur = &it->data[chan + (fromCt - it->headCt)*nchans];
            else
                cur = &it->data[chan];

            lim = &it->data[0] + siz;

            return true;
        }
    }

    return false;
}


bool VBWalker::next()
{
    if( (cur += nchans) >= lim ) {

        if( ++it != end )
            nextBlock();
        else {
            --it;
            return false;
        }
    }

    return true;
}


void VBWalker::nextBlock()
{
    siz = (int)it->data.size();
    cur = &it->data[chan];
    lim = &it->data[0] + siz;
}


/* ---------------------------------------------------------------- */
/* VBFltWalker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Utility class assists edge detection across block boundaries;
// filters data via callback usrFlt.

class VBFltWalker {
private:
    std::deque<AIQ::AIQBlock>::const_iterator
                    it,
                    end;
    AIQ::T_AIQBlockFilter
                    &usrFlt;
    vec_i16         data;
    const qint16    *lim;
    int             nchans,
                    chan,
                    siz;
public:
    const qint16    *cur;
public:
    VBFltWalker(
        const std::deque<AIQ::AIQBlock> &Q,
        AIQ::T_AIQBlockFilter           &usrFlt,
        int                             nchans,
        int                             chan )
    :   it(Q.begin()), end(Q.end()), usrFlt(usrFlt),
        lim(0), nchans(nchans), chan(chan) {}

    bool SetStart( quint64 fromCt );
    bool next();
    quint64 curCt() {return it->headCt + (cur - &data[0])/nchans;}
    quint64 endCt() {return (lim ? (--end)->headCt + siz/nchans : 0);}
private:
    void nextBlock();
};


bool VBFltWalker::SetStart( quint64 fromCt )
{
    for( ; it != end; ++it ) {

        siz = (int)it->data.size();

        if( it->headCt + siz/nchans > fromCt ) {

            data = it->data;
            usrFlt( data );

            if( fromCt > it->headCt )
                cur = &data[chan + (fromCt - it->headCt)*nchans];
            else
                cur = &data[chan];

            lim = &data[0] + siz;

            return true;
        }
    }

    return false;
}


bool VBFltWalker::next()
{
    if( (cur += nchans) >= lim ) {

        if( ++it != end )
            nextBlock();
        else {
            --it;
            return false;
        }
    }

    return true;
}


void VBFltWalker::nextBlock()
{
    data    = it->data;
    siz     = (int)it->data.size();
    cur     = &data[chan];
    lim     = &data[0] + siz;

    usrFlt( data );
}

/* ---------------------------------------------------------------- */
/* AIQ ------------------------------------------------------------ */
/* ---------------------------------------------------------------- */

AIQ::AIQ( double srate, int nchans, int capacitySecs )
    :   srate(srate),
        maxCts(capacitySecs * srate),
        nchans(nchans),
        curCts(0)
{
}


void AIQ::enqueue( vec_i16 &src, double nowT, quint64 headCt, int nWhole )
{
    const int   maxScansPerBlk = 100;

    QMutexLocker    ml( &QMtx );

    updateQCts( nWhole );

    if( nWhole <= maxScansPerBlk )
        Q.push_back( AIQBlock( src, nWhole * nchans, headCt, nowT ) );
    else {

        double  tailT,
                delT;
        int     offset  = 0,
                nhalf;

        if( Q.size() ) {

            tailT   = Q.back().tailT;
            delT    = (nowT - tailT) / nWhole;
        }
        else {
            delT    = 1.0 / srate;
            tailT   = nowT - nWhole * delT;
        }

        // Remove maxScansPerBlk chunks until
        // only about that much remains...

        while( nWhole - maxScansPerBlk > maxScansPerBlk ) {

            Q.push_back(
                AIQBlock(
                    src,
                    offset * nchans,
                    maxScansPerBlk * nchans,
                    headCt,
                    tailT += maxScansPerBlk * delT ) );

            offset += maxScansPerBlk;
            headCt += maxScansPerBlk;
            nWhole -= maxScansPerBlk;
        }

        // Then divide remainder into two "halves"

        // First half

        nhalf = nWhole / 2;

        Q.push_back(
            AIQBlock(
                src,
                offset * nchans,
                nhalf * nchans,
                headCt,
                tailT += nhalf * delT ) );

        offset += nhalf;
        headCt += nhalf;
        nWhole -= nhalf;

        // Second half

        Q.push_back(
            AIQBlock(
                src,
                offset * nchans,
                nWhole * nchans,
                headCt,
                tailT + nWhole * delT ) );
    }
}


// Return headCt of current Q.front.
//
quint64 AIQ::qHeadCt() const
{
    QMutexLocker    ml( &QMtx );

    if( Q.size() )
        return Q.front().headCt;

    return 0;
}


// Return count of scans since run start.
//
quint64 AIQ::curCount() const
{
    QMutexLocker    ml( &QMtx );

    if( Q.size() ) {
        const AIQBlock  &B = Q.back();
        return B.headCt + B.data.size() / nchans;
    }

    return 0;
}


// Map given time to corresponding count.
// Return true if time within stream.
//
bool AIQ::mapTime2Ct( quint64 &ct, double t ) const
{
    bool found = false;

    ct = 0;

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->tailT < t ) {

                // back one too far

                if( ++it != end ) {

                    int tail = int((it->tailT - t) * srate) + 1,
                        size = (int)it->data.size() / nchans;

                    ct      = it->headCt + (tail < size ? size - tail : 0);
                    found   = true;
                }

                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    return found;
}


// Map given count to corresponding time.
// Return true if count within stream.
//
bool AIQ::mapCt2Time( double &t, quint64 ct ) const
{
    bool found = false;

    t = 0;

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->headCt <= ct ) {

                int thisCt = (int)it->data.size() / nchans;

                t       = it->tailT - (it->headCt + thisCt - 1 - ct) / srate;
                found   = true;
                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    return found;
}


// If vB.size() > 1 then the data are concatenated into cat,
// and a reference to cat is returned in output param dst.
//
// Otherwise, nothing is done to cat, and a reference to
// vB[0].data is returned in dst.
//
// Returns true if operation (alloc) successful.
//
bool AIQ::catBlocks(
    vec_i16*                &dst,
    vec_i16                 &cat,
    std::vector<AIQBlock>   &vB ) const
{
// default
    dst = &vB[0].data;

    int nb = (int)vB.size();

    if( nb > 1 ) {

        cat.clear();

        try {
            cat.reserve( sumCt( vB ) * nchans );
        }
        catch( const std::exception& ) {
            Warning() << "AIQ::catBlocks failed.";
            return false;
        }

        for( int i = 0; i < nb; ++i ) {

            const vec_i16   &D = vB[i].data;
            cat.insert( cat.end(), D.begin(), D.end() );
        }

        dst = &cat;
    }

    return true;
}


// Return sum of scans in vB.
//
quint64 AIQ::sumCt( std::vector<AIQBlock> &vB ) const
{
    quint64 sum = 0;
    int     nb  = (int)vB.size();

    for( int i = 0; i < nb; ++i )
        sum += vB[i].data.size();

    return sum / nchans;
}


// Return count following vB.
//
quint64 AIQ::nextCt( std::vector<AIQBlock> &vB ) const
{
    int             nb = (int)vB.size();
    const AIQBlock  &B = vB[nb-1];

    return B.headCt + B.data.size() / nchans;
}


// Return count following catBlocks data.
//
quint64 AIQ::nextCt( vec_i16 *data, std::vector<AIQBlock> &vB ) const
{
    return vB[0].headCt + data->size() / nchans;
}


// Copy first block with headCt >= fromCt.
// Return true if found.
//
bool AIQ::copy1BlockFromCt(
    vec_i16 &dest,
    quint64 &headCt,
    quint64 fromCt ) const
{
    QMutexLocker    ml( &QMtx );

    if( !Q.empty() ) {

        // Work backwards

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->headCt < fromCt ) {

                // back one too far

                if( ++it != end ) {

                    dest    = it->data;
                    headCt  = it->headCt;
                    return true;
                }

                return false;
            }

        } while( it != begin );
    }

    return false;
}


// Copy all scans later than fromT.
//
// If trim is true, first block is trimmed such that only scans
// since fromT are included.
//
// Return block count.
//
int AIQ::getAllScansFromT(
    std::vector<AIQBlock>   &dest,
    double                  fromT ) const
{
    int nb  = 0;

    dest.clear();

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->tailT < fromT ) {

                // back one too far

                for( ++it; it != end; ++it ) {

                    dest.push_back( *it );
                    ++nb;
                }

                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        int nTot = (int)D.size() / nchans,
            keep = (B.tailT - fromT) * srate;

        if( keep <= 0 )
            keep = 1;

        if( keep < nTot ) {

            B.headCt += nTot - keep;
            D.erase( D.begin(), D.begin() + nchans*(nTot - keep) );
        }
    }

    return nb;
}


// Copy all scans with count >= fromCt.
//
// Return block count.
//
int AIQ::getAllScansFromCt(
    std::vector<AIQBlock>   &dest,
    quint64                 fromCt ) const
{
    int nb = 0;

    dest.clear();

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->headCt <= fromCt ) {

                for( ; it != end; ++it ) {

                    dest.push_back( *it );
                    ++nb;
                }

                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        if( fromCt > B.headCt ) {

            D.erase( D.begin(), D.begin() + nchans*(fromCt - B.headCt) );
            B.headCt = fromCt;
        }
    }

    return nb;
}


// Copy up to N scans with t >= fromT.
//
// Return block count.
//
int AIQ::getNScansFromT(
    std::vector<AIQBlock>   &dest,
    double                  fromT,
    int                     nMax ) const
{
    int nb = 0,
        ct = 0;

    dest.clear();

    if( nMax <= 0 )
        return 0;

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->tailT < fromT ) {

                // back one too far

                for( ++it; it != end; ++it ) {

                    int thisCt = (int)it->data.size() / nchans;

                    dest.push_back( *it );
                    ct += thisCt;

                    if( !nb++ ) {

                        int keep = int((it->tailT - fromT) / srate) + 1;

                        if( keep < thisCt )
                            ct = keep;
                    }

                    if( ct >= nMax )
                        break;
                }

                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B      = dest[0];
        vec_i16     &D      = B.data;
        int         keep    = int((B.tailT - fromT) / srate) + 1,
                    ct0     = (int)D.size() / nchans;

        if( keep < ct0 ) {
            ct0 -= keep;    // ct0 -> diff
            D.erase( D.begin(), D.begin() + nchans*ct0 );
            B.headCt += ct0;
        }

        if( ct > nMax ) {

            AIQBlock    &B = dest[nb-1];
            vec_i16     &D = B.data;

            ct -= nMax; // ct -> diff

            D.erase( D.begin() + D.size() - nchans*ct, D.end() );
            B.tailT -= ct / srate;
        }
    }

    return nb;
}


// Copy up to N scans with count >= fromCt.
//
// Return block count.
//
int AIQ::getNScansFromCt(
    std::vector<AIQBlock>   &dest,
    quint64                 fromCt,
    int                     nMax ) const
{
    int nb = 0,
        ct = 0;

    dest.clear();

    if( nMax <= 0 )
        return 0;

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->headCt <= fromCt ) {

                for( ; it != end; ++it ) {

                    int thisCt = (int)it->data.size() / nchans;

                    dest.push_back( *it );
                    ct += thisCt;

                    if( !nb++ && fromCt > it->headCt )
                        ct -= fromCt - it->headCt;

                    if( ct >= nMax )
                        break;
                }

                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        if( fromCt > B.headCt ) {
            D.erase( D.begin(), D.begin() + nchans*(fromCt - B.headCt) );
            B.headCt = fromCt;
        }

        if( ct > nMax ) {

            AIQBlock    &B = dest[nb-1];
            vec_i16     &D = B.data;

            ct -= nMax; // ct -> diff

            D.erase( D.begin() + D.size() - nchans*ct, D.end() );
            B.tailT -= ct / srate;
        }
    }

    return nb;
}


// Specialized for mono audio.
// Copy nScans for given channel starting at fromCt.
//
// - Caller must presize dst as nScans*sizeof(qint16).
// - nScans taken as exact: if insufficient scans return -1.
//
// Return headCt or -1 if failure.
//
qint64 AIQ::getNScansFromCtMono(
    qint16                  *dst,
    quint64                 fromCt,
    int                     nScans,
    int                     chan ) const
{
    qint64  headCt = -1;

    QMtx.lock();

// Enough samples available?

    const AIQBlock  &B = Q.back();
    quint64         curCt;

    curCt = B.headCt + B.data.size() / nchans;

    if( curCt >= fromCt + nScans ) {
        headCt = fromCt;
        goto enough;
    }

    goto unlock;

enough:
    {
        std::deque<AIQBlock>::const_iterator
            end = Q.end(), it = end;

        // Work backwards to find start

        for(;;) {
            --it;
            if( it->headCt <= fromCt )
                break;
        }

        // Work forward

        int k0      = fromCt - it->headCt,
            idst    = 0;

        do {
            const qint16    *src    = &it->data[0];
            int             nT      = it->data.size() / nchans;

            for( int kT = k0; idst < nScans && kT < nT; ++kT ) {

                dst[idst++] = src[chan + kT*nchans];
            }

            k0 = 0;

        } while( idst < nScans && ++it != end );
    }

unlock:
    QMtx.unlock();

    return headCt;
}


// Specialized for stereo audio.
// Copy nScans for two given channels starting at fromCt.
//
// - Caller must presize dst as 2*nScans*sizeof(qint16).
// - nScans taken as exact: if insufficient scans return -1.
//
// Return headCt or -1 if failure.
//
qint64 AIQ::getNScansFromCtStereo(
    qint16                  *dst,
    quint64                 fromCt,
    int                     nScans,
    int                     chan1,
    int                     chan2 ) const
{
    qint64  headCt = -1;

    QMtx.lock();

// Enough samples available?

    const AIQBlock  &B = Q.back();
    quint64         curCt;

    curCt = B.headCt + B.data.size() / nchans;

    if( curCt >= fromCt + nScans ) {
        headCt = fromCt;
        goto enough;
    }

    goto unlock;

enough:
    {
        std::deque<AIQBlock>::const_iterator
            end = Q.end(), it = end;

        // Work backwards to find start

        for(;;) {
            --it;
            if( it->headCt <= fromCt )
                break;
        }

        // Work forward

        int k0      = fromCt - it->headCt,
            idst    = 0;

        nScans *= 2;    // using as index limit

        do {
            const qint16    *src    = &it->data[0];
            int             nT      = it->data.size() / nchans;

            for( int kT = k0; idst < nScans && kT < nT; ++kT ) {

                dst[idst++] = src[chan1 + kT*nchans];
                dst[idst++] = src[chan2 + kT*nchans];
            }

            k0 = 0;

        } while( idst < nScans && ++it != end );
    }

unlock:
    QMtx.unlock();

    return headCt;
}


// Copy most recent N scans.
//
// Return block count.
//
int AIQ::getNewestNScans(
    std::vector<AIQBlock>   &dest,
    int                     nMax ) const
{
    int nb = 0,
        ct = 0;

    dest.clear();

    if( nMax <= 0 )
        return 0;

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards, starting with newest block,
        // and prepending until reach scan count nMax.

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), it = Q.end();

        do {
            --it;

            dest.insert( dest.begin(), *it );
            ct += (int)it->data.size() / nchans;
            ++nb;

        } while( ct < nMax && it != begin );
    }

    QMtx.unlock();

    if( ct > nMax ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        ct -= nMax;

        D.erase( D.begin(), D.begin() + nchans*ct );
        B.headCt += ct;
    }

    return nb;
}


// Specialized for mono audio.
// Copy most recent nScans for given channel.
//
// - Caller must presize dst as nScans*sizeof(qint16).
// - nScans taken as exact: if insufficient scans return -1.
//
// Return headCt or -1 if failure.
//
qint64 AIQ::getNewestNScansMono(
    qint16                  *dst,
    int                     nScans,
    int                     chan ) const
{
    qint64  headCt = -1;

    QMtx.lock();

// Enough samples available?

    if( Q.size() ) {

        const AIQBlock  &B = Q.back();
        qint64          curCt;

        curCt = B.headCt + B.data.size() / nchans;

        if( curCt >= nScans ) {
            headCt = curCt - nScans;
            goto enough;
        }
    }

    goto unlock;

enough:
// Work backwards, starting with newest block,
// and prepending until reach nScans.

    {
        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), it = Q.end();

        do {
            --it;

            const qint16    *src    = &it->data[0];
            int             nT      = it->data.size() / nchans;

            for( ; nScans > 0 && nT > 0; ) {

                --nScans;   // using as index
                --nT;       // using as index

                dst[nScans] = src[chan + nT*nchans];
            }

        } while( nScans > 0 && it != begin );
    }

unlock:
    QMtx.unlock();

    return headCt;
}


// Specialized for stereo audio.
// Copy most recent nScans for two given channels.
//
// - Caller must presize dst as 2*nScans*sizeof(qint16).
// - nScans taken as exact: if insufficient scans return -1.
//
// Return headCt or -1 if failure.
//
qint64 AIQ::getNewestNScansStereo(
    qint16                  *dst,
    int                     nScans,
    int                     chan1,
    int                     chan2 ) const
{
    qint64  headCt = -1;

    QMtx.lock();

// Enough samples available?

    if( Q.size() ) {

        const AIQBlock  &B = Q.back();
        qint64          curCt;

        curCt = B.headCt + B.data.size() / nchans;

        if( curCt >= nScans ) {
            headCt = curCt - nScans;
            goto enough;
        }
    }

    goto unlock;

enough:
// Work backwards, starting with newest block,
// and prepending until reach nScans.

    {
        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), it = Q.end();

        do {
            --it;

            const qint16    *src    = &it->data[0];
            int             nT      = it->data.size() / nchans;

            for( ; nScans > 0 && nT > 0; ) {

                --nScans;   // using as index
                --nT;       // using as index

                dst[2*nScans]       = src[chan1 + nT*nchans];
                dst[2*nScans + 1]   = src[chan2 + nT*nchans];
            }

        } while( nScans > 0 && it != begin );
    }

unlock:
    QMtx.unlock();

    return headCt;
}


// Starting from fromCt, scan given chan for rising edge
// (from below to >= T). Including first crossing, require
// signal stays >= T for at least inarow counts.
//
// Return:
// false = no edge; resume looking from outCt.
// true  = edge @ outCt.
//
bool AIQ::findRisingEdge(
    quint64                 &outCt,
    quint64                 fromCt,
    int                     chan,
    qint16                  T,
    int                     inarow ) const
{
    int     nhi     = 0;
    bool    found   = false;

    QMtx.lock();

    VBWalker    W( Q, nchans, chan );

    if( !W.SetStart( fromCt ) )
        goto exit;

// -------------------
// Must start on a low
// -------------------

    if( *W.cur >= T ) {

        while( W.next() ) {

            if( *W.cur < T )
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( W.next() ) {

        if( *W.cur >= T ) {

            // Mark edge start
            outCt   = W.curCt();
            nhi     = 1;

            if( inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( W.next() ) {

                if( *W.cur >= T ) {

                    if( ++nhi >= inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nhi = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    if( !found ) {

        if( nhi )
            --outCt;    // review last candidate again
        else
            outCt = qMax( fromCt, W.endCt() );
    }

    QMtx.unlock();

    return found;
}


// Acting on filtered data via callback usrFlt...
//
// Starting from fromCt, scan given chan for rising edge
// (from below to >= T). Including first crossing, require
// signal stays >= T for at least inarow counts.
//
// Return:
// false = no edge; resume looking from outCt.
// true  = edge @ outCt.
//
bool AIQ::findFltRisingEdge(
    quint64                 &outCt,
    quint64                 fromCt,
    int                     chan,
    qint16                  T,
    int                     inarow,
    T_AIQBlockFilter        &usrFlt ) const
{
    int     nhi     = 0;
    bool    found   = false;

    QMtx.lock();

    VBFltWalker    W( Q, usrFlt, nchans, chan );

    if( !W.SetStart( fromCt ) )
        goto exit;

// -------------------
// Must start on a low
// -------------------

    if( *W.cur >= T ) {

        while( W.next() ) {

            if( *W.cur < T )
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( W.next() ) {

        if( *W.cur >= T ) {

            // Mark edge start
            outCt   = W.curCt();
            nhi     = 1;

            if( inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( W.next() ) {

                if( *W.cur >= T ) {

                    if( ++nhi >= inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nhi = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    if( !found ) {

        if( nhi )
            --outCt;    // review last candidate again
        else
            outCt = qMax( fromCt, W.endCt() );
    }

    QMtx.unlock();

    return found;
}


// Using specified digital bit within a word...
//
// Starting from fromCt, scan given chan for rising edge
// (0 -> 1). Including first crossing, require
// signal stays high for at least inarow counts.
//
// Return:
// false = no edge; resume looking from outCt.
// true  = edge @ outCt.
//
bool AIQ::findBitRisingEdge(
    quint64                 &outCt,
    quint64                 fromCt,
    int                     chan,
    int                     bit,
    int                     inarow ) const
{
    int     nhi     = 0;
    bool    found   = false;

    QMtx.lock();

    VBWalker    W( Q, nchans, chan );

    if( !W.SetStart( fromCt ) )
        goto exit;

// -------------------
// Must start on a low
// -------------------

    if( (*W.cur >> bit) & 1 ) {

        while( W.next() ) {

            if( !((*W.cur >> bit) & 1) )
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( W.next() ) {

        if( (*W.cur >> bit) & 1 ) {

            // Mark edge start
            outCt   = W.curCt();
            nhi     = 1;

            if( inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( W.next() ) {

                if( (*W.cur >> bit) & 1 ) {

                    if( ++nhi >= inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nhi = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    if( !found ) {

        if( nhi )
            --outCt;    // review last candidate again
        else
            outCt = qMax( fromCt, W.endCt() );
    }

    QMtx.unlock();

    return found;
}


// Starting from fromCt, scan given chan for falling edge
// (from above to < T). Including first crossing, require
// signal stays < T for at least inarow counts.
//
// Return:
// false = no edge; resume looking from outCt.
// true  = edge @ outCt.
//
bool AIQ::findFallingEdge(
    quint64                 &outCt,
    quint64                 fromCt,
    int                     chan,
    qint16                  T,
    int                     inarow ) const
{
    int     nlo     = 0;
    bool    found   = false;

    QMtx.lock();

    VBWalker    W( Q, nchans, chan );

    if( !W.SetStart( fromCt ) )
        goto exit;

// --------------------
// Must start on a high
// --------------------

    if( *W.cur < T ) {

        while( W.next() ) {

            if( *W.cur >= T )
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( W.next() ) {

        if( *W.cur < T ) {

            // Mark edge start
            outCt   = W.curCt();
            nlo     = 1;

            if( inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( W.next() ) {

                if( *W.cur < T ) {

                    if( ++nlo >= inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nlo = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    if( !found ) {

        if( nlo )
            --outCt;    // review last candidate again
        else
            outCt = qMax( fromCt, W.endCt() );
    }

    QMtx.unlock();

    return found;
}


// Acting on filtered data via callback usrFlt...
//
// Starting from fromCt, scan given chan for falling edge
// (from above to < T). Including first crossing, require
// signal stays < T for at least inarow counts.
//
// Return:
// false = no edge; resume looking from outCt.
// true  = edge @ outCt.
//
bool AIQ::findFltFallingEdge(
    quint64                 &outCt,
    quint64                 fromCt,
    int                     chan,
    qint16                  T,
    int                     inarow,
    T_AIQBlockFilter        &usrFlt ) const
{
    int     nlo     = 0;
    bool    found   = false;

    QMtx.lock();

    VBFltWalker    W( Q, usrFlt, nchans, chan );

    if( !W.SetStart( fromCt ) )
        goto exit;

// --------------------
// Must start on a high
// --------------------

    if( *W.cur < T ) {

        while( W.next() ) {

            if( *W.cur >= T )
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( W.next() ) {

        if( *W.cur < T ) {

            // Mark edge start
            outCt   = W.curCt();
            nlo     = 1;

            if( inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( W.next() ) {

                if( *W.cur < T ) {

                    if( ++nlo >= inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nlo = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    if( !found ) {

        if( nlo )
            --outCt;    // review last candidate again
        else
            outCt = qMax( fromCt, W.endCt() );
    }

    QMtx.unlock();

    return found;
}


// Using specified digital bit within a word...
//
// Starting from fromCt, scan given chan for falling edge
// (0 -> 1). Including first crossing, require
// signal stays low for at least inarow counts.
//
// Return:
// false = no edge; resume looking from outCt.
// true  = edge @ outCt.
//
bool AIQ::findBitFallingEdge(
    quint64                 &outCt,
    quint64                 fromCt,
    int                     chan,
    int                     bit,
    int                     inarow ) const
{
    int     nlo     = 0;
    bool    found   = false;

    QMtx.lock();

    VBWalker    W( Q, nchans, chan );

    if( !W.SetStart( fromCt ) )
        goto exit;

// --------------------
// Must start on a high
// --------------------

    if( !((*W.cur >> bit) & 1) ) {

        while( W.next() ) {

            if( (*W.cur >> bit) & 1 )
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( W.next() ) {

        if( !((*W.cur >> bit) & 1) ) {

            // Mark edge start
            outCt   = W.curCt();
            nlo     = 1;

            if( inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( W.next() ) {

                if( !((*W.cur >> bit) & 1) ) {

                    if( ++nlo >= inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nlo = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    if( !found ) {

        if( nlo )
            --outCt;    // review last candidate again
        else
            outCt = qMax( fromCt, W.endCt() );
    }

    QMtx.unlock();

    return found;
}

/* ---------------------------------------------------------------- */
/* AIQ private ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void AIQ::updateQCts( int nWhole )
{
    if( (curCts += nWhole) > maxCts ) {

        do {

            if( !Q.size() )
                return;

            curCts -= Q.front().data.size() / nchans;
            Q.pop_front();

        } while( curCts > maxCts );
    }
}


