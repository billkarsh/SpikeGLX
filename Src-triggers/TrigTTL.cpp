
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
    const int   nID = vID.size();
    bool        ok  = true;

    for(;;) {

        if( !shr.wake( ok ) )
            break;

        for( int iID = 0; iID < nID; ++iID ) {

            if( shr.preMidPost == -1 )
                ok = writePreMarginIm( vID[iID] );
            else if( !shr.preMidPost )
                ok = doSomeHIm( vID[iID] );
           else
                ok = writePostMarginIm( vID[iID] );

            if( !ok )
                break;
        }
    }

    emit finished();
}


// Write margin up to but not including rising edge.
//
// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrTTLWorker::writePreMarginIm( int ip )
{
    TrigTTL::CountsIm   &C = ME->imCnt;

    if( C.remCt[ip] <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = imQ[ip]->getNScansFromCt(
            vB,
            C.edgeCt[ip] - C.remCt[ip],
            (C.remCt[ip] <= C.maxFetch ? C.remCt[ip] : C.maxFetch) );

    if( !nb )
        return true;

// Status in this state should be what's done: +(margin - rem).
// If next = edge - rem, then status = +(next - edge + margin).
//
// When rem falls to zero, (next = edge) sets us up for state H.

    C.remCt[ip] -= imQ[ip]->sumCt( vB );
    C.nextCt[ip] = C.edgeCt[ip] - C.remCt[ip];

    return ME->writeAndInvalVB( ME->DstImec, ip, vB );
}


// Write margin, including falling edge.
//
// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrTTLWorker::writePostMarginIm( int ip )
{
    TrigTTL::CountsIm   &C = ME->imCnt;

    if( C.remCt[ip] <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = imQ[ip]->getNScansFromCt(
            vB,
            C.fallCt[ip] + C.marginCt - C.remCt[ip],
            (C.remCt[ip] <= C.maxFetch ? C.remCt[ip] : C.maxFetch) );

    if( !nb )
        return true;

// Status in this state should be: +(margin + H + margin - rem).
// With next defined as below, status = +(next - edge + margin)
// = margin + (fall-edge) + margin - rem = correct.

    C.remCt[ip] -= imQ[ip]->sumCt( vB );
    C.nextCt[ip] = C.fallCt[ip] + C.marginCt - C.remCt[ip];

    return ME->writeAndInvalVB( ME->DstImec, ip, vB );
}


// Write from rising edge up to but not including falling edge.
//
// Return true if no errors.
//
bool TrTTLWorker::doSomeHIm( int ip )
{
    TrigTTL::CountsIm   &C = ME->imCnt;

    if( shr.p.trgTTL.mode != DAQ::TrgTTLFollowAI && !C.remCt[ip] )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// ---------------
// Fetch a la mode
// ---------------

    if( shr.p.trgTTL.mode == DAQ::TrgTTLLatch ) {

        // Latched case
        // Get all since last fetch

        nb = imQ[ip]->getAllScansFromCt( vB, C.nextCt[ip] );
    }
    else if( shr.p.trgTTL.mode == DAQ::TrgTTLTimed ) {

        // Timed case
        // In-progress H fetch using counts

        C.fallCt[ip]  = C.edgeCt[ip] + C.hiCtMax;
        C.remCt[ip]   = C.hiCtMax - (C.nextCt[ip] - C.edgeCt[ip]);

        nb = imQ[ip]->getNScansFromCt(
                vB,
                C.nextCt[ip],
                (C.remCt[ip] <= C.maxFetch ? C.remCt[ip] : C.maxFetch) );
    }
    else {

        // Follower case
        // Fetch up to falling edge

        if( !C.fallCt[ip] )
            ME->getFallEdge();
        else
            C.remCt[ip] = C.fallCt[ip] - C.nextCt[ip];

        nb = imQ[ip]->getNScansFromCt(
                vB,
                C.nextCt[ip],
                (C.remCt[ip] <= C.maxFetch ? C.remCt[ip] : C.maxFetch) );
    }

// ------------------------
// Write/update all H cases
// ------------------------

    if( !nb )
        return true;

    C.nextCt[ip] = imQ[ip]->nextCt( vB );
    C.remCt[ip] -= C.nextCt[ip] - vB[0].headCt;

    return ME->writeAndInvalVB( ME->DstImec, ip, vB );
}

/* ---------------------------------------------------------------- */
/* TrTTLThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrTTLThread::TrTTLThread(
    TrTTLShared         &shr,
    const QVector<AIQ*> &imQ,
    QVector<int>        &vID )
{
    thread  = new QThread;
    worker  = new TrTTLWorker( shr, imQ, vID );

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
/* TrigTTL -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigTTL::TrigTTL(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const QVector<AIQ*> &imQ,
    const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        imCnt( p, p.im.all.srate ),
        niCnt( p, p.ni.srate ),
        nCycMax(
            p.trgTTL.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgTTL.nH),
        aEdgeCtNext(0),
        thresh(
            p.trgTTL.stream == "nidq" ?
            p.ni.vToInt16( p.trgTTL.T, p.trgTTL.chan )
            : p.im.vToInt10( p.trgTTL.T, p.streamID( p.trgTTL.stream ),
                p.trgTTL.chan )),
        digChan(
            p.trgTTL.isAnalog ? -1 :
            (p.trgTTL.stream == "nidq" ?
             p.ni.niCumTypCnt[CniCfg::niSumAnalog] + p.trgTTL.bit/16
             : p.im.each[p.streamID( p.trgTTL.stream )]
                .imCumTypCnt[CimCfg::imSumNeural]))
{
}


void TrigTTL::setGate( bool hi )
{
    runMtx.lock();
    initState();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigTTL::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
    initState();
    runMtx.unlock();
}


#define SETSTATE_PreMarg    (state = 1)
#define SETSTATE_PostMarg   (state = 3)

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

// Create worker threads

    const int               nPrbPerThd = 2;

    QVector<TrTTLThread*>   trT;
    TrTTLShared             shr( p );

    nThd = 0;

    for( int ip0 = 0; ip0 < nImQ; ip0 += nPrbPerThd ) {

        QVector<int>    vID;

        for( int id = 0; id < nPrbPerThd; ++id ) {

            if( ip0 + id < nImQ )
                vID.push_back( ip0 + id );
            else
                break;
        }

        trT.push_back( new TrTTLThread( shr, imQ, vID ) );
        ++nThd;
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                usleep( 10 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

// -----
// Start
// -----

    setYieldPeriod_ms( LOOP_MS );

    initState();

    while( !isStopped() ) {

        double  loopT = getTime();
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

        if( inactive ) {

            endTrig();
            goto next_loop;
        }

        // ---------------------------------
        // L phase (waiting for rising edge)
        // ---------------------------------

        if( ISSTATE_L ) {

            if( !getRiseEdge() )
                goto next_loop;

            imCnt.remCt.fill( imCnt.marginCt, nImQ );
            niCnt.remCt = niCnt.marginCt;

            if( niCnt.marginCt )
                SETSTATE_PreMarg;
            else
                SETSTATE_H();

            // -------------------
            // Open files together
            // -------------------

            {
                int ig, it;

                if( !newTrig( ig, it ) )
                    break;
            }

            ++nH;
        }

        // ----------------
        // Write pre-margin
        // ----------------

        if( ISSTATE_PreMarg ) {

            if( !xferAll( shr, -1 ) )
                goto endrun;

            if( (!niQ || niCnt.remCt <= 0) && imCnt.remCtDone() )
                SETSTATE_H();
        }

        // -------
        // Write H
        // -------

        if( ISSTATE_H ) {

            if( !xferAll( shr, 0 ) )
                goto endrun;

            // Done?

            if( p.trgTTL.mode == DAQ::TrgTTLLatch )
                goto next_loop;

            if( p.trgTTL.mode == DAQ::TrgTTLFollowAI ) {

                // In this mode, a zero remainder doesn't mean done.

                if( p.trgTTL.stream == "nidq" ) {

                    if( !niCnt.fallCt )
                        goto next_loop;
                }
                else {

                    if( !imCnt.fallCt[0] )
                        goto next_loop;
                }
            }

            // Check for zero remainder

            if( (!niQ || niCnt.remCt <= 0) && imCnt.remCtDone() ) {

                if( !niCnt.marginCt )
                    goto check_done;
                else {
                    imCnt.remCt.fill( imCnt.marginCt, nImQ );
                    niCnt.remCt = niCnt.marginCt;
                    SETSTATE_PostMarg;
                }
            }
        }

        // -----------------
        // Write post-margin
        // -----------------

        if( ISSTATE_PostMarg ) {

            if( !xferAll( shr, 1 ) )
                goto endrun;

            // Done?

            if( (!niQ || niCnt.remCt <= 0) && imCnt.remCtDone() ) {

check_done:
                if( nH >= nCycMax ) {
                    SETSTATE_Done();
                    inactive = true;
                }
                else {

                    if( p.trgTTL.mode == DAQ::TrgTTLTimed ) {

                        imCnt.advanceByTime();
                        niCnt.advanceByTime();
                    }
                    else {
                        imCnt.advancePastFall();
                        niCnt.advancePastFall();
                    }

                    imCnt.edgeCt = imCnt.nextCt;
                    niCnt.edgeCt = niCnt.nextCt;
                    SETSTATE_L();
                }

                endTrig();
            }
        }

        // ------
        // Status
        // ------

next_loop:
        if( loopT - statusT > 0.25 ) {

            QString sOn, sT, sWr;
            int     ig, it;

            getGT( ig, it );
            statusOnSince( sOn, loopT, ig, it );
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

endrun:
// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd )
        delete trT[iThd];

// Done

    endRun();

    Debug() << "Trigger thread stopped.";

    emit finished();
}


void TrigTTL::SETSTATE_L()
{
    state           = 0;
    aEdgeCtNext     = 0;
    imCnt.fallCt.fill( 0, nImQ );
    niCnt.fallCt    = 0;
}


void TrigTTL::SETSTATE_H()
{
    state       = 2;
    imCnt.remCt.fill( -1, nImQ );
    niCnt.remCt = -1;
}


void TrigTTL::SETSTATE_Done()
{
    state = 4;
    mainApp()->getRun()->dfSetRecordingEnabled( false, true );
}


void TrigTTL::initState()
{
    imCnt.nextCt.fill( 0, nImQ );
    niCnt.nextCt    = 0;
    nH              = 0;
    SETSTATE_L();
}


// Find rising edge in stream A...but translate time-points to stream B.
//
bool TrigTTL::_getRiseEdge(
    quint64     &aNextCt,
    const AIQ   *qA,
    quint64     &bNextCt,
    const AIQ   *qB )
{
    bool    found;

// First edge is the gate edge for (state L) status report
// wherein edgeCt is just time we started seeking an edge.

    if( !aNextCt ) {

        double  gateT = getGateHiT();

        if( !qA->mapTime2Ct( aNextCt, gateT ) )
            return false;

        if( qB )
            qB->mapTime2Ct( bNextCt, gateT );
    }

// For multistream, we need mappable data for both A and B.
// aEdgeCtNext saves us from costly refinding of A in cases
// where A already succeeded but B not yet.

    if( aEdgeCtNext )
        found = true;
    else {

        if( digChan < 0 ) {

            found = qA->findRisingEdge(
                        aEdgeCtNext,
                        aNextCt,
                        p.trgTTL.chan,
                        thresh,
                        p.trgTTL.inarow );
        }
        else {
            found = qA->findBitRisingEdge(
                        aEdgeCtNext,
                        aNextCt,
                        digChan,
                        p.trgTTL.bit,
                        p.trgTTL.inarow );
        }

        if( !found ) {
            aNextCt     = aEdgeCtNext;  // pick up search here
            aEdgeCtNext = 0;
        }
    }

    quint64 bOutCt;

    if( found && qB ) {

        double  wallT;

        qA->mapCt2Time( wallT, aEdgeCtNext );
        found = qB->mapTime2Ct( bOutCt, wallT );
    }

    if( found ) {

        alignX12( qA, aEdgeCtNext, bOutCt );

        aNextCt     = aEdgeCtNext;
        aFallCtNext = aEdgeCtNext;
        aEdgeCtNext = 0;

        if( qB )
            bNextCt = bOutCt;
    }

    return found;
}


// Find falling edge in stream A...but translate time-points to stream B.
//
bool TrigTTL::_getFallEdge(
    quint64     &aFallCt,
    const AIQ   *qA,
    quint64     &bFallCt,
    const AIQ   *qB )
{
    bool    found;

// For multistream, we need mappable data for both A and B.
// aEdgeCtNext saves us from costly refinding of A in cases
// where A already succeeded but B not yet.

    if( aEdgeCtNext )
        found = true;
    else {

        if( digChan < 0 ) {

            found = qA->findFallingEdge(
                        aFallCtNext,
                        aFallCtNext,
                        p.trgTTL.chan,
                        thresh,
                        p.trgTTL.inarow );
        }
        else {
            found = qA->findBitFallingEdge(
                        aFallCtNext,
                        aFallCtNext,
                        digChan,
                        p.trgTTL.bit,
                        p.trgTTL.inarow );
        }

        if( found )
            aEdgeCtNext = aFallCtNext;
    }

    quint64 bOutCt;

    if( found && qB ) {

        double  wallT;

        qA->mapCt2Time( wallT, aEdgeCtNext );
        found = qB->mapTime2Ct( bOutCt, wallT );
    }

    if( found ) {

        alignX12( qA, aEdgeCtNext, bOutCt );

        aFallCt     = aEdgeCtNext;
        aEdgeCtNext = 0;

        if( qB )
            bFallCt = bOutCt;
    }

    return found;
}


bool TrigTTL::getRiseEdge()
{
    quint64 imNextCt;

    if( p.trgTTL.stream == "nidq" ) {

        const AIQ   *aiQ = (nImQ ? imQ[0] : 0);

        if( !_getRiseEdge( niCnt.nextCt, niQ, imNextCt, aiQ ) )
            return false;
    }
    else {

        int ip = p.streamID( p.trgTTL.stream );

        imNextCt = imCnt.nextCt[ip];

        if( !_getRiseEdge( imNextCt, imQ[ip], niCnt.nextCt, niQ ) )
            return false;
    }

    imCnt.nextCt.fill( imNextCt, nImQ );

    imCnt.edgeCt = imCnt.nextCt;
    niCnt.edgeCt = niCnt.nextCt;

    return true;
}


void TrigTTL::getFallEdge()
{
    quint64 imFallCt;

    if( p.trgTTL.stream == "nidq" ) {

        const AIQ   *aiQ = (nImQ ? imQ[0] : 0);

        if( _getFallEdge( niCnt.fallCt, niQ, imFallCt, aiQ ) ) {

            niCnt.remCt = niCnt.fallCt - niCnt.nextCt;

            imCnt.fallCt.fill( imFallCt, nImQ );

            for( int ip = 0; ip < nImQ; ++ip )
                imCnt.remCt[ip] = imFallCt - imCnt.nextCt[ip];
        }
        else {
            // If we didn't yet find the falling edge we have to
            // limit the next read so we don't go too far...
            // For A, don't go farther than we've looked.
            // For B, use the current B count.
            //
            // Note for main driver loop:
            // Since we're setting a finite remCt, exhausting
            // that in a read doesn't mean we're done. Rather,
            // for the follower mode we need to test if the
            // falling edge was found.

            niCnt.remCt = aFallCtNext - niCnt.nextCt;

            imCnt.remCt.resize( nImQ );

            for( int ip = 0; ip < nImQ; ++ip )
                imCnt.remCt[ip] = imQ[ip]->curCount() - imCnt.nextCt[ip];
        }
    }
    else {

        int ip = p.streamID( p.trgTTL.stream );

        imFallCt = imCnt.nextCt[ip];

        if( _getFallEdge( imFallCt, imQ[ip], niCnt.fallCt, niQ ) ) {

            imCnt.fallCt.fill( imFallCt, nImQ );

            for( int ip = 0; ip < nImQ; ++ip )
                imCnt.remCt[ip] = imFallCt - imCnt.nextCt[ip];

            if( niQ )
                niCnt.remCt = niCnt.fallCt - niCnt.nextCt;
        }
        else {
            // If we didn't yet find the falling edge we have to
            // limit the next read so we don't go too far...
            // For A, don't go farther than we've looked.
            // For B, use the current B count.
            //
            // Note for main driver loop:
            // Since we're setting a finite remCt, exhausting
            // that in a read doesn't mean we're done. Rather,
            // for the follower mode we need to test if the
            // falling edge was found.

            imCnt.remCt.resize( nImQ );

            for( int ip = 0; ip < nImQ; ++ip )
                imCnt.remCt[ip] = aFallCtNext - imCnt.nextCt[ip];

            if( niQ )
                niCnt.remCt = niQ->curCount() - niCnt.nextCt;
            else
                niCnt.remCt = 0;
        }
    }
}


// Write margin up to but not including rising edge.
//
// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrigTTL::writePreMarginNi()
{
    CountsNi    &C = niCnt;

    if( !niQ || C.remCt <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = niQ->getNScansFromCt(
            vB,
            C.edgeCt - C.remCt,
            (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );

    if( !nb )
        return true;

// Status in this state should be what's done: +(margin - rem).
// If next = edge - rem, then status = +(next - edge + margin).
//
// When rem falls to zero, (next = edge) sets us up for state H.

    C.remCt -= niQ->sumCt( vB );
    C.nextCt = C.edgeCt - C.remCt;

    return writeAndInvalVB( DstNidq, 0, vB );
}


// Write margin, including falling edge.
//
// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrigTTL::writePostMarginNi()
{
    CountsNi    &C = niCnt;

    if( !niQ || C.remCt <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = niQ->getNScansFromCt(
            vB,
            C.fallCt + C.marginCt - C.remCt,
            (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );

    if( !nb )
        return true;

// Status in this state should be: +(margin + H + margin - rem).
// With next defined as below, status = +(next - edge + margin)
// = margin + (fall-edge) + margin - rem = correct.

    C.remCt -= niQ->sumCt( vB );
    C.nextCt = C.fallCt + C.marginCt - C.remCt;

    return writeAndInvalVB( DstNidq, 0, vB );
}


// Write from rising edge up to but not including falling edge.
//
// Return true if no errors.
//
bool TrigTTL::doSomeHNi()
{
    CountsNi    &C = niCnt;

    if( !niQ || (p.trgTTL.mode != DAQ::TrgTTLFollowAI && !C.remCt) )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// ---------------
// Fetch a la mode
// ---------------

    if( p.trgTTL.mode == DAQ::TrgTTLLatch ) {

        // Latched case
        // Get all since last fetch

        nb = niQ->getAllScansFromCt( vB, C.nextCt );
    }
    else if( p.trgTTL.mode == DAQ::TrgTTLTimed ) {

        // Timed case
        // In-progress H fetch using counts

        C.fallCt  = C.edgeCt + C.hiCtMax;
        C.remCt   = C.hiCtMax - (C.nextCt - C.edgeCt);

        nb = niQ->getNScansFromCt(
                vB,
                C.nextCt,
                (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );
    }
    else {

        // Follower case
        // Fetch up to falling edge

        if( !C.fallCt )
            getFallEdge();
        else
            C.remCt = C.fallCt - C.nextCt;

        nb = niQ->getNScansFromCt(
                vB,
                C.nextCt,
                (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );
    }

// ------------------------
// Write/update all H cases
// ------------------------

    if( !nb )
        return true;

    C.nextCt = niQ->nextCt( vB );
    C.remCt -= C.nextCt - vB[0].headCt;

    return writeAndInvalVB( DstNidq, 0, vB );
}


// Set preMidPost to {-1,0,1} to select {premargin, H, postmargin}.
//
// Return true if no errors.
//
bool TrigTTL::xferAll( TrTTLShared &shr, int preMidPost )
{
    int niOK;

    shr.preMidPost  = preMidPost;
    shr.awake       = 0;
    shr.asleep      = 0;
    shr.errors      = 0;

// Wake all imec threads

    shr.condWake.wakeAll();

// Do nidq locally

    if( preMidPost == -1 )
        niOK = writePreMarginNi();
    else if( !preMidPost )
        niOK = doSomeHNi();
    else
        niOK = writePostMarginNi();

// Wait all threads started, and all done

    shr.runMtx.lock();
        while( shr.awake  < nThd
            || shr.asleep < nThd ) {

            shr.runMtx.unlock();
                msleep( LOOP_MS/8 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

    return niOK && !shr.errors;
}


void TrigTTL::statusProcess( QString &sT, bool inactive )
{
    if( inactive || !niCnt.nextCt )
        sT = " TX";
    else if( ISSTATE_L ) {

        double  dt;

        if( niQ )
            dt = (niCnt.nextCt - niCnt.edgeCt)/p.ni.srate;
        else
            dt = (imCnt.nextCt[0] - imCnt.edgeCt[0])/p.im.all.srate;

        sT = QString(" T-%1s").arg( dt, 0, 'f', 1 );
    }
    else {

        double  dt;

        if( niQ ) {
            dt = (niCnt.nextCt - niCnt.edgeCt + niCnt.marginCt)
                    / p.ni.srate;
        }
        else {
            dt = (imCnt.nextCt[0] - imCnt.edgeCt[0] + imCnt.marginCt)
                    / p.im.all.srate;
        }

        sT = QString(" T+%1s").arg( dt, 0, 'f', 1 );
    }
}


