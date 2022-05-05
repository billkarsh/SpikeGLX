
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"

#include <QDir>


#define STOPCHECK   if( isStopped() ) return false;


bool CimAcqImec::_1t_openProbe( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = np_openProbe( P.slot, P.port, P.dock );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC openProbe(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_init( P.slot, P.port, P.dock );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC init(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

#if 0
// 09/05/19 Advised by Jan as required for NP 2.0.
// 06/17/20 Advised by Jan as no longer needed for NP 3.0.

    err = np_writeI2Cflex( P.slot, P.port, P.dock, 0xE0, 0x03, 0x08 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC writeI2Cflex(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }
#endif

    return true;
}


bool CimAcqImec::_1t_calibrateADC( const CimCfg::ImProbeDat &P )
{
    if( !p.im.prbj[P.ip].roTbl->needADCCal() )
        return true;

    if( p.im.prbAll.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 ADC calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( p.im.prbAll.calPolicy == 1 )
            goto warn;
        else {
            runError(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.sn ).arg( P.ip ) );
            return false;
        }
    }

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_ADCCalibration.csv").arg( path ).arg( P.sn );

    if( !QFile( path ).exists() ) {
        runError( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = np_setADCCalibration( P.slot, P.port, STR2CHR( path ) );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC setADCCalibration(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_calibrateGain( const CimCfg::ImProbeDat &P )
{
    if( P.type == 1200 )
        return true;

    if( p.im.prbAll.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 gain calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( p.im.prbAll.calPolicy == 1 )
            goto warn;
        else {
            runError(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.sn ).arg( P.ip ) );
            return false;
        }
    }

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_gainCalValues.csv").arg( path ).arg( P.sn );

    if( !QFile( path ).exists() ) {
        runError( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = np_setGainCalibration( P.slot, P.port, P.dock, STR2CHR( path ) );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC setGainCalibration(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_calibrateOpto( const CimCfg::ImProbeDat &P )
{
    if( P.type != 1300 )
        return true;

    if( p.im.prbAll.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 optical calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( p.im.prbAll.calPolicy == 1 )
            goto warn;
        else {
            runError(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.sn ).arg( P.ip ) );
            return false;
        }
    }

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_optoCalibration.csv").arg( path ).arg( P.sn );

    if( !QFile( path ).exists() ) {
        runError( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = np_setOpticalCalibration( P.slot, P.port, P.dock, STR2CHR( path ) );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC setOpticalCalibration(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


#if 0   // selectDataSource now private in NeuropixAPI.h
// Synthetic data generation for testing:
// 0 = normal
// 1 = each chan is ADC id num
// 2 = each chan is chan id num
// 3 = each chan linear ramping
//
bool CimAcqImec::_1t_dataGenerator( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = IM.selectDataSource( P.slot, P.port, 3 );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC selectDataSource(slot %1, port %2)%3")
            .arg( P.slot ).arg( P.port )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    Log() << QString("IMEC probe %1 generating synthetic data").arg( P.ip );
    return true;
}
#endif


bool CimAcqImec::_1t_setLEDs( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err;

    err = np_setHSLed( P.slot, P.port, p.im.prbj[P.ip].LEDEnable );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setHSLed(slot %1, port %2)%3")
            .arg( P.slot ).arg( P.port )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_selectElectrodes( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(p.im.prbj[P.ip].roTbl->
                            selectSites( P.slot, P.port, P.dock, false ));

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC selectSites(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_selectReferences( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(p.im.prbj[P.ip].roTbl->
                            selectRefs( P.slot, P.port, P.dock ));

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC selectRefs(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_selectGains( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(p.im.prbj[P.ip].roTbl->
                            selectGains( P.slot, P.port, P.dock ));

    if( err != SUCCESS ) {
        runError(
            QString("IMEC selectGains(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_selectAPFilters( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = NP_ErrorCode(p.im.prbj[P.ip].roTbl->
                            selectAPFlts( P.slot, P.port, P.dock ));

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC selectAPFlts(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_setStandby( const CimCfg::ImProbeDat &P )
{
// --------------------------------------------------
// Turn ALL channels on or off according to stdbyBits
// --------------------------------------------------

    int nC = p.im.prbj[P.ip].roTbl->nChan();

    for( int ic = 0; ic < nC; ++ic ) {

        NP_ErrorCode    err;

        err = np_setStdb( P.slot, P.port, P.dock, ic,
                p.im.prbj[P.ip].stdbyBits.testBit( ic ) );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setStdb(slot %1, port %2, dock %3)%4")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}


bool CimAcqImec::_1t_writeProbe( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err;

    err = np_writeProbeConfiguration( P.slot, P.port, P.dock, true );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC writeProbeConfiguration(slot %1, port %2, dock %3)%4")
            .arg( P.slot ).arg( P.port ).arg( P.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_1t_configProbes( const CimCfg::ImProbeTable &T )
{
    for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip ) {

        STOPCHECK;

        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );

        if( !_1t_openProbe( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_calibrateADC( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_calibrateGain( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_calibrateOpto( P ) )
            return false;

//        STEPPROBE( P.ip );
//        STOPCHECK;

//        if( !_dataGenerator( P ) )
//            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_setLEDs( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_selectElectrodes( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_selectReferences( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_selectGains( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_selectAPFilters( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_setStandby( P ) )
            return false;

        STEPPROBE( P.ip );
        STOPCHECK;

        if( !_1t_writeProbe( P ) )
            return false;

        STEPPROBE( P.ip );
    }

    return true;
}

#endif  // HAVE_IMEC


