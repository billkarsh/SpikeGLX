
#include "AIQ.h"
#include "Util.h"


/* ---------------------------------------------------------------- */
/* AIQBlock ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

AIQ::AIQBlock::AIQBlock( const vec_i16 &src, int len, quint64 headCt )
    :   headCt(headCt)
{
    data.insert( data.begin(), src.begin(), src.begin() + len );
}


AIQ::AIQBlock::AIQBlock(
    const vec_i16   &src,
    int             offset,
    int             len,
    quint64         headCt )
    :   headCt(headCt)
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

    bool setStart( quint64 fromCt );
    bool next();
    quint64 curCt() {return it->headCt + (cur - &it->data[0])/nchans;}
    quint64 endCt() {return (lim ? (--end)->headCt + siz/nchans : 0);}
private:
    void nextBlock();
};


bool VBWalker::setStart( quint64 fromCt )
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

    bool setStart( quint64 fromCt );
    bool next();
    quint64 curCt() {return it->headCt + (cur - &data[0])/nchans;}
    quint64 endCt() {return (lim ? (--end)->headCt + siz/nchans : 0);}
private:
    void nextBlock();
};


bool VBFltWalker::setStart( quint64 fromCt )
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
    :   srate(srate), maxCts(capacitySecs * srate),
        nchans(nchans), tzero(0), curCts(0), endCt(0)
{
}


// Return ok.
//
// We will enqueue blocks of more or less constant size such that:
// (scans/block)*(chans/scan)*(2byte/chan) = 48KB/block.
//
bool AIQ::enqueue( const vec_i16 &src, quint64 headCt, int nWhole )
{
    const int   maxScansPerBlk = 24*1024 / nchans;

    QMutexLocker    ml( &QMtx );

    updateQCts( nWhole );

    try {
        if( nWhole <= maxScansPerBlk )
            Q.push_back( AIQBlock( src, nWhole * nchans, headCt ) );
        else {

            int offset  = 0,
                nhalf;

            // Remove maxScansPerBlk chunks until
            // only about that much remains...

            while( nWhole - maxScansPerBlk > maxScansPerBlk ) {

                Q.push_back(
                    AIQBlock(
                        src,
                        offset * nchans,
                        maxScansPerBlk * nchans,
                        headCt ) );

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
                    headCt ) );

            offset += nhalf;
            headCt += nhalf;
            nWhole -= nhalf;

            // Second half

            Q.push_back(
                AIQBlock(
                    src,
                    offset * nchans,
                    nWhole * nchans,
                    headCt ) );
        }
    }
    catch( const std::exception& ) {
        Error() << "AIQ::enqueue low mem. SRate " << srate;
        return false;
    }

    return true;
}


// Return headCt of current Q.front.
//
quint64 AIQ::qHeadCt() const
{
    QMutexLocker    ml( &QMtx );

    if( curCts )
        return Q.front().headCt;

    return 0;
}


// Return count of scans since run start.
//
quint64 AIQ::endCount() const
{
    QMutexLocker    ml( &QMtx );

    return endCt;
}


// Map given time to corresponding count.
// Return {-2=way left, -1=left, 0=inside, 1=right} of stream.
//
int AIQ::mapTime2Ct( quint64 &ct, double t ) const
{
    ct = 0;

    QMutexLocker    ml( &QMtx );

    if( t < tzero || !curCts )
        return -2;

    quint64 C = (t - tzero) * srate;

    if( C >= endCt )
        return 1;

    if( C < Q.front().headCt )
        return -1;

    ct = C;

    return 0;
}


// Map given count to corresponding time.
// Return {-2=way left, -1=left, 0=inside, 1=right} of stream.
//
int AIQ::mapCt2Time( double &t, quint64 ct ) const
{
    t = 0;

    QMutexLocker    ml( &QMtx );

    if( !curCts )
        return -2;

    if( ct >= endCt )
        return 1;

    if( ct < Q.front().headCt )
        return -1;

    t = tzero + ct / srate;

    return 0;
}


// If vB.size() > 1 then the data are concatenated into cat,
// and a reference to cat is returned in output param dst.
//
// Otherwise, nothing is done to cat, and a reference to
// vB[0].data is returned in dst.
//
// Return ok.
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
            Warning() << "AIQ::catBlocks low mem. SRate " << srate;
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


// Copy all scans later than fromT.
//
// Return ok.
//
bool AIQ::getAllScansFromT(
    std::vector<AIQBlock>   &dest,
    double                  fromT ) const
{
    return getAllScansFromCt( dest, (fromT - tzero) * srate );
}


// Copy all scans with count >= fromCt.
//
// Return ok.
//
bool AIQ::getAllScansFromCt(
    std::vector<AIQBlock>   &dest,
    quint64                 fromCt ) const
{
    int     nb = 0;
    bool    ok = true;

    dest.clear();

    QMtx.lock();

    if( fromCt < endCt ) {

        // Work backward

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->headCt <= fromCt ) {

                // Work forward

                for( ; it != end; ++it ) {

                    try {
                        dest.push_back( *it );
                        ++nb;
                    }
                    catch( const std::exception& ) {
                        Warning()
                            << "AIQ::allScans low mem. SRate " << srate;
                        ok = false;
                        break;
                    }
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

    return ok;
}


// Copy all scans with count >= fromCt.
//
// On entry dest should be cleared and reserved to nominal size.
//
// Return ok.
//
bool AIQ::getAllScansFromCt(
    vec_i16                 &dest,
    quint64                 fromCt ) const
{
    int     ct = 0;
    bool    ok = true;

    QMtx.lock();

    if( fromCt < endCt ) {

        // Work backward

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->headCt <= fromCt ) {

                // Work forward

                for( ; it != end; ++it ) {

                    const vec_i16   &D = it->data;

                    int len = D.size(),
                        off = 0;

                    if( !ct ) {
                        off  = nchans*(fromCt - it->headCt);
                        len -= off;
                    }

                    try {
                        dest.insert( dest.end(), D.begin() + off, D.end() );
                        ct += len;
                    }
                    catch( const std::exception& ) {
                        Warning()
                            << "AIQ::allScans low mem. SRate " << srate;
                        ok = false;
                        break;
                    }
                }

                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    return ok;
}


// Copy up to N scans with t >= fromT.
//
// Return ok.
//
bool AIQ::getNScansFromT(
    std::vector<AIQBlock>   &dest,
    double                  fromT,
    int                     nMax ) const
{
    return getNScansFromCt( dest, (fromT - tzero) * srate, nMax );
}


// Copy up to N scans with count >= fromCt.
//
// Return ok.
//
bool AIQ::getNScansFromCt(
    std::vector<AIQBlock>   &dest,
    quint64                 fromCt,
    int                     nMax ) const
{
    int     nb = 0,
            ct = 0;
    bool    ok = true;

    dest.clear();

    if( nMax <= 0 )
        return 0;

    QMtx.lock();

    if( fromCt < endCt ) {

        // Work backward

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        do {
            --it;

            if( it->headCt <= fromCt ) {

                // Work forward

                for( ; it != end; ++it ) {

                    int thisCt = (int)it->data.size() / nchans;

                    try {
                        dest.push_back( *it );
                        ct += thisCt;

                        if( !nb++ && fromCt > it->headCt )
                            ct -= fromCt - it->headCt;

                        if( ct >= nMax )
                            break;
                    }
                    catch( const std::exception& ) {
                        Warning()
                            << "AIQ::nScans low mem. SRate " << srate;
                        ok = false;
                        break;
                    }
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

            D.erase( D.end() - nchans*(ct - nMax), D.end() );
        }
    }

    return ok;
}


// Copy up to N scans with count >= fromCt.
//
// On entry dest should be cleared and reserved to nominal size.
//
// Return ok.
//
bool AIQ::getNScansFromCt(
    vec_i16                 &dest,
    quint64                 fromCt,
    int                     nMax ) const
{
    int     ct = 0;
    bool    ok = true;

    QMtx.lock();

    if( fromCt < endCt ) {

        // Work backward

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), end = Q.end(), it = end;

        nMax *= nchans;

        do {
            --it;

            if( it->headCt <= fromCt ) {

                // Work forward

                for( ; it != end; ++it ) {

                    const vec_i16   &D = it->data;

                    int len = D.size(),
                        off = 0;

                    if( !ct ) {
                        off  = nchans*(fromCt - it->headCt);
                        len -= off;
                    }

                    if( ct + len > nMax )
                        len = nMax - ct;

                    try {
                        dest.insert(
                            dest.end(),
                            D.begin() + off,
                            D.begin() + off + len );

                        ct += len;

                        if( ct >= nMax )
                            break;
                    }
                    catch( const std::exception& ) {
                        Warning()
                            << "AIQ::allScans low mem. SRate " << srate;
                        ok = false;
                        break;
                    }
                }

                break;
            }

        } while( it != begin );
    }

    QMtx.unlock();

    return ok;
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

    if( endCt >= fromCt + nScans ) {
        headCt = fromCt;
        goto enough;
    }

    goto unlock;

enough:
    {
        std::deque<AIQBlock>::const_iterator
            end = Q.end(), it = end;

        // Work backward to find start

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

    if( endCt >= fromCt + nScans ) {
        headCt = fromCt;
        goto enough;
    }

    goto unlock;

enough:
    {
        std::deque<AIQBlock>::const_iterator
            end = Q.end(), it = end;

        // Work backward to find start

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
// Return ok.
//
bool AIQ::getNewestNScans(
    std::vector<AIQBlock>   &dest,
    int                     nMax ) const
{
    int     ct = 0;
    bool    ok = true;

    dest.clear();

    if( nMax <= 0 )
        return 0;

    QMtx.lock();

    if( curCts ) {

        // Work backward, starting with newest block,
        // and prepending until reach scan count nMax.

        std::deque<AIQBlock>::const_iterator
            begin = Q.begin(), it = Q.end();

        do {
            --it;

            try {
                dest.insert( dest.begin(), *it );
                ct += (int)it->data.size() / nchans;
            }
            catch( const std::exception& ) {
                Warning()
                    << "AIQ::newestScans low mem. SRate " << srate;
                ct = dest.size() / nchans;
                ok = false;
                break;
            }

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

    return ok;
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

    if( endCt >= (quint64)nScans ) {
        headCt = endCt - nScans;
        goto enough;
    }

    goto unlock;

enough:
// Work backward, starting with newest block,
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

    if( endCt >= (quint64)nScans ) {
        headCt = endCt - nScans;
        goto enough;
    }

    goto unlock;

enough:
// Work backward, starting with newest block,
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

    if( !W.setStart( fromCt ) )
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

    if( !W.setStart( fromCt ) )
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

    if( !W.setStart( fromCt ) )
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

    if( !W.setStart( fromCt ) )
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

    if( !W.setStart( fromCt ) )
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

    if( !W.setStart( fromCt ) )
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
    curCts += nWhole;
    endCt  += nWhole;

    while( curCts > maxCts ) {

        if( !Q.size() )
            return;

        curCts -= Q.front().data.size() / nchans;
        Q.pop_front();
    }
}


