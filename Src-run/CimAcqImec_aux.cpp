
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"


#define STOPCHECK   if( isStopped() ) return false;


// @@@ FIX Leave buffers at defaults until understand better.
// @@@ FIX Need to scale buf size with probe count.
// NP_PARAM_BUFFERSIZE:     default 128K
// NP_PARAM_BUFFERCOUNT:    default 64
//
bool CimAcqImec::_aux_sizeStreamBufs()
{
#if 0
    NP_ErrorCode    err;

    err = np_setParameter( NP_PARAM_BUFFERSIZE, 128*1024 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(BUFSIZE)%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }
#endif

#if 0
    err = np_setParameter( NP_PARAM_BUFFERCOUNT, 64 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(BUFCOUNT)%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }
#endif

    return true;
}


// Standard settings for Obx:
// - ADC_enableProbe( true ) enable sync/bit-6 and/or ADC.
// - DAC_enableOutput( false ) all channels input by default.
// - ADC_setVoltageRange( config or 5 ).
// - ADC_setComparatorThreshold( rmax/10, rmax/10 ) all channels.
//
bool CimAcqImec::_aux_initObxSlot( const CimCfg::ImProbeTable &T, int slot )
{
    double          v       = 5.0;
    int             ob_ip   = -1;
    ADCrange_t      rng     = ADC_RANGE_5V;
    NP_ErrorCode    err;

// -----------------------------
// Find OneBox ip with this slot
// -----------------------------

    for( int ip = 0, np = p.stream_nOB(); ip < np; ++ip ) {

        if( T.get_iOneBox( ip ).slot == slot ) {
            ob_ip = ip;
            break;
        }
    }

// -------
// Program
// -------

    err = np_ADC_enableProbe( slot, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC ADC_enableProbe(slot %1)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    for( int ic = 0; ic < imOBX_NCHN; ++ic ) {

        err = np_DAC_enableOutput( slot, ic, false );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC DAC_enableOutput(slot %1, chn %2, false)%3")
                .arg( slot ).arg( ic ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    if( ob_ip >= 0 ) {

        v = p.im.obxj[ob_ip].range.rmax;

        if( v < 4.0 )
            rng = ADC_RANGE_2_5V;
        else if( v > 6.0 )
            rng = ADC_RANGE_10V;
    }

    err = np_ADC_setVoltageRange( slot, rng );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC ADC_setVoltageRange(slot %1 %2V)%3")
            .arg( slot ).arg( v ).arg( makeErrorString( err ) ) );
        return false;
    }

    for( int ic = 0; ic <= 11; ++ic ) {

        err = np_ADC_setComparatorThreshold( slot, ic, 0.1 * v, 0.1 * v );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC ADC_setComparatorThreshold(slot %1 chn %2 lo 0.5 hi 1.8)%3")
                .arg( slot ).arg( ic ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}


bool CimAcqImec::_aux_open( const CimCfg::ImProbeTable &T )
{
    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        int slot = T.getEnumSlot( is );

        if( T.simprb.isSimSlot( slot ) )
            continue;

        NP_ErrorCode    err = np_openBS( slot );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC openBS(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }

        if( T.isSlotUSBType( slot ) ) {
            if( !_aux_initObxSlot( T, slot ) )
                return false;
        }
    }

    return true;
}


bool CimAcqImec::_aux_setObxSyncAsOutput( int slot )
{
    NP_ErrorCode    err;

//@OBX Can't allow selecting obx as sync source if also running PXI!!
    err = np_setParameter( NP_PARAM_SYNCMASTER, slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCMASTER)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_setParameter( NP_PARAM_SYNCSOURCE, SyncSource_Clock );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCSOURCE)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_setParameter( NP_PARAM_SYNCPERIOD_MS, 1000 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCPERIOD)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_SMA1, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OUTOBXSMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

//@OBX Not needed if use syncmaster calls to set clock
//    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_SyncClk, true );

//    if( err != SUCCESS ) {
//        runError(
//            QString("IMEC switchmatrix_set(slot %1, OUTOBXSTATSYNC)%2")
//            .arg( slot ).arg( makeErrorString( err ) ) );
//        return false;
//    }

    return true;
}


bool CimAcqImec::_aux_setObxSyncAsInput( int slot )
{
    NP_ErrorCode    err;

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_SMA1, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, INOBXSTATSYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setPXISyncAsOutput( int slot )
{
    NP_ErrorCode    err;

    err = np_setParameter( NP_PARAM_SYNCMASTER, slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCMASTER)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_setParameter( NP_PARAM_SYNCSOURCE, SyncSource_Clock );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCSOURCE)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_setParameter( NP_PARAM_SYNCPERIOD_MS, 1000 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCPERIOD)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

//@OBX Don't understand why this needed
    err = np_switchmatrix_set( slot, SM_Output_SMA, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OUTSMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setPXISyncAsInput( int slot )
{
    NP_ErrorCode    err;

    err = np_setParameter( NP_PARAM_SYNCMASTER, slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCMASTER)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_setParameter( NP_PARAM_SYNCSOURCE, SyncSource_SMA );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCSOURCE)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setSync( const CimCfg::ImProbeTable &T )
{
    QVector<int>    vslots;
    int             ns,
                    srcSlot = -1;
    bool            ok      = true;

    if( p.sync.sourceIdx >= DAQ::eSyncSourceIM )
        srcSlot = T.getEnumSlot( p.sync.sourceIdx - DAQ::eSyncSourceIM );

// PXI

    ns = T.getTypedSlots( vslots, NPPlatform_PXI );

    for( int is = 0; is < ns; ++is ) {

        int slot = vslots[is];

        if( T.simprb.isSimSlot( slot ) )
            continue;

        ok = true;  // listener automatically

        if( slot == srcSlot )
            ok = _aux_setPXISyncAsOutput( slot );
        else if( slot == p.sync.imPXIInputSlot )
            ok = _aux_setPXISyncAsInput( slot );

        if( !ok )
            return false;
    }

// Obx

    ns = T.getTypedSlots( vslots, NPPlatform_USB );

    for( int is = 0; is < ns; ++is ) {

        int slot = vslots[is];

        if( T.simprb.isSimSlot( slot ) )
            continue;

        if( slot == srcSlot )
            ok = _aux_setObxSyncAsOutput( srcSlot );
        else
            ok = _aux_setObxSyncAsInput( slot );

        if( !ok )
            return false;
    }

    if( p.sync.sourceIdx >= DAQ::eSyncSourceIM )
        Log() << "IMEC syncing set to: ENABLED/OUTPUT";
    else
        Log() << "IMEC syncing set to: ENABLED/INPUT";

    return true;
}


bool CimAcqImec::_aux_config()
{
    STOPCHECK;

    if( !_aux_sizeStreamBufs() )
        return false;

    STEPAUX();
    STOPCHECK;

    if( !_aux_open( T ) )
        return false;

    STEPAUX();
    STOPCHECK;

    if( !_aux_setSync( T ) )
        return false;

    STEPAUX();
    return true;
}

#endif  // HAVE_IMEC


