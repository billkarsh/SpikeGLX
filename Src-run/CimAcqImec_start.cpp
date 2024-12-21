
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"


#define STOPCHECK       if( isStopped() ) return false;
#define TRIGSOFTONLY    1


// Set all hardware slots to software trigger.
//
bool CimAcqImec::_st_setHwrTriggers()
{
    QVector<int>    vslot;
    int             ns = T.getTypedSlots( vslot, NPPlatform_ALL );

    for( int is = 0; is < ns; ++is ) {

        int             slot;
        NP_ErrorCode    err;

        // IN = software

        slot = vslot[is];

        err = np_switchmatrix_set( slot, SM_Output_AcquisitionTrigger, SM_Input_SWTrigger1, true );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC switchmatrix_set(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }

        // EDGE = rising

//@OBX367 Revisit TRIGGER
//        err = np_setTriggerEdge( slot, true );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerEdge(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}


// Set all Obx to software trigger.
//
bool CimAcqImec::_st_setObxTriggers()
{
    QVector<int>    vslot;
    int             ns = T.getTypedSlots( vslot, NPPlatform_USB );

    for( int is = 0; is < ns; ++is ) {

        int             slot;
        NP_ErrorCode    err;

        // IN = software

        slot = vslot[is];

        err = np_switchmatrix_set( slot, SM_Output_AcquisitionTrigger, SM_Input_SWTrigger1, true );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC switchmatrix_set(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }

        // EDGE = rising

//@OBX367 Revisit TRIGGER
//        err = np_setTriggerEdge( slot, true );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerEdge(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}


// Set lowest slot for software trigger.
// Pass to others on PXI_TRIG<1> with rising edge sensitivity.
//
bool CimAcqImec::_st_setPXITriggers()
{
    QVector<int>    vslot;
    int             ns  = T.getTypedSlots( vslot, NPPlatform_PXI ),
                    slot0;
    NP_ErrorCode    err;

    if( !ns )
        return true;

// -----------
// Lowest slot
// -----------

    slot0 = vslot[0];

// IN = software

    err = np_switchmatrix_set( slot0, SM_Output_AcquisitionTrigger, SM_Input_SWTrigger1, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1)%2")
            .arg( slot0 ).arg( makeErrorString( err ) ) );
        return false;
    }

// OUT = PXI_TRIG<1>

    err = np_switchmatrix_set( slot0, SM_Output_PXI1, SM_Input_SWTrigger1, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1)%2")
            .arg( slot0 ).arg( makeErrorString( err ) ) );
        return false;
    }

// EDGE = rising

//@OBX367 Revisit TRIGGER
//    err = np_setTriggerEdge( slot0, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setTriggerEdge(slot %1)%2")
            .arg( slot0 ).arg( makeErrorString( err ) ) );
        return false;
    }

// ------
// Others
// ------

// IN = PXI_TRIG<1>

    for( int is = 0; is < ns; ++is ) {

        int slot = vslot[is];

        if( slot == slot0 )
            continue;

        err = np_switchmatrix_set( slot, SM_Output_AcquisitionTrigger, SM_Input_PXI1, true );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC switchmatrix_set(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }

// EDGE = rising

//@OBX367 Revisit TRIGGER
//        err = np_setTriggerEdge( slot, true );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerEdge(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}


bool CimAcqImec::_st_setTriggers()
{
//@OBX Try software trigger all modules since PXIe chassis
//@OBX backplane bridge blocks signalling across slots 6-to-7.
#if TRIGSOFTONLY
    if( !_st_setHwrTriggers() )
        return false;
#else
    if( !_st_setObxTriggers() || !_st_setPXITriggers() )
        return false;
#endif

    Log()
        << "IMEC Trigger source: "
        << (p.im.prbAll.trgSource ? "hardware" : "software");
    return true;
}


bool CimAcqImec::_st_setArm()
{
    for( int is = 0, ns = T.nSelSlots(); is < ns; ++is ) {

        int slot = T.getEnumSlot( is );

        if( T.simprb.isSimSlot( slot ) )
            continue;

        NP_ErrorCode    err = np_arm( slot );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC arm(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    Log() << "IMEC Armed";
    return true;
}


#if TRIGSOFTONLY
bool CimAcqImec::_st_softStart()
{
    QVector<int>    vs;
    int             ns  = T.getTypedSlots( vs, NPPlatform_ALL );
    NP_ErrorCode    err;

    for( int is = 0; is < ns; ++is ) {

        int slot = vs[is];

        err = np_setSWTrigger( slot );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setSWTrigger(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}
#else
bool CimAcqImec::_st_softStart()
{
    QVector<int>    v1b;
    int             n1b     = T.getTypedSlots( v1b, NPPlatform_USB );
    int             sPXI    = T.getEnumSlot( 0 );
    NP_ErrorCode    err;

    for( int is = 0; is < n1b; ++is ) {

        int s1b = v1b[is];

        err = np_setSWTrigger( s1b );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setSWTrigger(slot %1)%2")
                .arg( s1b ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    if( !T.simprb.isSimSlot( sPXI )
        && T.getSlotType( sPXI ) == NPPlatform_PXI ) {

        err = np_setSWTrigger( sPXI );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setSWTrigger(slot %1)%2")
                .arg( sPXI ).arg( makeErrorString( err ) ) );
            return false;
        }
    }

    return true;
}
#endif


bool CimAcqImec::_st_config()
{
    STOPCHECK;

    if( !_st_setTriggers() )
        return false;

    STEPSTART();
    STOPCHECK;

    if( !_st_setArm() )
        return false;

    STEPSTART();
    return true;
}

#endif  // HAVE_IMEC


