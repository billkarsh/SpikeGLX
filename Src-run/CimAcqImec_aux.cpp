
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
            QString("IMEC setParameter( BUFSIZE )%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }
#endif

#if 0
    err = np_setParameter( NP_PARAM_BUFFERCOUNT, 64 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( BUFCOUNT )%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }
#endif

    return true;
}


// Standard settings for 1BX:
// - ADC_enableProbe( true ) enable sync/bit-6 and/or ADC.
// - DAC_enableOutput( false ) all channels input by default.
// - ADC_setVoltageRange( config or 5 ).
// - ADC_setComparatorThreshold( 0.5, 1.8 ) all channels 3.3V or 5V TTL.
//
bool CimAcqImec::_aux_init1BXSlot( const CimCfg::ImProbeTable &T, int slot )
{
    double          v;
    int             ob_ip = -1;
    NP_ErrorCode    err;

// -----------------------------
// Find Onebox ip with this slot
// -----------------------------

    for( int ip = 0, np = p.stream_nOB(); ip < np; ++ip ) {

        if( T.get_iOnebox( ip ).slot == slot ) {
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

    for( int ic = 0; ic < im1BX_NCHN; ++ic ) {

        err = np_DAC_enableOutput( slot, ic, false );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC DAC_enableOutput(slot %1, chn %2, false)%3")
                .arg( slot ).arg( ic ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    v = (ob_ip >= 0 ? p.im.obxj[ob_ip].range.rmax : 5.0);

    err = np_ADC_setVoltageRange( slot, v );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC ADC_setVoltageRange(slot %1 %2V)%3")
            .arg( slot ).arg( v ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_ADC_setComparatorThreshold( slot, 0.5, 1.8 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC ADC_setComparatorThreshold(slot %1 lo 0.5 hi 2.0)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
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
            if( !_aux_init1BXSlot( T, slot ) )
                return false;
        }
    }

    return true;
}


bool CimAcqImec::_aux_set1BXSyncAsOutput( int slot )
{
    NP_ErrorCode    err;

//@OBX review setting up Onebox sync output and input modes
    err = np_setParameter( NP_PARAM_SYNCMASTER, slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCMASTER )%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }

//    err = np_setParameter( NP_PARAM_SYNCSOURCE, SyncSource_Clock );

//    if( err != SUCCESS ) {
//        runError(
//            QString("IMEC setParameter( SYNCSOURCE )%1")
//            .arg( makeErrorString( err ) ) );
//        return false;
//    }

    err = np_setParameter( NP_PARAM_SYNCPERIOD_MS, 1000 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(slot %1, SYNCPERIOD )%1")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_SMA1, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OUT1BXSMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OUT1BXSTATSYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_set1BXSyncAsInput( int slot )
{
    NP_ErrorCode    err;

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_SMA1, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, IN1BXSTATSYNC)%2")
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
            QString("IMEC setParameter( SYNCMASTER )%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }

//    err = np_setParameter( NP_PARAM_SYNCSOURCE, SyncSource_Clock );

//    if( err != SUCCESS ) {
//        runError(
//            QString("IMEC setParameter( SYNCSOURCE )%1")
//            .arg( makeErrorString( err ) ) );
//        return false;
//    }

    err = np_setParameter( NP_PARAM_SYNCPERIOD_MS, 1000 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCPERIOD )%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_SMA, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OUTSMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_PXISYNC, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OUTPXISYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OUTSTATSYNC)%2")
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
            QString("IMEC setParameter( SYNCMASTER )%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_setParameter( NP_PARAM_SYNCSOURCE, SyncSource_SMA );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCSOURCE )%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_PXISYNC, SM_Input_SMA, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, INPXISYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_SMA, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, INSTATSYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setPXISyncAsListener( int slot )
{
    NP_ErrorCode    err;

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_PXISYNC, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, LISTENPXISYNC)%2")
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

        if( slot == srcSlot )
            ok = _aux_setPXISyncAsOutput( slot );
        else if( slot == p.sync.imPXIInputSlot )
            ok = _aux_setPXISyncAsInput( slot );
        else
            ok = _aux_setPXISyncAsListener( slot );

        if( !ok )
            return false;
    }

// 1BX

    ns = T.getTypedSlots( vslots, NPPlatform_USB );

    for( int is = 0; is < ns; ++is ) {

        int slot = vslots[is];

        if( T.simprb.isSimSlot( slot ) )
            continue;

        if( slot == srcSlot )
            ok = _aux_set1BXSyncAsOutput( srcSlot );
        else
            ok = _aux_set1BXSyncAsInput( slot );

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


