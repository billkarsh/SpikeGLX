
#include "TrigTTL.h"
#include "Util.h"
#include "MainApp.h"
#include "Run.h"

#include <QThread>


#define LOOP_MS     100


static TrigTTL      *ME;


/* ---------------------------------------------------------------- */
/* TrTTLWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void TrTTLWorker::run()
{
    const int   niq = viq.size();
    bool        ok  = true;

    for(;;) {

        if( !shr.wake( ok ) )
            break;

        for( int iiq = 0; iiq < niq; ++iiq ) {

            if( shr.preMidPost == -1 )
                ok = writePreMargin( viq[iiq] );
            else if( !shr.preMidPost )
                ok = doSomeH( viq[iiq] );
            else
                ok = writePostMargin( viq[iiq] );

            if( !ok )
                break;
        }
    }

    emit finished();
}


// Write margin up to but not including rising edge.
//
// Return true if no errors.
//
bool TrTTLWorker::writePreMargin( int iq )
{
    const SyncStream    &S = vS[iq];
    TrigTTL::Counts     &C = ME->cnt;

    if( C.remCt[iq] <= 0 )
        return true;

    vec_i16 data;
    quint64 headCt  = C.nextCt[iq];
    int     nMax    = (C.remCt[iq] <= C.maxFetch[iq] ?
                        C.remCt[iq] : C.maxFetch[iq]);

    if( !ME->nSampsFromCt( data, headCt, nMax, S.js, S.ip ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// Status in this state should be what's done: +(margin - rem).
// If next = edge - rem, then status = +(margin + next - edge).
//
// When rem falls to zero, (next = edge) sets us up for state H.

    C.remCt[iq] -= size / S.Q->nChans();
    C.nextCt[iq] = C.edgeCt[iq] - C.remCt[iq];

    return ME->writeAndInvalData( S.js, S.ip, data, headCt );
}


// Write margin, including falling edge.
//
// Return true if no errors.
//
bool TrTTLWorker::writePostMargin( int iq )
{
    const SyncStream    &S = vS[iq];
    TrigTTL::Counts     &C = ME->cnt;

    if( C.remCt[iq] <= 0 )
        return true;

    vec_i16 data;
    quint64 headCt  = C.nextCt[iq];
    int     nMax    = (C.remCt[iq] <= C.maxFetch[iq] ?
                        C.remCt[iq] : C.maxFetch[iq]);

    if( !ME->nSampsFromCt( data, headCt, nMax, S.js, S.ip ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// Status in this state should be: +(margin + H + margin - rem).
// With next defined as below, status = +(margin + next - edge)
// = margin + (fall-edge) + margin - rem = correct.

    C.remCt[iq] -= size / S.Q->nChans();
    C.nextCt[iq] = C.fallCt[iq] + C.marginCt[iq] - C.remCt[iq];

    return ME->writeAndInvalData( S.js, S.ip, data, headCt );
}


// Write from rising edge up to but not including falling edge.
//
// Return true if no errors.
//
bool TrTTLWorker::doSomeH( int iq )
{
    const SyncStream    &S = vS[iq];
    TrigTTL::Counts     &C = ME->cnt;

    vec_i16 data;
    quint64 headCt = C.nextCt[iq];
    bool    ok;

// ---------------
// Fetch a la mode
// ---------------

    if( shr.p.trgTTL.mode == DAQ::TrgTTLLatch )
        ok = ME->nSampsFromCt( data, headCt, -LOOP_MS, S.js, S.ip );
    else if( C.remCt[iq] <= 0 )
        return true;
    else {

        int nMax = (C.remCt[iq] <= C.maxFetch[iq] ?
                    C.remCt[iq] : C.maxFetch[iq]);

        ok = ME->nSampsFromCt( data, headCt, nMax, S.js, S.ip );
    }

    if( !ok )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// ------------------------
// Write/update all H cases
// ------------------------

    C.nextCt[iq]    += size / S.Q->nChans();
    C.remCt[iq]     -= C.nextCt[iq] - headCt;

    return ME->writeAndInvalData( S.js, S.ip, data, headCt );
}

/* ---------------------------------------------------------------- */
/* TrTTLThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrTTLThread::TrTTLThread(
    TrTTLShared             &shr,
    std::vector<SyncStream> &vS,
    std::vector<int>        &viq )
{
    thread  = new QThread;
    worker  = new TrTTLWorker( shr, vS, viq );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


TrTTLThread::~TrTTLThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* Counts --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigTTL::Counts::Counts( const DAQ::Params &p )
    :   iTrk(p.stream2iq(p.trgTTL.stream)), nq(p.stream_nq())
{
    edgeCt.assign( nq, 0 );
    fallCt.assign( nq, 0 );
    nextCt.assign( nq, 0 );
    remCt.assign( nq, 0 );

    hiCtMax.resize( nq );
    marginCt.resize( nq );
    refracCt.resize( nq );
    maxFetch.resize( nq );

    for( int iq = 0; iq < nq; ++iq ) {

        double  srate;
        int     ip, js = p.iq2jsip( ip, iq );

        switch( js ) {
            case jsNI: srate = p.ni.srate; break;
            case jsOB: srate = p.im.obxj[ip].srate; break;
            case jsIM: srate = p.im.prbj[ip].srate; break;
        }

        hiCtMax[iq]     =
            (p.trgTTL.mode == DAQ::TrgTTLTimed ?
             qint64(p.trgTTL.tH * srate) : UNSET64);

        marginCt[iq]    = p.trgTTL.marginSecs * srate;
        refracCt[iq]    = p.trgTTL.refractSecs * srate;
        maxFetch[iq]    = 0.400 * srate;
    }
}


// Set from and rem for universal premargin.
//
void TrigTTL::Counts::setPreMarg()
{
    remCt = marginCt;

    for( int iq = 0; iq < nq; ++iq )
        nextCt[iq] = edgeCt[iq] - remCt[iq];
}


// Set from, rem and fall (if applicable) for H phase.
//
void TrigTTL::Counts::setH( DAQ::TrgTTLMode mode )
{
    nextCt = edgeCt;

    if( mode == DAQ::TrgTTLLatch ) {

        remCt = hiCtMax;
        // fallCt N.A.
    }
    else if( mode == DAQ::TrgTTLTimed ) {

        remCt = hiCtMax;

        for( int iq = 0; iq < nq; ++iq )
            fallCt[iq] = edgeCt[iq] + remCt[iq];
    }
    else {

        // remCt must be set within H-writer which seeks
        // and sets true fallCt. Here we must zero fallCt.

        fallCt.assign( nq, 0 );
    }
}


// Set from and rem for postmargin; not applicable in latched mode.
//
void TrigTTL::Counts::setPostMarg()
{
    remCt   = marginCt;
    nextCt  = fallCt;
}


bool TrigTTL::Counts::allFallCtSet()
{
    for( int iq = 0; iq < nq; ++iq ) {

        if( !fallCt[iq] )
            return false;
    }

    return true;
}


bool TrigTTL::Counts::remCtDone()
{
    for( int iq = 0; iq < nq; ++iq ) {

        if( remCt[iq] > 0 )
            return false;
    }

    return true;
}


void TrigTTL::Counts::advanceByTime()
{
    for( int iq = 0; iq < nq; ++iq )
        nextCt[iq] = edgeCt[iq] + qMax( hiCtMax[iq], refracCt[iq] );
}


void TrigTTL::Counts::advancePastFall()
{
    for( int iq = 0; iq < nq; ++iq )
        nextCt[iq] = qMax( edgeCt[iq] + refracCt[iq], fallCt[iq] + 1 );
}


bool TrigTTL::Counts::isTracking()
{
    return nextCt[iTrk] > 0;
}


double TrigTTL::Counts::L_progress()
{
    return (nextCt[iTrk] - edgeCt[iTrk]) / ME->vS[iTrk].Q->sRate();
}


double TrigTTL::Counts::H_progress()
{
    return (marginCt[iTrk] + nextCt[iTrk] - edgeCt[iTrk]) / ME->vS[iTrk].Q->sRate();
}

/* ---------------------------------------------------------------- */
/* TrigTTL -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigTTL::TrigTTL(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const QVector<AIQ*> &imQ,
    const QVector<AIQ*> &obQ,
    const AIQ           *niQ )
    :   TrigBase(p, gw, imQ, obQ, niQ),
        cnt(p),
        highsMax(p.trgTTL.isNInf ? UNSET64 : p.trgTTL.nH),
        aEdgeCtNext(0),
        thresh(p.trigThreshAsInt()),
        digChan(p.trgTTL.isAnalog ? -1 : p.trigChan())
{
    vEdge.resize( cnt.nq );
}


#define TINYMARG            0.0001

#define ISSTATE_L           (state == 0)
#define ISSTATE_PreMarg     (state == 1)
#define ISSTATE_H           (state == 2)
#define ISSTATE_PostMarg    (state == 3)
#define ISSTATE_Done        (state == 4)


// TTL logic is driven by TrgTTLParams:
// {marginSecs, refractSecs, tH, mode, inarow, nH, T}.
// Corresponding states defined above.
//
// There are three TTL modes: {latched, timed, follower}.
// -latched  goes high on triggered; stays high for whole gate.
// -timed    goes high on triggered; stays high for tH.
// -follower goes high on triggered; stays high while chan high.
//
void TrigTTL::run()
{
    Debug() << "Trigger thread started.";

// ---------
// Configure
// ---------

    ME = this;

// Create workers and threads

    std::vector<TrTTLThread*>   trT;
    TrTTLShared                 shr( p );
    const int                   nPrbPerThd  = 2;

    iqMax   = cnt.nq - 1;
    nThd    = 0;

    for( int iq0 = 0; iq0 < iqMax; iq0 += nPrbPerThd ) {

        std::vector<int>    viq;

        for( int k = 0; k < nPrbPerThd; ++k ) {

            if( iq0 + k < iqMax )
                viq.push_back( iq0 + k );
            else
                break;
        }

        trT.push_back( new TrTTLThread( shr, vS, viq ) );
        ++nThd;
    }

    {   // local worker (last stream)
        std::vector<int>    iq1( 1, iqMax );
        locWorker = new TrTTLWorker( shr, vS, iq1 );
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                QThread::usleep( 10 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

// -----
// Start
// -----

    setYieldPeriod_ms( LOOP_MS );

    initState();

    QString err;

    while( !isStopped() ) {

        double  loopT = getTime();
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

        if( inactive ) {

            endTrig();
            initState();
            goto next_loop;
        }

        // ---------------------------------
        // L phase (waiting for rising edge)
        // ---------------------------------

        if( ISSTATE_L ) {

            if( !getRiseEdge() )
                goto next_loop;

            if( p.trgTTL.marginSecs > TINYMARG )
                SETSTATE_PreMarg();
            else
                SETSTATE_H();

            // -------------------
            // Open files together
            // -------------------

            {
                int ig, it;

                if( !newTrig( ig, it ) ) {
                    err = "open file failed";
                    break;
                }
            }
        }

        // ----------------
        // Write pre-margin
        // ----------------

        if( ISSTATE_PreMarg ) {

            if( !xferAll( shr, -1, err ) )
                break;

            if( cnt.remCtDone() )
                SETSTATE_H();
        }

        // -------
        // Write H
        // -------

        if( ISSTATE_H ) {

            // Set falling edge

            if( p.trgTTL.mode == DAQ::TrgTTLFollowV && !cnt.fallCt[cnt.iTrk] )
                getFallEdge();

            // Write

            if( !xferAll( shr, 0, err ) )
                break;

            // Done?

            if( p.trgTTL.mode == DAQ::TrgTTLLatch )
                goto next_loop;

            // In TrgTTLFollowV mode, remCt isn't set until fallCt is set.

            if( p.trgTTL.mode == DAQ::TrgTTLFollowV && !cnt.allFallCtSet() )
                goto next_loop;

            // Check for zero remainder

            if( cnt.remCtDone() ) {

                if( p.trgTTL.marginSecs <= TINYMARG )
                    goto check_done;
                else
                    SETSTATE_PostMarg();
            }
        }

        // -----------------
        // Write post-margin
        // -----------------

        if( ISSTATE_PostMarg ) {

            if( !xferAll( shr, 1, err ) )
                break;

            // Done?

            if( cnt.remCtDone() ) {

check_done:
                if( ++nHighs >= highsMax ) {
                    SETSTATE_Done();
                    inactive = true;
                }
                else {

                    if( p.trgTTL.mode == DAQ::TrgTTLTimed )
                        cnt.advanceByTime();
                    else
                        cnt.advancePastFall();

                    // Strictly for L status message
                    cnt.edgeCt = cnt.nextCt;
                    SETSTATE_L();
                }

                endTrig();
            }
        }

        // ------
        // Status
        // ------

next_loop:
        if( loopT - statusT > 1.0 ) {

            QString sOn, sT, sWr;

            statusOnSince( sOn );
            statusProcess( sT, inactive );
            statusWrPerf( sWr );

            Status() << sOn << sT << sWr;

            statusT = loopT;
        }

        // -------------------
        // Moderate fetch rate
        // -------------------

        yield( loopT );
    }

// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd ) {
        trT[iThd]->thread->wait( 10000/nThd );
        delete trT[iThd];
    }

    delete locWorker;

// Done

    endRun( err );
}


void TrigTTL::SETSTATE_L()
{
    aEdgeCtNext = 0;
    aFallCtNext = 0;
    state = 0;
}


// Set from and rem for universal premargin.
//
void TrigTTL::SETSTATE_PreMarg()
{
    cnt.setPreMarg();
    state = 1;
}


// Set from, rem and fall (if applicable) for H phase.
//
void TrigTTL::SETSTATE_H()
{
    cnt.setH( DAQ::TrgTTLMode(p.trgTTL.mode) );
    state = 2;
}


// Set from and rem for postmargin;
// not applicable in latched mode.
//
void TrigTTL::SETSTATE_PostMarg()
{
    if( p.trgTTL.mode != DAQ::TrgTTLLatch )
        cnt.setPostMarg();

    state = 3;
}


void TrigTTL::SETSTATE_Done()
{
    state = 4;
    mainApp()->getRun()->dfSetRecordingEnabled( false, true );
}


void TrigTTL::initState()
{
    cnt.nextCt.assign( iqMax + 1, 0 );
    nHighs = 0;
    SETSTATE_L();
}


// Find rising edge in source stream;
// translate to all streams 'vEdge'.
//
bool TrigTTL::_getRiseEdge()
{
    quint64 srcNextCt;
    int     iSrc = cnt.iTrk;

    srcNextCt = cnt.nextCt[iSrc];

// First edge is the gate edge for (state L) status report
// wherein edgeCt is just time we started seeking an edge.

    if( !srcNextCt ) {
        int where = vS[iSrc].Q->mapTime2Ct( srcNextCt, getGateHiT() );
        if( where != 0 )
            return false;
    }

// It may take several tries to achieve pulser sync for multi streams.
// aEdgeCtNext saves us from costly refinding of edge-A while hunting.

    bool    found;

    if( aEdgeCtNext )
        found = true;
    else {

        if( digChan < 0 ) {

            found = vS[iSrc].Q->findRisingEdge(
                        aEdgeCtNext,
                        srcNextCt,
                        p.trgTTL.chan,
                        thresh,
                        p.trgTTL.inarow );
        }
        else {
            found = vS[iSrc].Q->findBitRisingEdge(
                        aEdgeCtNext,
                        srcNextCt,
                        digChan,
                        p.trgTTL.bit % 16,
                        p.trgTTL.inarow );
        }

        if( !found ) {
            srcNextCt   = aEdgeCtNext;  // pick up search here
            aEdgeCtNext = 0;
        }
    }

    if( found && iqMax > 0 ) {

        syncDstTAbsMult( aEdgeCtNext, iSrc, vS, p );

        for( int iq = 0; iq <= iqMax; ++iq ) {

            if( iq != iSrc ) {

                const SyncStream    &S = vS[iq];

                if( p.sync.sourceIdx != DAQ::eSyncSourceNone && !S.bySync )
                    return false;

                vEdge[iq] = S.TAbs2Ct( S.tAbs );
            }
        }
    }

    if( found ) {
        vEdge[iSrc] = aEdgeCtNext;
        srcNextCt   = aEdgeCtNext;  // for status tracking
    }

    return found;
}


// Find falling edge in source stream;
// translate to all streams 'vEdge'.
//
// nextCt tracks where we are writing,
// aFallCtNext tracks edge hunting.
//
bool TrigTTL::_getFallEdge()
{
    int     iSrc = cnt.iTrk;
    bool    found;

// new or continuing hunt?

    if( !aFallCtNext )
        aFallCtNext = cnt.edgeCt[iSrc];

    if( digChan < 0 ) {

        found = vS[iSrc].Q->findFallingEdge(
                    aFallCtNext,
                    aFallCtNext,
                    p.trgTTL.chan,
                    thresh,
                    p.trgTTL.inarow );
    }
    else {
        found = vS[iSrc].Q->findBitFallingEdge(
                    aFallCtNext,
                    aFallCtNext,
                    digChan,
                    p.trgTTL.bit % 16,
                    p.trgTTL.inarow );
    }

    vEdge[iSrc] = aFallCtNext;

    if( iqMax > 0 ) {

        syncDstTAbsMult( aFallCtNext, iSrc, vS, p );

        for( int iq = 0; iq <= iqMax; ++iq ) {

            if( iq != iSrc ) {
                const SyncStream    &S = vS[iq];
                vEdge[iq] = S.TAbs2Ct( S.tAbs );
            }
        }
    }

    return found;
}


// Find source edge; copy from vEdge to Count records.
//
bool TrigTTL::getRiseEdge()
{
    if( !_getRiseEdge() )
        return false;

    cnt.edgeCt = vEdge;
    return true;
}


// Set fallCt(s) if edge found, set remCt(s) whether found or not.
//
void TrigTTL::getFallEdge()
{
    if( _getFallEdge() )
        cnt.fallCt = vEdge;

    for( int iq = 0; iq <= iqMax; ++iq )
        cnt.remCt[iq] = vEdge[iq] - cnt.nextCt[iq];
}


// Set preMidPost to {-1,0,1} to select {premargin, H, postmargin}.
//
// Return true if no errors.
//
bool TrigTTL::xferAll( TrTTLShared &shr, int preMidPost, QString &err )
{
    bool    maxOK;

    shr.preMidPost  = preMidPost;
    shr.awake       = 0;
    shr.asleep      = 0;
    shr.errors      = 0;

// Wake all threads

    shr.condWake.wakeAll();

// Do last stream locally

    if( preMidPost == -1 )
        maxOK = locWorker->writePreMargin( iqMax );
    else if( !preMidPost )
        maxOK = locWorker->doSomeH( iqMax );
    else
        maxOK = locWorker->writePostMargin( iqMax );

// Wait all threads started, and all done

    shr.runMtx.lock();
        while( shr.awake  < nThd
            || shr.asleep < nThd ) {

            shr.runMtx.unlock();
                QThread::msleep( LOOP_MS/8 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

    if( maxOK && !shr.errors )
        return true;

    err = "write failed";
    return false;
}


void TrigTTL::statusProcess( QString &sT, bool inactive )
{
    if( inactive || !cnt.isTracking() )
        sT = " TX";
    else if( ISSTATE_L )
        sT = QString(" T-%1s").arg( cnt.L_progress(), 0, 'f', 1 );
    else
        sT = QString(" T+%1s").arg( cnt.H_progress(), 0, 'f', 1 );
}


