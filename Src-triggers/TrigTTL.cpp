
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
// Return true if no errors.
//
bool TrTTLWorker::writePreMarginIm( int ip )
{
    TrigTTL::CountsIm   &C = ME->imCnt;

    if( C.remCt[ip] <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !imQ[ip]->getNScansFromCt(
            vB,
            C.nextCt[ip],
            (C.remCt[ip] <= C.maxFetch ? C.remCt[ip] : C.maxFetch) ) ) {

        return false;
    }

    if( !vB.size() )
        return true;

// Status in this state should be what's done: +(margin - rem).
// If next = edge - rem, then status = +(margin + next - edge).
//
// When rem falls to zero, (next = edge) sets us up for state H.

    C.remCt[ip] -= imQ[ip]->sumCt( vB );
    C.nextCt[ip] = C.edgeCt[ip] - C.remCt[ip];

    return ME->writeAndInvalVB( ME->DstImec, ip, vB );
}


// Write margin, including falling edge.
//
// Return true if no errors.
//
bool TrTTLWorker::writePostMarginIm( int ip )
{
    TrigTTL::CountsIm   &C = ME->imCnt;

    if( C.remCt[ip] <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !imQ[ip]->getNScansFromCt(
            vB,
            C.nextCt[ip],
            (C.remCt[ip] <= C.maxFetch ? C.remCt[ip] : C.maxFetch) ) ) {

        return false;
    }

    if( !vB.size() )
        return true;

// Status in this state should be: +(margin + H + margin - rem).
// With next defined as below, status = +(margin + next - edge)
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

    std::vector<AIQ::AIQBlock>  vB;
    bool                        ok;

// ---------------
// Fetch a la mode
// ---------------

    if( shr.p.trgTTL.mode == DAQ::TrgTTLLatch )
        ok = imQ[ip]->getAllScansFromCt( vB, C.nextCt[ip] );
    else if( C.remCt[ip] <= 0 )
        return true;
    else {

        ok = imQ[ip]->getNScansFromCt(
                vB,
                C.nextCt[ip],
                (C.remCt[ip] <= C.maxFetch ? C.remCt[ip] : C.maxFetch) );
    }

    if( !ok )
        return false;

// ------------------------
// Write/update all H cases
// ------------------------

    if( !vB.size() )
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
        highsMax(
            p.trgTTL.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgTTL.nH),
        aEdgeCtNext(0),
        thresh(
            p.trgTTL.stream == "nidq" ?
            p.ni.vToInt16( p.trgTTL.T, p.trgTTL.chan )
            : p.im.vToInt10( p.trgTTL.T, p.streamID( p.trgTTL.stream ),
                p.trgTTL.chan )),
// MS: Analog and digital aux may be redefined in phase 3B2
        digChan(
            p.trgTTL.isAnalog ? -1 :
            (p.trgTTL.stream == "nidq" ?
             p.ni.niCumTypCnt[CniCfg::niSumAnalog] + p.trgTTL.bit/16
             : p.im.each[p.streamID( p.trgTTL.stream )]
                .imCumTypCnt[CimCfg::imSumNeural]))
{
    vEdge.resize( vS.size() );
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


#define TINYMARG            0.0001

#define ISSTATE_L           (state == 0)
#define ISSTATE_PreMarg     (state == 1)
#define ISSTATE_H           (state == 2)
#define ISSTATE_PostMarg    (state == 3)
#define ISSTATE_Done        (state == 4)

#define ZEROREM ((!niQ || niCnt.remCt <= 0) && imCnt.remCtDone())


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
                    err = "Generic error";
                    break;
                }
            }
        }

        // ----------------
        // Write pre-margin
        // ----------------

        if( ISSTATE_PreMarg ) {

            if( !xferAll( shr, -1 ) ) {
                err = "Generic error";
                break;
            }

            if( ZEROREM )
                SETSTATE_H();
        }

        // -------
        // Write H
        // -------

        if( ISSTATE_H ) {

            // Set falling edge

            if( p.trgTTL.mode == DAQ::TrgTTLFollowAI ) {

                if( p.trgTTL.stream == "nidq" ) {

                    if( !niCnt.fallCt )
                        getFallEdge();
                }
                else {

                    if( !imCnt.fallCt[imCnt.iTrk] )
                        getFallEdge();
                }
            }

            // Write

            if( !xferAll( shr, 0 ) ) {
                err = "Generic error";
                break;
            }

            // Done?

            if( p.trgTTL.mode == DAQ::TrgTTLLatch )
                goto next_loop;

            if( p.trgTTL.mode == DAQ::TrgTTLFollowAI ) {

                // In this mode, remCt isn't set until fallCt is set.

                if( p.trgTTL.stream == "nidq" ) {

                    if( !niCnt.fallCt )
                        goto next_loop;
                }
                else {

                    if( !imCnt.allFallCtSet() )
                        goto next_loop;
                }
            }

            // Check for zero remainder

            if( ZEROREM ) {

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

            if( !xferAll( shr, 1 ) ) {
                err = "Generic error";
                break;
            }

            // Done?

            if( ZEROREM ) {

check_done:
                if( ++nHighs >= highsMax ) {
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

                    // Strictly for L status message
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

// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd )
        delete trT[iThd];

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
    imCnt.setPreMarg();
    niCnt.setPreMarg();

    state = 1;
}


// Set from, rem and fall (if applicable) for H phase.
//
void TrigTTL::SETSTATE_H()
{
    imCnt.setH( DAQ::TrgTTLMode(p.trgTTL.mode) );
    niCnt.setH( DAQ::TrgTTLMode(p.trgTTL.mode) );

    state = 2;
}


// Set from and rem for postmargin;
// not applicable in latched mode.
//
void TrigTTL::SETSTATE_PostMarg()
{
    if( p.trgTTL.mode != DAQ::TrgTTLLatch ) {

        imCnt.setPostMarg();
        niCnt.setPostMarg();
    }

    state = 3;
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
    nHighs          = 0;
    SETSTATE_L();
}


// Find rising edge in source stream;
// translate to all streams 'vEdge'.
//
bool TrigTTL::_getRiseEdge( quint64 &srcNextCt, int iSrc )
{
// First edge is the gate edge for (state L) status report
// wherein edgeCt is just time we started seeking an edge.

    if( !srcNextCt ) {
        if( 0 != vS[iSrc].Q->mapTime2Ct( srcNextCt, getGateHiT() ) )
            return false;
    }

// It may take several tries to achieve pulser sync for multi streams.
// aEdgeCtNext saves us from costly refinding of edge-A while hunting.

    int     ns;
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

    if( found && (ns = vS.size()) > 1 ) {

        syncDstTAbsMult( aEdgeCtNext, iSrc, vS, p );

        for( int is = 0; is < ns; ++is ) {

            if( is != iSrc ) {

                const SyncStream    &S = vS[is];

                if( p.sync.sourceIdx != DAQ::eSyncSourceNone && !S.bySync )
                    return false;

                vEdge[is] = S.TAbs2Ct( S.tAbs );
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
bool TrigTTL::_getFallEdge( quint64 srcEdgeCt, int iSrc )
{
    int     ns;
    bool    found;

    if( !aFallCtNext )
        aFallCtNext = srcEdgeCt;

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

    if( (ns = vS.size()) > 1 ) {

        syncDstTAbsMult( aFallCtNext, iSrc, vS, p );

        for( int is = 0; is < ns; ++is ) {

            if( is != iSrc ) {
                const SyncStream    &S = vS[is];
                vEdge[is] = S.TAbs2Ct( S.tAbs );
            }
        }
    }

    return found;
}


// Find source edge; copy from vEdge to Count records.
//
bool TrigTTL::getRiseEdge()
{
    if( p.trgTTL.stream == "nidq" ) {

        if( !_getRiseEdge( niCnt.nextCt, 0 ) )
            return false;

        niCnt.edgeCt = vEdge[0];

        for( int ip = 0; ip < nImQ; ++ip )
            imCnt.edgeCt[ip] = vEdge[1+ip];
    }
    else {

        int ip      = imCnt.iTrk,
            offset  = (niQ ? 1 : 0);

        if( !_getRiseEdge( imCnt.nextCt[ip], offset+ip  ) )
            return false;

        if( niQ )
            niCnt.edgeCt = vEdge[0];

        for( int ip = 0; ip < nImQ; ++ip )
            imCnt.edgeCt[ip] = vEdge[offset+ip];
    }

    return true;
}


// Set fallCt(s) if edge found, set remCt(s) whether found or not.
//
void TrigTTL::getFallEdge()
{
    int     ip      = imCnt.iTrk,
            offset  = (niQ ? 1: 0);
    bool    found;

    if( p.trgTTL.stream == "nidq" )
        found = _getFallEdge( niCnt.edgeCt, 0 );
    else
        found = _getFallEdge( imCnt.edgeCt[ip], offset+ip );

    if( found ) {

        if( niQ ) {
            niCnt.fallCt = vEdge[0];
            niCnt.remCt  = niCnt.fallCt - niCnt.nextCt;
        }

        for( int ip = 0; ip < nImQ; ++ip ) {
            imCnt.fallCt[ip] = vEdge[offset+ip];
            imCnt.remCt[ip]  = imCnt.fallCt[ip] - imCnt.nextCt[ip];
        }
    }
    else {

        if( niQ )
            niCnt.remCt = vEdge[0] - niCnt.nextCt;

        for( int ip = 0; ip < nImQ; ++ip )
            imCnt.remCt[ip] = vEdge[offset+ip] - imCnt.nextCt[ip];
    }
}


// Write margin up to but not including rising edge.
//
// Return true if no errors.
//
bool TrigTTL::writePreMarginNi()
{
    CountsNi    &C = niCnt;

    if( !niQ || C.remCt <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !niQ->getNScansFromCt(
            vB,
            C.nextCt,
            (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) ) ) {

        return false;
    }

    if( !vB.size() )
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
// Return true if no errors.
//
bool TrigTTL::writePostMarginNi()
{
    CountsNi    &C = niCnt;

    if( !niQ || C.remCt <= 0 )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !niQ->getNScansFromCt(
            vB,
            C.nextCt,
            (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) ) ) {

        return false;
    }

    if( !vB.size() )
        return true;

// Status in this state should be: +(margin + H + margin - rem).
// With next defined as below, status = +(margin + next - edge)
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

    if( !niQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    bool                        ok;

// ---------------
// Fetch a la mode
// ---------------

    if( p.trgTTL.mode == DAQ::TrgTTLLatch )
        ok = niQ->getAllScansFromCt( vB, C.nextCt );
    else if( C.remCt <= 0 )
        return true;
    else {

        ok = niQ->getNScansFromCt(
                vB,
                C.nextCt,
                (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );
    }

    if( !ok )
        return false;

// ------------------------
// Write/update all H cases
// ------------------------

    if( !vB.size() )
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
    bool    niOK;

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
    bool trackNI = p.trgTTL.stream == "nidq";

    if( inactive
        || (trackNI  && !niCnt.nextCt)
        || (!trackNI && !imCnt.isTracking()) ) {

        sT = " TX";
    }
    else if( ISSTATE_L ) {

        double  dt = (trackNI ? niCnt.L_progress() : imCnt.L_progress());

        sT = QString(" T-%1s").arg( dt, 0, 'f', 1 );
    }
    else {

        double  dt = (trackNI ? niCnt.H_progress() : imCnt.H_progress());

        sT = QString(" T+%1s").arg( dt, 0, 'f', 1 );
    }
}


