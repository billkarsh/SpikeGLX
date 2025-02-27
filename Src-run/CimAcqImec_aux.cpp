
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "Subset.h"

#ifdef HAVE_VISA
#define NIVISA_PXI  // include PXI VISA attributes
#include "NI/visa.h"
#include <QSettings>
#endif


#define STOPCHECK   if( isStopped() ) return false;


// @@@ FIX Leave buffers at defaults until understand better.
// Setting is system-wide, affecting PXI and OneBox.
// NP_PARAM_INPUT_LATENCY_US:   default 132, ~ 4 samples
//
// Testing notes:
// - Minimum legal value is 20.
// - At 50, sytems using Thunderbolt have difficulty waking
//  up from screen saver.
// - Performance looks stable at 100 and 132 (default), that was
//  tested with eight NP2.0 probes in a single BS.
// - Other values have not been tested.
//
bool CimAcqImec::_aux_sizeStreamBufs()
{
#if 0
    NP_ErrorCode    err;

    err = np_setParameter( NP_PARAM_INPUT_LATENCY_US, 132 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter(LATENCY)%1")
            .arg( makeErrorString( err ) ) );
        return false;
    }
#endif

    return true;
}


bool CimAcqImec::_aux_initObxSlot( int slot )
{
    int             istr = p.im.obx_slot2istr( slot );
    NP_ErrorCode    err;

// ------
// Always
// ------

// Enable Sync

    err = np_ADC_enableProbe( slot, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC ADC_enableProbe(slot %1)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

// Not XIO

    if( istr < 0 ) {

        for( int ic = 0; ic < imOBX_NCHN; ++ic ) {

            err = np_DAC_enableOutput( slot, ic, false );

            if( err != SUCCESS ) {
                runError(
                    QString("IMEC DAC_enableOutput(slot %1, chn %2, false)%3")
                    .arg( slot ).arg( ic ).arg( makeErrorString( err ) ) );
                return false;
            }
        }

        return true;
    }

// ---
// ADC
// ---

    const CimCfg::ObxEach   &E = p.im.get_iStrOneBox( istr );

    if( E.isStream() ) {

        double      v   = E.range.rmax;
        ADCrange_t  rng = ADC_RANGE_5V;

        if( E.range.rmax < 4.0 )
            rng = ADC_RANGE_2_5V;
        else if( E.range.rmax > 6.0 )
            rng = ADC_RANGE_10V;

        err = np_ADC_setVoltageRange( slot, rng );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC ADC_setVoltageRange(slot %1 %2V)%3")
                .arg( slot ).arg( v ).arg( makeErrorString( err ) ) );
            return false;
        }

        for( int ic = 0; ic < imOBX_NCHN; ++ic ) {

            err = np_ADC_setComparatorThreshold( slot, ic, 0.1 * v, 0.1 * v );

            if( err != SUCCESS ) {
                runError(
                    QString("IMEC ADC_setComparatorThreshold(slot %1 chn %2 T %3)%4")
                    .arg( slot ).arg( ic ).arg( 0.1 * v ).arg( makeErrorString( err ) ) );
                return false;
            }
        }
    }

// ---------
// ADC / DAC
// ---------

    QVector<uint>   vAO;
    Subset::rngStr2Vec( vAO, E.uiAOStr );

    for( int ic = 0; ic < imOBX_NCHN; ++ic ) {

        bool    isDAC = vAO.contains( ic );

        err = np_DAC_enableOutput( slot, ic, isDAC );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC DAC_enableOutput(slot %1, chn %2, false)%3")
                .arg( slot ).arg( ic ).arg( makeErrorString( err ) ) );
            return false;
        }

        if( isDAC ) {

            err = np_DAC_setVoltage( slot, ic, 0 );

            if( err != SUCCESS ) {
                runError(
                    QString("IMEC DAC_setVoltage(slot %1, chn %2, 0)%3")
                    .arg( slot ).arg( ic ).arg( makeErrorString( err ) ) );
                return false;
            }
        }
    }

    return true;
}


void CimAcqImec::_aux_doneObxSlot( int slot )
{
    np_switchmatrix_clear( slot, SM_Output_SMA1 );

    int istr = p.im.obx_slot2istr( slot );
    if( istr < 0 )
        return;

    const CimCfg::ObxEach   &E   = p.im.get_iStrOneBox( istr );
    QVector<uint>           vAO;

    Subset::rngStr2Vec( vAO, E.uiAOStr );

    for( int ic = 0; ic < imOBX_NCHN; ++ic ) {

        if( vAO.contains( ic ) ) {
            np_DAC_setVoltage( slot, ic, 0 );
            np_DAC_enableOutput( slot, ic, false );
        }
    }
}


bool CimAcqImec::_aux_open( const CimCfg::ImProbeTable &T )
{
    for( int is = 0, ns = T.nSelSlots(); is < ns; ++is ) {

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
            if( !_aux_initObxSlot( slot ) )
                return false;
        }
    }

    return true;
}


bool CimAcqImec::_aux_clrObxSync( int slot )
{
    NP_ErrorCode    err;

    err = np_switchmatrix_clear( slot, SM_Output_StatusBit );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_clear(slot %1, OBXBIT6SYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_clear( slot, SM_Output_SMA1 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_clear(slot %1, OBXSMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setObxSyncAsOutput( int slot )
{
    if( !_aux_clrObxSync( slot ) )
        return false;

    NP_ErrorCode    err;

    err = np_setSyncClockPeriod( slot, 1000 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setOBXSyncClockPeriod(slot %1)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot,
            SM_Output_StatusBit, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OBXOUTBIT6SYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_SMA1, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OBXOUTSMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setObxSyncAsInput( int slot )
{
    if( !_aux_clrObxSync( slot ) )
        return false;

    NP_ErrorCode    err;

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_SMA1, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, OBXINBIT6SYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_clrPXISync( int slot )
{
    NP_ErrorCode    err;

    err = np_switchmatrix_clear( slot, SM_Output_StatusBit );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_clear(slot %1, PXIBIT6SYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_clear( slot, SM_Output_PXISYNC );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_clear(slot %1, PXIBKPLSYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_clear( slot, SM_Output_SMA );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_clear(slot %1, PXISMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setPXISyncAsOutput( int slot )
{
    if( !_aux_clrPXISync( slot ) )
        return false;

    NP_ErrorCode    err;

    err = np_setSyncClockPeriod( slot, 1000 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setSyncClockPeriod(slot %1)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot,
            SM_Output_StatusBit, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, PXIOUTBIT6SYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_PXISYNC, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, PXIOUTBKPLSYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_SMA, SM_Input_SyncClk, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, PXIOUTSMASYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setPXISyncAsInput( int slot )
{
    if( !_aux_clrPXISync( slot ) )
        return false;

    NP_ErrorCode    err;

    err = np_switchmatrix_set( slot,
            SM_Output_StatusBit, SM_Input_SMASYNC, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, PXIINBIT6SYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    err = np_switchmatrix_set( slot, SM_Output_PXISYNC, SM_Input_SMASYNC, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, PXIINBKPLSYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_aux_setPXISyncAsListener( int slot )
{
    if( !_aux_clrPXISync( slot ) )
        return false;

    NP_ErrorCode    err;

    err = np_switchmatrix_set( slot, SM_Output_StatusBit, SM_Input_PXISYNC, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1, PXILSTNBIT6SYNC)%2")
            .arg( slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    return true;
}


// Note:
// Trigger bus settings applied here are undone by system reboot.
// That allows NI-MAX to regain control of routes and reservations.
//
bool CimAcqImec::_aux_setPXITrigBus( const QVector<int> &vslots, int srcSlot )
{
#ifdef HAVE_VISA

// --------------------------
// More than 1 selected slot?
// --------------------------

    int ns = vslots.size();

    if( ns < 2 )
        return true;

// ----------------------------
// Chassis has more than 1 bus?
// ----------------------------

    QSettings   S( osPath() + "/pxisys.ini", QSettings::IniFormat );
    QStringList busList;

    S.beginGroup( "Chassis1" );
    busList = S.value( "TriggerBusList" ).toString()
                .split( ",", QString::SkipEmptyParts );
    S.endGroup();

    int nb = busList.size();

    if( nb < 2 )
        return true;

// ------------------------------
// Parse highest slot on each bus
// ------------------------------

    QVector<int>    ib2maxslot;

    for( int ib = 0; ib < nb; ++ib ) {

        S.beginGroup( QString("Chassis1TriggerBus%1").arg( busList[ib] ) );
        QStringList sl = S.value( "SlotList" ).toString()
                            .split( ",", QString::SkipEmptyParts );
        S.endGroup();
        ib2maxslot.push_back( sl.last().toInt() );
    }

// -------------------------------------
// Which buses do selected slots occupy?
// -------------------------------------

    QVector<int>    ibOcc( nb, 0 );
    int             src_ibus = 0;

    for( int is = 0; is < ns; ++is ) {

        int slot = vslots[is],
            ibus = 0;

        for( int ib = 0; ib < nb; ++ib ) {
            if( slot <= ib2maxslot[ib] ) {
                ibus = ib;
                break;
            }
        }

        if( slot == srcSlot )
            src_ibus = ibus;

        ++ibOcc[ibus];
    }

// --------------------------------------
// Any occupied bus differs from src_bus?
// --------------------------------------

    bool    differs = false;

    for( int ib = 0; ib < nb; ++ib ) {
        if( ib != src_ibus && ibOcc[ib] > 0 ) {
            differs = true;
            break;
        }
    }

    if( !differs )
        return true;

// --------------------------------------
// Need to map at least one bus with VISA
// --------------------------------------

    ViSession   rsrcMgr, rsrc;
    ViStatus    status;
    bool        ok = false;

    status = viOpenDefaultRM( &rsrcMgr );
    if( status < VI_SUCCESS ) {
        Error() <<
        QString("VISA: (%1) Failed to open resource manager.").arg( status );
        return false;
    }

    status = viOpen( rsrcMgr, "PXI::1::BACKPLANE", 0, 0, &rsrc );
    if( status < VI_SUCCESS ) {
        Error() <<
        QString("VISA: (%1) Failed to open backplane resource.").arg( status );
        goto closeMgr;
    }

// ---------------
// Unmap the world
// ---------------

    for( int is = 0; is < nb; ++is ) {

        status = viSetAttribute( rsrc, VI_ATTR_PXI_SRC_TRIG_BUS, is + 1 );
        if( status < VI_SUCCESS )
            continue;

        for( int id = 0; id < nb; ++id ) {

            if( id == is )
                continue;

            status = viSetAttribute( rsrc, VI_ATTR_PXI_DEST_TRIG_BUS, id + 1 );
            if( status < VI_SUCCESS )
                continue;

            viUnmapTrigger( rsrc, VI_TRIG_TTL7, VI_TRIG_TTL7 );
        }
    }

// -----------
// New mapping
// -----------

    status = viSetAttribute( rsrc, VI_ATTR_PXI_SRC_TRIG_BUS, src_ibus + 1 );
    if( status < VI_SUCCESS ) {
        Error() <<
        QString("VISA: (%1) Failed to set source trigger bus.").arg( status );
        goto closeRsrc;
    }

    for( int ib = 0; ib < nb; ++ib ) {

        if( ib != src_ibus && ibOcc[ib] > 0 ) {

            status = viSetAttribute( rsrc, VI_ATTR_PXI_DEST_TRIG_BUS, ib + 1 );
            if( status < VI_SUCCESS ) {
                Error() <<
                QString("VISA: (%1) Failed to set destination trigger bus.").arg( status );
                goto closeRsrc;
            }

            status = viMapTrigger( rsrc, VI_TRIG_TTL7, VI_TRIG_TTL7, VI_NULL );
            if( status < VI_SUCCESS ) {
                Error() <<
                QString("VISA: (%1) Failed to map bus line 7.").arg( status );
                goto closeRsrc;
            }
        }
    }

    ok = true;

closeRsrc:
    status = viClose( rsrc );
closeMgr:
    status = viClose( rsrcMgr );
    return ok;

#else   // VISA disabled

    return true;

#endif
}


bool CimAcqImec::_aux_setSync( const CimCfg::ImProbeTable &T )
{
    if( p.stream_nIM() + p.stream_nOB() == 0 )
        return true;

    QVector<int>    vslots;
    int             ns,
                    srcSlot = -1,
                    inpSlot = -1;
    bool            ok      = true;

// Special slot roles

    if( p.sync.sourceIdx >= DAQ::eSyncSourceIM ) {

        srcSlot = T.getEnumSlot( p.sync.sourceIdx - DAQ::eSyncSourceIM );

        if( !T.isSlotPXIType( srcSlot ) )
            inpSlot = p.sync.imPXIInputSlot;
    }
    else
        inpSlot = p.sync.imPXIInputSlot;

// PXI

    ns = T.getTypedSlots( vslots, NPPlatform_PXI );

    for( int is = 0; is < ns; ++is ) {

        int slot = vslots[is];

        if( slot == srcSlot ) {
            ok = _aux_setPXITrigBus( vslots, slot ) &&
                 _aux_setPXISyncAsOutput( slot );
        }
        else if( slot == inpSlot ) {
            ok = _aux_setPXITrigBus( vslots, slot ) &&
                 _aux_setPXISyncAsInput( slot );
        }
        else
            ok = _aux_setPXISyncAsListener( slot );

        if( !ok )
            return false;
    }

// Obx

    ns = T.getTypedSlots( vslots, NPPlatform_USB );

    for( int is = 0; is < ns; ++is ) {

        int slot = vslots[is];

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


