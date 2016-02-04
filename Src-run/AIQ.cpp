
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
    int             nChans,
                    chan,
                    siz;
public:
    const qint16    *cur;
public:
    VBWalker( const std::deque<AIQ::AIQBlock> &Q, int nChans, int chan )
    :   it(Q.begin()), end(Q.end()), lim(0),
        nChans(nChans), chan(chan) {}

    bool SetStart( quint64 fromCt );
    bool next();
    quint64 curCt() {return it->headCt + (cur - &it->data[0])/nChans;}
    quint64 endCt() {return (lim ? (--end)->headCt + siz/nChans : 0);}
private:
    void nextBlock();
};


bool VBWalker::SetStart( quint64 fromCt )
{
    for( ; it != end; ++it ) {

        siz = (int)it->data.size();

        if( it->headCt + siz/nChans > fromCt ) {

            if( fromCt > it->headCt )
                cur = &it->data[chan + (fromCt - it->headCt)*nChans];
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
    if( (cur += nChans) >= lim ) {

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
    int             nChans,
                    chan,
                    siz;
public:
    const qint16    *cur;
public:
    VBFltWalker(
        const std::deque<AIQ::AIQBlock> &Q,
        AIQ::T_AIQBlockFilter           &usrFlt,
        int                             nChans,
        int                             chan )
    :   it(Q.begin()), end(Q.end()), usrFlt(usrFlt),
        lim(0), nChans(nChans), chan(chan) {}

    bool SetStart( quint64 fromCt );
    bool next();
    quint64 curCt() {return it->headCt + (cur - &data[0])/nChans;}
    quint64 endCt() {return (lim ? (--end)->headCt + siz/nChans : 0);}
private:
    void nextBlock();
};


bool VBFltWalker::SetStart( quint64 fromCt )
{
    for( ; it != end; ++it ) {

        siz = (int)it->data.size();

        if( it->headCt + siz/nChans > fromCt ) {

            data = it->data;
            usrFlt( data );

            if( fromCt > it->headCt )
                cur = &data[chan + (fromCt - it->headCt)*nChans];
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
    if( (cur += nChans) >= lim ) {

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

AIQ::AIQ( double srate, int nChans, int capacitySecs )
    :   srate(srate),
        maxCts(capacitySecs * srate),
        nChans(nChans),
        curCts(0)
{
}


void AIQ::enqueue( vec_i16 &src, int nWhole, quint64 headCt )
{
    const int   maxScansPerBlk = 100;
    double      nowT = getTime();

    QMutexLocker    ml( &QMtx );

    updateQCts( nWhole );

    if( nWhole <= maxScansPerBlk )
        Q.push_back( AIQBlock( src, nWhole * nChans, headCt, nowT ) );
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
                    offset * nChans,
                    maxScansPerBlk * nChans,
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
                offset * nChans,
                nhalf * nChans,
                headCt,
                tailT += nhalf * delT ) );

        offset += nhalf;
        headCt += nhalf;
        nWhole -= nhalf;

        // Second half

        Q.push_back(
            AIQBlock(
                src,
                offset * nChans,
                nWhole * nChans,
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
        return B.headCt + B.data.size() / nChans;
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

    std::deque<AIQBlock>::const_iterator it = Q.begin(), end = Q.end();

    for( ; it != end; ++it ) {

        if( it->tailT >= t ) {

            int tail = int((it->tailT - t) * srate) + 1,
                size = (int)it->data.size() / nChans;

            ct      = it->headCt + (tail < size ? size - tail : 0);
            found   = true;
            break;
        }
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

    std::deque<AIQBlock>::const_iterator it = Q.begin(), end = Q.end();

    for( ; it != end; ++it ) {

        int thisCt = (int)it->data.size() / nChans;

        if( it->headCt + thisCt > ct ) {

            if( it->headCt <= ct ) {
                t       = it->tailT - (it->headCt + thisCt - 1 - ct) / srate;
                found   = true;
            }

            break;
        }
    }

    QMtx.unlock();

    return found;
}


// If vB.size() > 1 then the data are concatenated into cat,
// and a reference to cat is returned.
//
// Otherwise, nothing is done to cat, and a reference to
// vB[0].data is returned.
//
vec_i16 &AIQ::catBlocks( vec_i16 &cat, std::vector<AIQBlock> &vB ) const
{
    int nb = (int)vB.size();

    if( nb > 1 ) {

        cat.clear();

        for( int i = 0; i < nb; ++i ) {
            const vec_i16   &D = vB[i].data;
            cat.insert( cat.end(), D.begin(), D.end() );
        }

        return cat;
    }

    return vB[0].data;
}


// Return sum of scans in vB.
//
quint64 AIQ::sumCt( std::vector<AIQBlock> &vB ) const
{
    quint64 sum = 0;
    int     nb  = (int)vB.size();

    for( int i = 0; i < nb; ++i )
        sum += vB[i].data.size();

    return sum / nChans;
}


// Return count following vB.
//
quint64 AIQ::nextCt( std::vector<AIQBlock> &vB ) const
{
    int             nb = (int)vB.size();
    const AIQBlock  &B = vB[nb-1];

    return B.headCt + B.data.size() / nChans;
}


// Copy 1 block with headCt >= fromCt.
// Return true if found.
//
bool AIQ::copy1BlockFromCt(
    vec_i16 &dest,
    quint64 &headCt,
    quint64 fromCt ) const
{
    QMutexLocker    ml( &QMtx );

    std::deque<AIQBlock>::const_iterator it = Q.begin(), end = Q.end();

    for( ; it != end; ++it ) {

        if( it->headCt >= fromCt ) {

            dest    = it->data;
            headCt  = it->headCt;
            return true;
        }
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

    std::deque<AIQBlock>::const_iterator it = Q.begin(), end = Q.end();

    for( ; it != end; ++it ) {

        if( it->tailT >= fromT ) {

            dest.push_back( *it );
            ++nb;
        }
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        int nTot = (int)D.size() / nChans,
            keep = (B.tailT - fromT) * srate;

        if( keep <= 0 )
            keep = 1;

        if( keep < nTot ) {

            B.headCt += nTot - keep;
            D.erase( D.begin(), D.begin() + nChans*(nTot - keep) );
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

    std::deque<AIQBlock>::const_iterator it = Q.begin(), end = Q.end();

    for( ; it != end; ++it ) {

        if( it->headCt + it->data.size() / nChans > fromCt ) {

            dest.push_back( *it );
            ++nb;
        }
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        if( fromCt > B.headCt ) {

            D.erase( D.begin(), D.begin() + nChans*(fromCt - B.headCt) );
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

    QMtx.lock();

    std::deque<AIQBlock>::const_iterator it = Q.begin(), end = Q.end();

    for( ; it != end; ++it ) {

        if( it->tailT >= fromT ) {

            int thisCt = (int)it->data.size() / nChans;

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
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B      = dest[0];
        vec_i16     &D      = B.data;
        int         keep    = int((B.tailT - fromT) / srate) + 1,
                    ct0     = (int)D.size() / nChans;

        if( keep < ct0 ) {
            ct0 -= keep;    // ct0 -> diff
            D.erase( D.begin(), D.begin() + nChans*ct0 );
            B.headCt += ct0;
        }

        if( ct > nMax ) {

            AIQBlock    &B = dest[nb-1];
            vec_i16     &D = B.data;

            ct -= nMax; // ct -> diff

            D.erase( D.begin() + D.size() - nChans*ct, D.end() );
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

    QMtx.lock();

    std::deque<AIQBlock>::const_iterator it = Q.begin(), end = Q.end();

    for( ; it != end; ++it ) {

        int thisCt = (int)it->data.size() / nChans;

        if( it->headCt + thisCt > fromCt ) {

            dest.push_back( *it );
            ct += thisCt;

            if( !nb++ && fromCt > it->headCt )
                ct -= fromCt - it->headCt;

            if( ct >= nMax )
                break;
        }
    }

    QMtx.unlock();

    if( nb ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        if( fromCt > B.headCt ) {
            D.erase( D.begin(), D.begin() + nChans*(fromCt - B.headCt) );
            B.headCt = fromCt;
        }

        if( ct > nMax ) {

            AIQBlock    &B = dest[nb-1];
            vec_i16     &D = B.data;

            ct -= nMax; // ct -> diff

            D.erase( D.begin() + D.size() - nChans*ct, D.end() );
            B.tailT -= ct / srate;
        }
    }

    return nb;
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

    QMtx.lock();

    if( !Q.empty() ) {

        // Work backwards, starting with newest block,
        // and prepending until reach scan count nMax.

        std::deque<AIQBlock>::const_iterator begin = Q.begin(), it = Q.end();

        do {
            --it;

            dest.insert( dest.begin(), *it );
            ct += (int)it->data.size() / nChans;
            ++nb;

        } while( ct < nMax && it != begin );
    }

    QMtx.unlock();

    if( ct > nMax ) {

        AIQBlock    &B = dest[0];
        vec_i16     &D = B.data;

        ct -= nMax;

        D.erase( D.begin(), D.begin() + nChans*ct );
        B.headCt += ct;
    }

    return nb;
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

    VBWalker    W( Q, nChans, chan );

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

            // Check run length
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
            outCt = W.endCt();
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

    VBFltWalker    W( Q, usrFlt, nChans, chan );

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

            // Check run length
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
            outCt = W.endCt();
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

    VBWalker    W( Q, nChans, chan );

    if( !W.SetStart( fromCt ) )
        goto exit;

// -------------------
// Seek edge candidate
// -------------------

    while( W.next() ) {

        if( *W.cur < T ) {

            // Mark edge start
            outCt   = W.curCt();
            nlo     = 1;

            // Check run length
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
            outCt = W.endCt();
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
            curCts -= Q.front().data.size() / nChans;
            Q.pop_front();

        } while( curCts > maxCts );
    }
}


