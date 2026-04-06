
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"

#include <QDir>
#include <QThread>


#define STOPCHECK   if( isStopped() ) return false;
#define TSTOPCHECK  if( acq->isStopped() ) break;


/* ---------------------------------------------------------------- */
/* ImCfgWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ImCfgWorker::run()
{
    for( int ip = 0, np = (int)vip.size(); ip < np; ++ip ) {

        const CimCfg::ImProbeDat    &P = T.get_iProbe( vip[ip] );

        if( acq->ip2simdat[P.ip] >= 0 ) {

            if( !_mt_simProbe( P ) )
                break;

            continue;
        }

        if( !_mt_openProbe( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_calibrateADC( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_calibrateGain( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_calibrateOpto( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_setLEDs( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_selectElectrodes( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_selectReferences( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_selectGains( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_selectAPFilters( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_setStandby( P ) )
            break;

        acq->STEPPROBE();
        TSTOPCHECK;

        if( !_mt_writeProbe( P ) )
            break;

        acq->STEPPROBE();
    }

    emit finished();
}


bool ImCfgWorker::_mt_simProbe( const CimCfg::ImProbeDat &P )
{
    QString err;

    if( !acq->simDat[acq->ip2simdat[P.ip]].init(
            err, T.simprb.file( P.adr ) ) ) {

        shr.seterror(
            QString("Probe file(%1) %2")
            .arg( P.adr.tx_spd() ).arg( err ) );
        return false;
    }

    for( int i = 0; i < 11; ++i )
        acq->STEPPROBE();

    return true;
}


bool ImCfgWorker::_mt_openProbe( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = np_openProbe( P.adr.slot, P.adr.port, P.adr.dock );

    if( err != SUCCESS ) {
        shr.seterror(
            QString("IMEC openProbe(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    for( int itry = 1; itry <= 10; ++itry ) {

        err = np_init( P.adr.slot, P.adr.port, P.adr.dock );

        if( err == SUCCESS ) {
            if( itry > 1 ) {
                Warning() <<
                QString("Probe %1: init() took %2 tries.")
                .arg( P.ip ).arg( itry );
            }
            break;
        }
        else if( err == ERROR_SR_CHAIN ) {
            if( acq->p.im.prbAll.srAtDetect && !P.srDoCheck() )
                return true;
        }

        QThread::msleep( 100 );
    }

    if( err != SUCCESS ) {
        shr.seterror(
            QString("IMEC init(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_calibrateADC( const CimCfg::ImProbeDat &P )
{
    if( !acq->p.im.prbj[P.ip].roTbl->needADCCal() )
        return true;

    if( acq->p.im.prbAll.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 ADC calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( acq->p.im.prbAll.calPolicy == 1 )
            goto warn;
        else {
            shr.seterror(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.calSN() ).arg( P.ip ) );
            return false;
        }
    }

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        shr.seterror( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_ADCCalibration.csv").arg( path ).arg( P.calSN() );

    if( !QFile( path ).exists() ) {
        shr.seterror( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = np_setADCCalibration( P.adr.slot, P.adr.port, STR2CHR( path ) );

    if( err != SUCCESS ) {
        shr.seterror(
            QString(
            "IMEC setADCCalibration(%1)%2")
            .arg( P.adr.tx_sp() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_calibrateGain( const CimCfg::ImProbeDat &P )
{
    if( !acq->p.im.prbj[P.ip].roTbl->needGainCal() )
        return true;

    if( acq->p.im.prbAll.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 gain calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( acq->p.im.prbAll.calPolicy == 1 )
            goto warn;
        else {
            shr.seterror(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.calSN() ).arg( P.ip ) );
            return false;
        }
    }

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        shr.seterror( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_gainCalValues.csv").arg( path ).arg( P.calSN() );

    if( !QFile( path ).exists() ) {
        shr.seterror( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = np_setGainCalibration(
            P.adr.slot, P.adr.port, P.adr.dock, STR2CHR( path ) );

    if( err != SUCCESS ) {
        shr.seterror(
            QString(
            "IMEC setGainCalibration(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_calibrateOpto( const CimCfg::ImProbeDat &P )
{
    if( acq->p.im.prbj[P.ip].roTbl->probeTech() != t_tech_opto )
        return true;

    if( acq->p.im.prbAll.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 optical calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( acq->p.im.prbAll.calPolicy == 1 )
            goto warn;
        else {
            shr.seterror(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.calSN() ).arg( P.ip ) );
            return false;
        }
    }

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        shr.seterror( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_optoCalibration.csv").arg( path ).arg( P.calSN() );

    if( !QFile( path ).exists() ) {
        shr.seterror( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = np_setOpticalCalibration(
            P.adr.slot, P.adr.port, P.adr.dock, STR2CHR( path ) );

    if( err != SUCCESS ) {
        shr.seterror(
            QString(
            "IMEC setOpticalCalibration(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_setLEDs( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err;

    err = np_setHSLed( P.adr.slot, P.adr.port, acq->p.im.prbj[P.ip].LEDEnable );

    if( err != SUCCESS ) {
        shr.seterror(
            QString("IMEC setHSLed(%1)%2")
            .arg( P.adr.tx_sp() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_selectElectrodes( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(acq->p.im.prbj[P.ip].roTbl->
                            selectSites4( P.adr, false, true ));

    if( err != SUCCESS ) {
        shr.seterror(
            QString("IMEC selectSites4(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_selectReferences( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(acq->p.im.prbj[P.ip].roTbl->
                            selectRefs4( P.adr ));

    if( err != SUCCESS ) {
        shr.seterror(
            QString(
            "IMEC selectRefs4(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_selectGains( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(acq->p.im.prbj[P.ip].roTbl->
                            selectGains4( P.adr ));

    if( err != SUCCESS ) {
        shr.seterror(
            QString("IMEC selectGains4(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_selectAPFilters( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(acq->p.im.prbj[P.ip].roTbl->
                            selectAPFlts4( P.adr ));

    if( err != SUCCESS ) {
        shr.seterror(
            QString(
            "IMEC selectAPFlts4(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool ImCfgWorker::_mt_setStandby( const CimCfg::ImProbeDat &P )
{
// --------------------------------------------------
// Turn ALL channels on or off according to stdbyBits
// --------------------------------------------------

    int nC = acq->p.im.prbj[P.ip].roTbl->nAP();

    for( int ic = 0; ic < nC; ++ic ) {

        NP_ErrorCode    err;

        err = np_setStdb( P.adr.slot, P.adr.port, P.adr.dock, ic,
                acq->p.im.prbj[P.ip].stdbyBits.testBit( ic ) );

        if( err != SUCCESS ) {
            shr.seterror(
                QString("IMEC setStdb(%1)%2")
                .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}


bool ImCfgWorker::_mt_writeProbe( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err;
    bool            check = !acq->p.im.prbAll.srAtDetect || P.srDoCheck();

    for( int itry = 1; itry <= 10; ++itry ) {

        err = np_writeProbeConfiguration(
                P.adr.slot, P.adr.port, P.adr.dock, check );

        if( err == SUCCESS ) {
            if( itry > 1 ) {
                Warning() <<
                QString("Probe %1: writeConfig() took %2 tries.")
                .arg( P.ip ).arg( itry );
            }
            break;
        }

        QThread::msleep( 100 );
    }

    if( err != SUCCESS ) {
        shr.seterror(
            QString(
            "IMEC writeProbeConfiguration(%1)%2")
            .arg( P.adr.tx_spd() ).arg( acq->makeErrorString( err ) ) );
        return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* ImCfgThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImCfgThread::ImCfgThread(
    CimAcqImec                  *acq,
    const CimCfg::ImProbeTable  &T,
    const std::vector<int>      &vip,
    ImCfgShared                 &shr )
{
    thread  = new QThread;
    worker  = new ImCfgWorker( acq, T, vip, shr );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


ImCfgThread::~ImCfgThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* _mt_configProbes ----------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::_mt_configProbes( const CimCfg::ImProbeTable &T )
{
    int np = p.stream_nIM();

// Alloc and map simDat before threading

    ip2simdat.assign( np, -1 );

    for( int ip = 0; ip < np; ++ip ) {
        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
        if( T.simprb.isSimProbe( P.adr ) ) {
            int isim = (int)simDat.size();
            simDat.resize( isim + 1 );
            ip2simdat[ip] = isim;
        }
    }

// Create threads

    for( int ip = 0; ip < np; ) {

        STOPCHECK;

        std::vector<int>    vip;
        vip.push_back( ip );

        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip++ );

//@OBX Rules preventing concurrent config of OneBox ports
//@OBX and both docks of 2.0 HS. Workaround until imec fixes.

        if( T.isSlotUSBType( P.adr.slot ) ) {
#if 0
// All probes on a GIVEN OneBox (slot) in same thread
            int pslot = P.adr.slot;

            while( ip < np ) {

                const CimCfg::ImProbeDat    &Q = T.get_iProbe( ip );

                if( Q.slot == pslot )
                    vip.push_back( ip++ );
            }
#else
// All probes on ANY OneBox (rest of table) in same thread
            while( ip < np )
                vip.push_back( ip++ );
#endif
        }
        else if( P.adr.dock == 1 && ip < np ) {

            // Both 2.0 docks in same thread

            PAddr   A = P.adr;  // prev with dock == 1
            A.dock = 2;         // putative next

            if( T.get_iProbe( ip ).adr == A )
                vip.push_back( ip++ );
        }

        cfgThd.push_back( new ImCfgThread( this, T, vip, cfgShr ) );
    }

    for( int nThd = (int)cfgThd.size(), iThd = nThd - 1; iThd >= 0; --iThd ) {
        ImCfgThread *T = cfgThd[iThd];
        if( T ) {
            if( T->thread->isRunning() )
                T->thread->wait( 10000/nThd );
            delete T;
            cfgThd.erase( cfgThd.begin() + iThd );
        }
    }

    int nerr = cfgShr.error.size();

    for( int ie = 0; ie < nerr; ++ie )
        Error() << cfgShr.error[ie];

    if( nerr ) {
        emit owner->daqError(
        "Error configuring probe(s); see Log or Metrics window." );
    }

    return !nerr;
}

#endif  // HAVE_IMEC


