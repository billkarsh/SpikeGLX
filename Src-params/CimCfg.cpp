
#include "Util.h"
#include "CimCfg.h"
#include "KVParams.h"
#include "GeomMap.h"
#include "Subset.h"
#include "SignalBlocker.h"
#include "Version.h"

#ifdef HAVE_IMEC
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;
#else
#pragma message("*** Message to self: Building simulated IMEC version ***")
#endif

#include <QDirIterator>
#include <QFileInfo>
#include <QSettings>
#include <QTableWidget>


/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_IMEC
static QString makeErrorString( NP_ErrorCode err )
{
    char    buf[2048];
    size_t  n = np_getLastErrorMessage( buf, sizeof(buf) );

    if( n >= sizeof(buf) )
        n = sizeof(buf) - 1;

    buf[n] = 0;

    return QString(" error %1 '%2'.").arg( err ).arg( QString(buf) );
}
#endif

/* ---------------------------------------------------------------- */
/* struct IMProbeDat ---------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimCfg::ImProbeDat::setProbeType()
{
    return IMROTbl::pnToType( type, pn );
}


int CimCfg::ImProbeDat::nHSDocks() const
{
    if( type == 21 || type == 2003 ||
        type == 24 || type == 2013 || type == 2020 ) {

        return 2;
    }

    return 1;
}


void CimCfg::ImProbeDat::loadSettings( QSettings &S, int i )
{
    QString defRow =
        QString("slot:%1 port:%2 dock:%3 enab:1")
        .arg( 2 + i/4 ).arg( 1 + i%4 ).arg( 1 + (i & 1) );

    QString row =
        S.value( QString("row%1").arg( i ), defRow ).toString();

    QStringList sl = row.split(
                        QRegExp("\\s+"),
                        QString::SkipEmptyParts );

    QStringList s;

    s = sl[0].split(
            QRegExp("^\\s+|\\s*:\\s*"),
            QString::SkipEmptyParts );

    slot = s.at( 1 ).toUInt();

    s = sl[1].split(
            QRegExp("^\\s+|\\s*:\\s*"),
            QString::SkipEmptyParts );

    port = s.at( 1 ).toUInt();

    s = sl[2].split(
            QRegExp("^\\s+|\\s*:\\s*"),
            QString::SkipEmptyParts );

    dock = s.at( 1 ).toUInt();

    s = sl[3].split(
            QRegExp("^\\s+|\\s*:\\s*"),
            QString::SkipEmptyParts );

    enab = s.at( 1 ).toUInt();
}


void CimCfg::ImProbeDat::saveSettings( QSettings &S, int i ) const
{
    S.setValue(
        QString("row%1").arg( i ),
        QString("slot:%1 port:%2 dock:%3 enab:%4")
            .arg( slot ).arg( port ).arg( dock ).arg( enab ) );
}

/* ---------------------------------------------------------------- */
/* struct ImProbeTable -------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimCfg::ImProbeTable::init()
{
    for( int i = 0, n = probes.size(); i < n; ++i )
        probes[i].init();

    iprb2dat.clear();
    iobx2dat.clear();
    slotsUsed.clear();
    slot2zIdx.clear();

    api.clear();
    slot2Vers.clear();
}


// Build {iprb2dat[], iobx2dat[], slot[]} index arrays based on:
// entry enabled.
//
// Return enabled entries.
//
int CimCfg::ImProbeTable::buildEnabIndexTables()
{
    QMap<int,int>   mapSlots;   // ordered keys, arbitrary value

    iprb2dat.clear();
    iobx2dat.clear();
    slotsUsed.clear();
    slot2zIdx.clear();
    slot2type.clear();

    for( int i = 0, n = probes.size(); i < n; ++i ) {

        const ImProbeDat    &P = probes[i];

        if( P.enab ) {

            if( P.isProbe() )
                iprb2dat.push_back( i );
            else
                iobx2dat.push_back( i );

            mapSlots[P.slot] = 0;
        }
    }

    foreach( int key, mapSlots.keys() ) {

        slot2zIdx[key] = slotsUsed.size();
        slotsUsed.push_back( key );
    }

    return iprb2dat.size() + iobx2dat.size();
}


// Build {iprb2dat[], iobx2dat[], slot[]} index arrays based on:
// entry enabled AND version data valid.
//
// Return qualified entries.
//
int CimCfg::ImProbeTable::buildQualIndexTables()
{
    QMap<int,int>   mapSlots;   // ordered keys, arbitrary value
    int             nProbes = 0,
                    nOneBox = 0;

    iprb2dat.clear();
    iobx2dat.clear();
    slotsUsed.clear();
    slot2zIdx.clear();

    for( int i = 0, n = probes.size(); i < n; ++i ) {

        ImProbeDat  &P = probes[i];

        if( !P.enab )
            continue;

        if( P.isOneBox() && P.obsn >= 0 ) {

            P.ip = nOneBox++;
            iobx2dat.push_back( i );
            mapSlots[P.slot] = 0;
        }
        else if( P.hssn != UNSET64 && P.sn != UNSET64 ) {

            P.ip = nProbes++;
            iprb2dat.push_back( i );
            mapSlots[P.slot] = 0;
        }
    }

    foreach( int key, mapSlots.keys() ) {

        slot2zIdx[key] = slotsUsed.size();
        slotsUsed.push_back( key );
    }

    return nProbes + nOneBox;
}


bool CimCfg::ImProbeTable::haveQualCalFiles() const
{
    for( int i = 0, n = probes.size(); i < n; ++i ) {

        const ImProbeDat    &P = probes[i];

        if( P.enab && P.hssn != UNSET64 && P.sn != UNSET64 ) {

            if( P.cal != 1 )
                return false;
        }
    }

    return true;
}


// Update:
// - In-memory probe and slot tables.
// - Config file improbetable.
// - Config file imslottable.
//
void CimCfg::ImProbeTable::setCfgSlots( const QVector<CfgSlot> &vCS )
{
    probes.clear();
    onebx2slot.clear();

    for( int ics = 0, ncs = vCS.size(); ics < ncs; ++ics ) {

        const CfgSlot   &CS = vCS[ics];

        if( CS.ID )
           onebx2slot[CS.ID] = CS.slot;

        if( CS.show == 0 )
            continue;

        for( int iport = 1, np = (CS.ID ? 2 : 4); iport <= np; ++iport ) {
            for( int idock = 1; idock <= CS.show; ++idock ) {

                bool    enab = setEnabled.has( CS.slot, iport, idock );
                probes.push_back( ImProbeDat(CS.slot, iport, idock, enab) );
            }
        }

        if( CS.ID ) {
            bool    enab = setEnabled.has( CS.slot, 9, 1 );
            probes.push_back( ImProbeDat(CS.slot, 9, 1, enab) );
        }
    }

    std::sort( probes.begin(), probes.end() );

    saveProbeTable();
    saveSlotTable();
}


// PXI: Get slots in probe table.
// OBX: Get union of probe table and imslottable.ini.
//
void CimCfg::ImProbeTable::getCfgSlots( QVector<CfgSlot> &vCS )
{
    vCS.clear();

// ---------------------------
// Get probe table slot/ndocks
// ---------------------------

    QMap<int,int>   slot2CS;

    for( int i = 0, n = probes.size(); i < n; ++i ) {

        const ImProbeDat    &P = probes[i];

        // Look back from entry (port,dock) = (2,1):
        // is prior entry (1,1) or (1,2)?

        if( P.port == 2 && P.dock == 1 && i > 0 ) {
            slot2CS[P.slot] = vCS.size();
            vCS.push_back( CfgSlot( P.slot, 0, probes[i-1].dock, false ) );
        }
    }

// -----------------
// Get OBX from file
// -----------------

    loadSlotTable();

    QMap<int,int>::const_iterator   it;

    for( it = onebx2slot.begin(); it != onebx2slot.end(); ++it ) {

        int ID      = it.key(),
            slot    = it.value();

        if( slot2CS.find( slot ) != slot2CS.end() )
            vCS[slot2CS[slot]].ID = ID;
        else {
            slot2CS[slot] = vCS.size();
            vCS.push_back( CfgSlot( slot, ID, 0, false ) );
        }
    }

// ----------
// Final sort
// ----------

    std::sort( vCS.begin(), vCS.end() );
}


// Set detected fields of existing members.
// Add new detected members, with new OneBoxes getting slot imSlotNone.
//
bool CimCfg::ImProbeTable::scanCfgSlots( QVector<CfgSlot> &vCS, QString &msg ) const
{
// Indices into table

    QMap<int,int>   slot2CS, ID2CS;

    for( int i = 0, n = vCS.size(); i < n; ++i ) {

        CfgSlot &CS = vCS[i];

#ifdef HAVE_IMEC
        CS.detected = CS.slot >= imSlotSIMMin;  // reset until scanned below
#else
        CS.detected = true;
#endif

        slot2CS[CS.slot] = i;

        if( CS.ID )
            ID2CS[CS.ID] = i;
    }

#ifdef HAVE_IMEC
// Scan

    basestationID   BS[imSlotPhyLim];
    int             nBS;
    NP_ErrorCode    err;

    err = np_scanBS();

    if( err != SUCCESS ) {
        msg = QString("Error scanBS() [%1]").arg( makeErrorString( err ) );
        return false;
    }

// Detect PXI

    for( int slot = imSlotPXIMin; slot < imSlotPXILim; ++slot ) {

        if( SUCCESS == np_getDeviceInfo( slot, BS )
            && BS->platformid == NPPlatform_PXI ) {

            if( slot2CS.find( slot ) != slot2CS.end() )
                vCS[slot2CS[slot]].detected = true;
            else
                vCS.push_back( CfgSlot( slot, 0, 0, true ) );
        }
    }

// Detect OneBoxes

    nBS = np_getDeviceList( BS, imSlotPhyLim );

    for( int ibs = 0; ibs < nBS; ++ibs ) {

        if( BS[ibs].platformid == NPPlatform_USB ) {

            int ID = BS[ibs].ID;

            if( ID2CS.find( ID ) != ID2CS.end() )
                vCS[ID2CS[ID]].detected = true;
            else
                vCS.push_back( CfgSlot( imSlotNone, ID, 0, true ) );
        }
    }
#endif

// ----------
// Final sort
// ----------

    std::sort( vCS.begin(), vCS.end() );

    msg = "Bus scan OK";
    return true;
}


bool CimCfg::ImProbeTable::mapObxSlots( QStringList &slVers )
{
#ifdef HAVE_IMEC

// Scan devices

    NP_ErrorCode    err;

    err = np_scanBS();

    if( err != SUCCESS ) {
        slVers.append(
            QString("IMEC scanBS()%1").arg( makeErrorString( err ) ) );
        return false;
    }

// Table: ID -> slot

    loadSlotTable();

// Table: slot -> ID

    QMap<int,int>                   inv;
    QMap<int,int>::const_iterator   it, end = onebx2slot.end();

    for( it = onebx2slot.begin(); it != end; ++it )
        inv[it.value()] = it.key();

// Loop over enabled slots

    for( int is = 0, ns = nLogSlots(); is < ns; ++is ) {

        int slot = getEnumSlot( is );

        if( simprb.isSimSlot( slot ) )
            continue;

        // OneBox slot

        if( slot >= imSlotUSBMin ) {

            int ID = inv.value( slot, -1 );

            // Got user assignment?

            if( ID == -1 ) {
                slVers.append(
                    QString("SLOTS: You haven't assigned slot %1"
                    " to a OneBox.").arg( slot ) );
instruct:
                slVers.append(
                    "Use 'Configure Slots' (above the table)"
                    " to check your slot assignments." );
                return false;
            }

            // Map it

            err = np_mapBS( ID, slot );

            if( err != SUCCESS ) {
                slVers.append(
                    QString("IMEC mapBS(slot %1, ID %2)%3")
                    .arg( slot ).arg( ID )
                    .arg( makeErrorString( err ) ) );
                goto instruct;
            }

            // Check it

            basestationID   BS;

            err = np_getDeviceInfo( slot, &BS );

            if( err != SUCCESS ) {
                slVers.append(
                    QString("IMEC mapBS-check(slot %1, ID %2)%3")
                    .arg( slot ).arg( ID )
                    .arg( makeErrorString( err ) ) );
                goto instruct;
            }

            if( BS.platformid != NPPlatform_USB || BS.ID != ID ) {
                slVers.append(
                    QString("SLOTS: You haven't assigned slot %1"
                    " to an existing OneBox.").arg( slot ) );
                goto instruct;
            }
        }
    }
#else
    Q_UNUSED( slVers )
#endif

    return true;
}


bool CimCfg::ImProbeTable::anySlotPXIType() const
{
    foreach( int slot, slotsUsed ) {
        if( isSlotPXIType( slot ) )
            return true;
    }

    return false;
}


bool CimCfg::ImProbeTable::isSlotPXIType( int slot ) const
{
#ifdef HAVE_IMEC
    return slot2type[slot] == NPPlatform_PXI;
#else
    Q_UNUSED( slot )
    return true;
#endif
}


bool CimCfg::ImProbeTable::isSlotUSBType( int slot ) const
{
#ifdef HAVE_IMEC
    return slot2type[slot] == NPPlatform_USB;
#else
    Q_UNUSED( slot )
    return true;
#endif
}


// List non-sim slots with bstype: NPPlatform_{PXI, USB, ALL}.
//
int CimCfg::ImProbeTable::getTypedSlots( QVector<int> &vslot, int bstype ) const
{
    vslot.clear();

    for( int is = 0, ns = slotsUsed.size(); is < ns; ++is ) {

        int slot = slotsUsed[is];

        if( !simprb.isSimSlot( slot ) && (slot2type[slot] & bstype) )
            vslot.push_back( slot );
    }

    return vslot.size();
}


int CimCfg::ImProbeTable::nQualStreamsThisSlot( int slot ) const
{
    int ns = 0;

    for( int i = 0, n = iprb2dat.size(); i < n; ++i ) {

        if( get_iProbe( i ).slot == slot )
            ++ns;
    }

    for( int i = 0, n = iobx2dat.size(); i < n; ++i ) {

        if( get_iOneBox( i ).slot == slot )
            ++ns;
    }

    return ns;
}


double CimCfg::ImProbeTable::get_iProbe_SRate( int i ) const
{
    if( i < iprb2dat.size() )
        return hssn2srate.value( probes[iprb2dat[i]].hssn, 30000.0 );

    return 30000.0;
}


double CimCfg::ImProbeTable::get_iOneBox_SRate( int i ) const
{
    if( i < iobx2dat.size() )
        return obsn2srate.value( probes[iobx2dat[i]].obsn, 30000.0 );

    return 30000.0;
}


// Calibrated probe (really, HS) sample rates.
// Maintained in _Calibration folder.
//
void CimCfg::ImProbeTable::loadProbeSRates()
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_imec" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedHeadStages" );

    hssn2srate.clear();

    foreach( const QString &hssn, settings.childKeys() ) {

        hssn2srate[hssn.toULongLong()] =
            settings.value( hssn, 30000.0 ).toDouble();
    }
}


// Calibrated probe (really, HS) sample rates.
// Maintained in _Calibration folder.
//
void CimCfg::ImProbeTable::saveProbeSRates() const
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_imec" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedHeadStages" );

    QMap<quint64,double>::const_iterator    it;

    for( it = hssn2srate.begin(); it != hssn2srate.end(); ++it )
        settings.setValue( QString("%1").arg( it.key() ), it.value() );
}


// Calibrated OneBox sample rates.
// Maintained in _Calibration folder.
//
void CimCfg::ImProbeTable::loadOneBoxSRates()
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_imec" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedOneBoxes" );

    obsn2srate.clear();

    foreach( const QString &obsn, settings.childKeys() ) {

        obsn2srate[obsn.toInt()] =
            settings.value( obsn, 30000.0 ).toDouble();
    }
}


// Calibrated OneBox sample rates.
// Maintained in _Calibration folder.
//
void CimCfg::ImProbeTable::saveOneBoxSRates() const
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_imec" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedOneBoxes" );

    QMap<int,double>::const_iterator    it;

    for( it = obsn2srate.begin(); it != obsn2srate.end(); ++it )
        settings.setValue( QString("%1").arg( it.key() ), it.value() );
}


// Table of OneBoxes and their user slot assignments.
// Slots must be unique and in range [imSlotUSBMin,imSlotLim).
// Maintained in _Configs folder.
//
void CimCfg::ImProbeTable::loadSlotTable()
{
    STDSETTINGS( settings, "imslottable" );
    settings.beginGroup( "OneBoxIdToSlot" );

    onebx2slot.clear();

    foreach( const QString &ID, settings.childKeys() )
        onebx2slot[ID.toInt()] = settings.value( ID, 0 ).toInt();
}


// Table of OneBoxes and their user slot assignments.
// Maintained in _Configs folder.
//
void CimCfg::ImProbeTable::saveSlotTable() const
{
    STDSETTINGS( settings, "imslottable" );
    settings.remove( "OneBoxIdToSlot" );
    settings.beginGroup( "OneBoxIdToSlot" );

    QMap<int,int>::const_iterator   it;

    for( it = onebx2slot.begin(); it != onebx2slot.end(); ++it )
        settings.setValue( QString("%1").arg( it.key() ), it.value() );
}


// Table of selectable (slot,port,dock) entries.
// Maintained in _Configs folder.
//
void CimCfg::ImProbeTable::loadProbeTable()
{
    probes.clear();

    STDSETTINGS( settings, "improbetable" );
    settings.beginGroup( "ImPrbTabUserInput" );

    int np = settings.value( "nrows", 0 ).toInt();

    probes.resize( np );

    for( int i = 0; i < np; ++i )
        probes[i].loadSettings( settings, i );

    std::sort( probes.begin(), probes.end() );
}


// Table of selectable (slot,port,dock) entries.
// Maintained in _Configs folder.
//
void CimCfg::ImProbeTable::saveProbeTable() const
{
    STDSETTINGS( settings, "improbetable" );
    settings.remove( "ImPrbTabUserInput" );
    settings.beginGroup( "ImPrbTabUserInput" );

    int np = probes.size();

    settings.setValue( "nrows", np );

    for( int i = 0; i < np; ++i )
        probes[i].saveSettings( settings, i );
}


#define TBL_SLOT    0
#define TBL_PORT    1
#define TBL_HSSN    2
#define TBL_DOCK    3
#define TBL_PRSN    4
#define TBL_PRPN    5
#define TBL_TYPE    6
#define TBL_ENAB    7
#define TBL_CAL     8
#define TBL_ID      9


void CimCfg::ImProbeTable::toGUI( QTableWidget *T ) const
{
    SignalBlocker   b0(T);

    int np = probes.size();

    T->setRowCount( np );

    for( int i = 0; i < np; ++i ) {

        QTableWidgetItem    *ti;
        const ImProbeDat    &P = probes[i];

        // ---------
        // Row label
        // ---------

        if( !(ti = T->verticalHeaderItem( i )) ) {
            ti = new QTableWidgetItem;
            T->setVerticalHeaderItem( i, ti );
        }

        ti->setText( QString::number( i ) );

        // ----
        // Slot
        // ----

        if( !(ti = T->item( i, TBL_SLOT )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_SLOT, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        ti->setText( QString::number( P.slot ) );

        // ----
        // Port
        // ----

        if( !(ti = T->item( i, TBL_PORT )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_PORT, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.isProbe() )
            ti->setText( QString::number( P.port ) );
        else
            ti->setText( "ADC" );

        // ----
        // HSSN
        // ----

        if( !(ti = T->item( i, TBL_HSSN )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_HSSN, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.isProbe() ) {
            if( P.hssn == UNSET64 )
                ti->setText( "???" );
            else
                ti->setText( QString::number( P.hssn ) );
        }

        // ----
        // Dock
        // ----

        if( !(ti = T->item( i, TBL_DOCK )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_DOCK, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.isProbe() )
            ti->setText( QString::number( P.dock ) );

        // --
        // SN
        // --

        if( !(ti = T->item( i, TBL_PRSN )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_PRSN, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.isProbe() ) {
            if( P.sn == UNSET64 )
                ti->setText( "???" );
            else
                ti->setText( QString::number( P.sn ) );
        }
        else if( P.obsn == -1 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.obsn ) );

        // --
        // PN
        // --

        if( !(ti = T->item( i, TBL_PRPN )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_PRPN, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.pn.isEmpty() )
            ti->setText( "???" );
        else
            ti->setText( P.pn );

        // ----
        // Type
        // ----

        if( !(ti = T->item( i, TBL_TYPE )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_TYPE, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.isProbe() ) {
            if( P.type == -1 )
                ti->setText( "???" );
            else
                ti->setText( QString::number( P.type ) );
        }

        // ----
        // Enab
        // ----

        if( !(ti = T->item( i, TBL_ENAB )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_ENAB, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
        }

        ti->setCheckState( P.enab ? Qt::Checked : Qt::Unchecked );

        // ---
        // Cal
        // ---

        if( !(ti = T->item( i, TBL_CAL )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_CAL, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.isProbe() ) {
            if( P.cal == quint16(-1) )
                ti->setText( "???" );
            else
                ti->setText( P.cal > 0 ? "Y" : "N" );
        }

        // --
        // Id
        // --

        if( !(ti = T->item( i, TBL_ID )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_ID, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.ip == quint16(-1) )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.ip ) );
    }
}


void CimCfg::ImProbeTable::fromGUI( QTableWidget *T )
{
    int np = T->rowCount();

    setEnabled.clear();

    for( int i = 0; i < np; ++i ) {

        ImProbeDat          &P = probes[i];
        QTableWidgetItem    *ti;
        quint64             v64;
        int                 val;
        bool                ok;

        // ----
        // Slot
        // ----

        ti  = T->item( i, TBL_SLOT );
        val = ti->text().toUInt( &ok );
        P.slot = (ok ? val : -1);

        // ----
        // Port
        // ----

        if( P.isProbe() ) {
            ti  = T->item( i, TBL_PORT );
            val = ti->text().toUInt( &ok );
            P.port = (ok ? val : -1);
        }

        // ----
        // HSSN
        // ----

        ti  = T->item( i, TBL_HSSN );
        v64 = ti->text().toULongLong( &ok );
        P.hssn = (ok ? v64 : UNSET64);

        // ----
        // Dock
        // ----

        if( P.isProbe() ) {
            ti  = T->item( i, TBL_DOCK );
            val = ti->text().toUInt( &ok );
            P.dock = (ok ? val : -1);
        }
        else
            P.dock = 1;

        // --
        // SN
        // --

        ti = T->item( i, TBL_PRSN );

        if( P.isProbe() ) {
            v64  = ti->text().toULongLong( &ok );
            P.sn = (ok ? v64 : UNSET64);
        }
        else {
            val    = ti->text().toInt( &ok );
            P.obsn = (ok ? val : -1);
        }

        // --
        // PN
        // --

        ti      = T->item( i, TBL_PRPN );
        P.pn    = ti->text();

        if( P.pn.contains( "?" ) )
            P.pn.clear();

        // ----
        // Type
        // ----

        ti  = T->item( i, TBL_TYPE );
        val = ti->text().toUInt( &ok );
        P.type = (ok ? val : -1);

        // ----
        // Enab
        // ----

        ti  = T->item( i, TBL_ENAB );
        P.enab = (ti->checkState() == Qt::Checked);

        // ---
        // Cal
        // ---

        ti  = T->item( i, TBL_CAL );

        if( P.isOneBox() || ti->text().contains( "?" ) )
            P.cal = -1;
        else
            P.cal = ti->text().contains( "Y" );

        // --
        // Id
        // --

        ti  = T->item( i, TBL_ID );
        val = ti->text().toUInt( &ok );
        P.ip = (ok ? val : -1);

        // ----------
        // setEnabled
        // ----------

        if( P.enab )
            setEnabled.store( P.slot, P.port, P.dock );
    }
}


// For probe entries...
// Set all checks to be like the one at row.
// subset = 0: ALL
// subset = 1: same slot
// subset = 2: same (slot, dock)
//
void CimCfg::ImProbeTable::toggleAll(
    QTableWidget    *T,
    int             row,
    int             subset ) const
{
    if( T->item( row, TBL_PORT )->text().toInt() == 9 )
        return;

    SignalBlocker   b0(T);

    QString         slot = T->item( row, TBL_SLOT )->text(),
                    dock = T->item( row, TBL_DOCK )->text();
    Qt::CheckState  enab = T->item( row, TBL_ENAB )->checkState();

    for( int i = 0, np = T->rowCount(); i < np; ++i ) {

        if( T->item( i, TBL_PORT )->text().toInt() == 9 )
            continue;

        // ----
        // Enab
        // ----

        if( subset == 0 ||
            (T->item( i, TBL_SLOT )->text() == slot &&
                (subset == 1 || T->item( i, TBL_DOCK )->text() == dock)) ) {

            T->item( i, TBL_ENAB )->setCheckState( enab );
        }
    }
}


// Create string: (slot,N):  (s,n)  (s,n)  ...
//
QString CimCfg::ImProbeTable::whosChecked( QTableWidget *T ) const
{
// Gather counts

    std::vector<int>    vS,
                        vN;
    int                 ns = 0;

    for( int i = 0, np = T->rowCount(); i < np; ++i ) {

        QTableWidgetItem    *ti;

        // ----
        // Enab
        // ----

        ti = T->item( i, TBL_ENAB );

        if( ti->checkState() == Qt::Checked ) {

            // ----
            // Slot
            // ----

            int     val;
            bool    ok;

            ti  = T->item( i, TBL_SLOT );
            val = ti->text().toUInt( &ok );

            if( ok ) {

                if( !ns || val != vS[ns - 1] ) {
                    vS.push_back( val );
                    vN.push_back( 1 );
                    ++ns;
                }
                else
                    ++vN[ns - 1];
            }
        }
    }

// Compose string

    QString s = "(slot,N):";

    for( int is = 0; is < ns; ++is )
        s += QString("  (%1,%2)").arg( vS[is] ).arg( vN[is] );

    return s;
}

/* ---------------------------------------------------------------- */
/* struct PrbAll -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimCfg::PrbAll::loadSettings( QSettings &S )
{
    qf_secsStr      = S.value( "imQfSecs", ".5" ).toString();
    qf_loCutStr     = S.value( "imQfLoCut", "300" ).toString();
    qf_hiCutStr     = S.value( "imQfHiCut", "9000" ).toString();
    calPolicy       = S.value( "imCalPolicy", 0 ).toInt();
    trgSource       = S.value( "imTrgSource", 0 ).toInt();
    svySecPerBnk    = S.value( "imSvySecPerBnk", 35 ).toInt();
    lowLatency      = S.value( "imLowLatency", false ).toBool();
    trgRising       = S.value( "imTrgRising", true ).toBool();
    bistAtDetect    = S.value( "imBistAtDetect", true ).toBool();
    isSvyRun        = false;
    qf_on           = S.value( "imQfOn", true ).toBool();
}


void CimCfg::PrbAll::saveSettings( QSettings &S ) const
{
    S.setValue( "imQfSecs", qf_secsStr );
    S.setValue( "imQfLoCut", qf_loCutStr );
    S.setValue( "imQfHiCut", qf_hiCutStr );
    S.setValue( "imCalPolicy", calPolicy );
    S.setValue( "imTrgSource", trgSource );
    S.setValue( "imSvySecPerBnk", svySecPerBnk );
    S.setValue( "imLowLatency", lowLatency );
    S.setValue( "imTrgRising", trgRising );
    S.setValue( "imBistAtDetect", bistAtDetect );
    S.setValue( "imQfOn", qf_on );
}


QString CimCfg::PrbAll::remoteGetPrbAll()
{
    STDSETTINGS( settings, "daq" );
    settings.beginGroup( "DAQ_Imec_All" );

    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    QStringList keys = settings.childKeys();

    foreach( const QString &key, keys )
        ts << key << "=" << settings.value( key ).toString() << "\n";

    return s;
}


void CimCfg::PrbAll::remoteSetPrbAll( const QString &str )
{
    STDSETTINGS( settings, "daqremote" );
    settings.beginGroup( "DAQ_Imec_All" );

    QTextStream ts( str.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString     line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > 0 && eq < line.length() - 1 ) {

            QString k = line.left( eq ).trimmed(),
                    v = line.mid( eq + 1 ).trimmed();

            settings.setValue( k, v );
        }
    }
}

/* ---------------------------------------------------------------- */
/* struct PrbEach ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Given input fields:
// - roTbl parameter
//
// Derive:
// - imCumTypCnt[]
//
// IMPORTANT:
// This is only called if Imec is enabled on the Devices tab,
// and then only for defined probes. That's OK because clients
// shouldn't access these data for an invalid probe.
//
void CimCfg::PrbEach::deriveChanCounts()
{
    if( !roTbl )
        return;

// --------------------------------
// First count each type separately
// --------------------------------

    imCumTypCnt[imTypeAP] = roTbl->nAP();
    imCumTypCnt[imTypeLF] = roTbl->nLF();
    imCumTypCnt[imTypeSY] = roTbl->nSY();

// ---------
// Integrate
// ---------

    for( int i = 1; i < imNTypes; ++i )
        imCumTypCnt[i] += imCumTypCnt[i - 1];
}


// Given input fields:
// - stdbyStr (trimmed)
// - nAP channels (parameter)
//
// Derive:
// - stdbyBits
//
// Return true if stdbyStr format OK.
//
bool CimCfg::PrbEach::deriveStdbyBits( QString &err, int nAP, int ip )
{
    err.clear();

    if( stdbyStr.isEmpty() )
        stdbyBits.fill( false, nAP );
    else if( Subset::isAllChansStr( stdbyStr ) ) {

        stdbyStr = "all";
        Subset::defaultBits( stdbyBits, nAP );
    }
    else if( Subset::rngStr2Bits( stdbyBits, stdbyStr ) ) {

        stdbyStr = Subset::bits2RngStr( stdbyBits );

        if( stdbyBits.size() > nAP ) {
            err = QString(
                    "Imec %1: Bad-channel string includes channels"
                    " outside range [0..%2].")
                    .arg( ip )
                    .arg( nAP - 1 );
            return false;
        }

        // in case smaller
        stdbyBits.resize( nAP );
    }
    else {
        err = QString(
                "Imec %1: Bad-channel string has incorrect format.")
                .arg( ip );
        return false;
    }

    return true;
}


void CimCfg::PrbEach::justAPBits(
    QBitArray       &apBits,
    const QBitArray &saveBits ) const
{
    apBits = saveBits;
    apBits.fill( 0, imCumTypCnt[imTypeAP], imCumTypCnt[imTypeLF] );
}


void CimCfg::PrbEach::justLFBits(
    QBitArray       &lfBits,
    const QBitArray &saveBits ) const
{
    lfBits = saveBits;
    lfBits.fill( 0, 0, imCumTypCnt[imTypeAP] );
}


void CimCfg::PrbEach::apSaveBits( QBitArray &apBits ) const
{
    justAPBits( apBits, sns.saveBits );
}


void CimCfg::PrbEach::lfSaveBits( QBitArray &lfBits ) const
{
    justLFBits( lfBits, sns.saveBits );
}


int CimCfg::PrbEach::apSaveChanCount() const
{
    QBitArray   apBits;
    apSaveBits( apBits );
    return apBits.count( true );
}


int CimCfg::PrbEach::lfSaveChanCount() const
{
    QBitArray   lfBits;
    lfSaveBits( lfBits );
    return lfBits.count( true );
}


bool CimCfg::PrbEach::lfIsSaving() const
{
    QBitArray   lfBits;
    lfSaveBits( lfBits );
    lfBits.fill( 0, imCumTypCnt[imTypeLF], imCumTypCnt[imSumAll] );
    return lfBits.count( true );
}


double CimCfg::PrbEach::chanGain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = imCumTypCnt[imTypeAP],
            nNu = imCumTypCnt[imTypeLF];

        if( ic < nAP )
            g = roTbl->apGain( ic );
        else if( ic < nNu )
            g = roTbl->lfGain( ic - nAP );
        else
            return 1.0;

        if( g < 50.0 )
            g = 50.0;
    }

    return g;
}


int CimCfg::PrbEach::vToInt( double v, int ic ) const
{
    return (roTbl->maxInt() - 1) * v * chanGain( ic ) / roTbl->maxVolts();
}


double CimCfg::PrbEach::intToV( int i, int ic ) const
{
    return roTbl->maxVolts() * i / (roTbl->maxInt() * chanGain( ic ));
}


void CimCfg::PrbEach::loadSettings( QSettings &S )
{
    when                = S.value( "__when", QDateTime::currentDateTime().toString() ).toString();
    imroFile            = S.value( "imroFile", QString() ).toString();
    stdbyStr            = S.value( "imStdby", QString() ).toString();
    svyMaxBnk           = S.value( "imSvyMaxBnk", -1 ).toInt();
    LEDEnable           = S.value( "imLEDEnable", false ).toBool();
    sns.chanMapFile     = S.value( "imSnsChanMapFile", QString() ).toString();
    sns.uiSaveChanStr   = S.value( "imSnsSaveChanSubset", "all" ).toString();
}


void CimCfg::PrbEach::saveSettings( QSettings &S ) const
{
    S.setValue( "__when", when );
    S.setValue( "imroFile", imroFile );
    S.setValue( "imStdby", stdbyStr );
    S.setValue( "imSvyMaxBnk", svyMaxBnk );
    S.setValue( "imLEDEnable", LEDEnable );
    S.setValue( "imSnsChanMapFile", sns.chanMapFile );
    S.setValue( "imSnsSaveChanSubset", sns.uiSaveChanStr );
}


QString CimCfg::PrbEach::remoteGetGeomMap() const
{
    QString         s;
    GeomMap         G;
    QVector<uint>   vC;
    int             nAP = roTbl->nAP();

    vC.reserve( nAP );
    for( int ic = 0; ic < nAP; ++ic )
        vC.push_back( ic );

    roTbl->toGeomMap_snsFileChans( G, vC, 0 );
    G.andOutImStdby( stdbyBits, vC, 0 );

    s  = QString("head_partNumber=%1\n").arg( G.pn );
    s += QString("head_numShanks=%1\n").arg( G.ns );
    s += QString("head_shankPitch=%1\n").arg( G.ds );
    s += QString("head_shankWidth=%1\n").arg( G.wd );

    for( int ic = 0; ic < nAP; ++ic ) {
        const GeomMapDesc   E = G.e[ic];
        s += QString("ch%1_s=%2\n").arg( ic ).arg( E.s );
        s += QString("ch%1_x=%2\n").arg( ic ).arg( E.x );
        s += QString("ch%1_z=%2\n").arg( ic ).arg( E.z );
        s += QString("ch%1_u=%2\n").arg( ic ).arg( E.u );
    }

    return s;
}


QString CimCfg::PrbEach::remoteGetPrbEach() const
{
    QString s;

    s  = QString("imroFile=%1\n").arg( imroFile );
    s += QString("imStdby=%1\n").arg( stdbyStr );
    s += QString("imSvyMaxBnk=%1\n").arg( svyMaxBnk );
    s += QString("imLEDEnable=%1\n").arg( LEDEnable );
    s += QString("imSnsChanMapFile=%1\n").arg( sns.chanMapFile );
    s += QString("imSnsSaveChanSubset=%1\n").arg( sns.uiSaveChanStr );

    return s;
}

/* ---------------------------------------------------------------- */
/* struct ObxEach ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Given input fields:
// - uiXAStr
// - digital
//
// Derive:
// - obCumTypCnt[]
//
// IMPORTANT:
// This is only called if Imec is enabled on the Devices tab,
// and then only for defined OneBoxes. That's OK because clients
// shouldn't access these data for an invalid OneBox.
//
void CimCfg::ObxEach::deriveChanCounts()
{
    QVector<uint>   vc;

// --------------------------------
// First count each type separately
// --------------------------------

    Subset::rngStr2Vec( vc, uiXAStr );
    obCumTypCnt[obTypeXA] = vc.size();
    obCumTypCnt[obTypeXD] = (digital ? 1 : 0);
    obCumTypCnt[obTypeSY] = 1;

// ---------
// Integrate
// ---------

    for( int i = 1; i < obNTypes; ++i )
        obCumTypCnt[i] += obCumTypCnt[i - 1];
}


int CimCfg::ObxEach::vToInt16( double v ) const
{
    return SHRT_MAX * v / range.rmax;
}


double CimCfg::ObxEach::int16ToV( int i16 ) const
{
    return range.rmax * i16 / SHRT_MAX;
}


void CimCfg::ObxEach::loadSettings( QSettings &S )
{
    range.rmax          = S.value( "obAiRangeMax", 5.0 ).toDouble();
    range.rmin          = -range.rmax;
    when                = S.value( "__when", QDateTime::currentDateTime().toString() ).toString();
    uiXAStr             = S.value( "obXAChans", "0:11" ).toString();
    digital             = S.value( "obDigital", true ).toBool();
//    sns.shankMapFile    = S.value( "obSnsShankMapFile", QString() ).toString();
    sns.chanMapFile     = S.value( "obSnsChanMapFile", QString() ).toString();
    sns.uiSaveChanStr   = S.value( "obSnsSaveChanSubset", "all" ).toString();
}


void CimCfg::ObxEach::saveSettings( QSettings &S ) const
{
    S.setValue( "obAiRangeMax", range.rmax );
    S.setValue( "__when", when );
    S.setValue( "obXAChans", uiXAStr );
    S.setValue( "obDigital", digital );
//    S.setValue( "obSnsShankMapFile", sns.shankMapFile );
    S.setValue( "obSnsChanMapFile", sns.chanMapFile );
    S.setValue( "obSnsSaveChanSubset", sns.uiSaveChanStr );
}


QString CimCfg::ObxEach::remoteGetObxEach() const
{
    QString s;

    s  = QString("obAiRangeMax=%1\n").arg( range.rmax );
    s += QString("obXAChans=%1\n").arg( uiXAStr );
    s += QString("obDigital=%1\n").arg( digital );
//    s += QString("obSnsShankMapFile=%1\n").arg( sns.shankMapFile );
    s += QString("obSnsChanMapFile=%1\n").arg( sns.chanMapFile );
    s += QString("obSnsSaveChanSubset=%1\n").arg( sns.uiSaveChanStr );

    return s;
}

/* ---------------------------------------------------------------- */
/* class CimCfg --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimCfg::set_ini_nprb_nobx( int nprb, int nobx )
{
    nProbes = nprb;
    nOneBox = nobx;
    prbj.resize( nprb );
    obxj.resize( nobx );
}


void CimCfg::set_cfg_def_no_streams( const CimCfg &RHS )
{
    *this   = RHS;
    enabled = false;
    nProbes = 0;
    nOneBox = 0;
    prbj.clear();
    obxj.clear();
}


void CimCfg::set_cfg_nprb( const QVector<PrbEach> &each, int nprb )
{
    enabled = nprb > 0;
    nProbes = nprb;
    prbj    = each;
    prbj.resize( nprb );

    for( int ip = 0; ip < nProbes; ++ip )
        prbj[ip].deriveChanCounts();
}


void CimCfg::set_cfg_nobx( const QVector<ObxEach> &each, int nobx )
{
    enabled = nobx > 0;
    nOneBox = nobx;
    obxj    = each;
    obxj.resize( nobx );

    for( int ip = 0; ip < nOneBox; ++ip )
        obxj[ip].deriveChanCounts();
}


void CimCfg::loadSettings( QSettings &S )
{
// ---
// ALL
// ---

    S.beginGroup( "DAQ_Imec_All" );

    prbAll.loadSettings( S );

    nProbes =
    S.value( "imNProbes", 0 ).toInt();

    nOneBox =
    S.value( "obNOneBox", 0 ).toInt();

    enabled =
    S.value( "imEnabled", false ).toBool();

    S.endGroup();

    set_ini_nprb_nobx( nProbes, nOneBox );
}


void CimCfg::saveSettings( QSettings &S ) const
{
// ---
// ALL
// ---

    S.beginGroup( "DAQ_Imec_All" );

    prbAll.saveSettings( S );

    S.setValue( "imNProbes", nProbes );
    S.setValue( "obNOneBox", nOneBox );
    S.setValue( "imEnabled", enabled );

    S.endGroup();
}


// Close all installed slots and decouple their sync outputs.
//
void CimCfg::closeAllBS( bool report )
{
#ifdef HAVE_IMEC
    if( report ) {
        QString s = "Manually closing hardware; about 4 seconds...";
        Systray() << s;
        Log() << s;

        guiBreathe();
        guiBreathe();
        guiBreathe();
    }

// Assign real (arb) slot numbers to OneBoxes so we can talk to them.

    basestationID   BS[imSlotPhyLim];
    int             nBS,
                    slotObx = imSlotUSBMin;

    np_scanBS();
    nBS = np_getDeviceList( BS, imSlotPhyLim );

    for( int ibs = 0; ibs < nBS; ++ibs ) {
        if( BS[ibs].platformid == NPPlatform_USB )
            np_mapBS( BS[ibs].ID, slotObx++ );
    }

// Loop over extant real slots

    for( int slot = imSlotMin; slot < imSlotPhyLim; ++slot ) {

        if( SUCCESS == np_getDeviceInfo( slot, BS ) ) {

            // Note: These calls all return SUCCESS regardless of API.

            np_closeBS( slot );
            np_openBS( slot );
            np_switchmatrix_clear( slot, SM_Output_SMA );
            np_switchmatrix_clear( slot, SM_Output_PXISYNC );
            np_closeBS( slot );
        }
    }

    if( report ) {
        QString s = "Done closing hardware";
        Systray() << s;
        Log() << s;
    }
#else
    Q_UNUSED( report )
#endif
}

#define DBG 0

bool CimCfg::detect(
    QStringList     &slVers,
    QStringList     &slBIST,
    QVector<int>    &vHSpsv,
    QVector<int>    &vHS20,
    ImProbeTable    &T,
    bool            doBIST )
{
// @@@ FIX closeAllBS is preferred but disrupts OneBox mapping
//    closeAllBS( false );

#if DBG
Log()<<"[[ Start Detect";
guiBreathe();
#endif
    T.init();
    slVers.clear();
    slBIST.clear();
    vHSpsv.clear();
    vHS20.clear();

    T.buildEnabIndexTables();

    T.simprb.loadSettings();

// slot real if running any real probe
    for( int ip = 0, np = T.nLogProbes(); ip < np; ++ip ) {
        const ImProbeDat    &P = T.get_iProbe( ip );
        if( !T.simprb.isSimProbe( P.slot, P.port, P.dock ) )
            T.simprb.addHwrSlot( P.slot );
    }

// slot real if running obx
    for( int ip = 0, np = T.nLogOneBox(); ip < np; ++ip )
        T.simprb.addHwrSlot( T.get_iOneBox( ip ).slot );

    if( !T.mapObxSlots( slVers ) )
        return false;
#if DBG
Log()<<"  Slots mapped";
guiBreathe();
#endif

#if DBG
Log()<<"[ Start detect_API";
guiBreathe();
#endif
    detect_API( slVers, T );
#if DBG
Log()<<"End detect_API ]";
guiBreathe();
#endif

#if DBG
Log()<<"[ Start detect_Slots";
guiBreathe();
#endif
    bool    ok = detect_Slots( slVers, T );
#if DBG
Log()<<"End detect_Slots ]";
guiBreathe();
#endif

#if DBG
Log()<<"[ Start detect_Probes";
guiBreathe();
#endif
    if( ok )
        ok = detect_Probes( slVers, slBIST, vHSpsv, vHS20, T, doBIST );
#if DBG
Log()<<"End detect_Probes ]";
guiBreathe();
#endif

#if DBG
Log()<<"[ Start detect_OneBoxes";
guiBreathe();
#endif
    if( ok )
        detect_OneBoxes( T );
#if DBG
Log()<<"End detect_OneBoxes ]";
guiBreathe();
#endif

#ifdef HAVE_IMEC
// @@@ FIX closeAllBS is preferred but disrupts OneBox mapping
//    closeAllBS( false );
    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {
        int slot = T.getEnumSlot( is );
        if( !T.simprb.isSimSlot( slot ) )
            np_closeBS( slot );
    }
#endif

#if DBG
Log()<<"  np_closeBS all slots";
Log()<<"End Detect ]]";
guiBreathe();
#endif
    return ok;
}


void CimCfg::detect_API(
    QStringList     &slVers,
    ImProbeTable    &T )
{
#ifdef HAVE_IMEC
    int verMaj, verMin;

    np_getAPIVersion( &verMaj, &verMin );

    T.api = QString("%1.%2").arg( verMaj ).arg( verMin );
#else
    T.api = "0.0";
#endif
#if DBG
Log()<<"  np_getAPIVersion done";
guiBreathe();
#endif

    slVers.append( QString("API version %1").arg( T.api ) );
}


bool CimCfg::detect_Slots(
    QStringList     &slVers,
    ImProbeTable    &T )
{
#ifdef HAVE_IMEC
    QStringList     bs_bsc;
    firmware_Info   info;
    NP_ErrorCode    err;
    HardwareID      hID;
#endif

    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        // ---------
        // Slot type
        // ---------

#ifdef HAVE_IMEC
        basestationID   bs;
#endif
        ImSlotVers      V;
        int             slot = T.getEnumSlot( is );
#if DBG
Log()<<"start slot "<<slot;
guiBreathe();
#endif

        // ----------
        // Simulated?
        // ----------

        if( T.simprb.isSimSlot( slot ) ) {

            detect_simSlot( slVers, T, slot );
            continue;
        }

#ifdef HAVE_IMEC
        err = np_getDeviceInfo( slot, &bs );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getDeviceInfo(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            slVers.append(
                "Use 'Configure Slots' (above the table)"
                " to check your slot assignments." );
            return false;
        }

        T.setSlotType( slot, bs.platformid );
#endif
#if DBG
Log()<<"  np_getDeviceInfo done";
guiBreathe();
#endif

        // ------
        // OpenBS
        // ------

#ifdef HAVE_IMEC
        err = np_openBS( slot );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC openBS(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            slVers.append(
                "Check {slot,port} assignments, connections and power." );
            return false;
        }
#endif
#if DBG
Log()<<"  np_openBS done";
guiBreathe();
#endif

        // ----
        // BSFW
        // ----

#ifdef HAVE_IMEC
        err = np_bs_getFirmwareInfo( slot, &info );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC bs_getFirmwareInfo(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }

        V.bsfw = QString("%1.%2.%3")
                    .arg( info.major ).arg( info.minor ).arg( info.build );

        if( T.getSlotType( slot ) == NPPlatform_PXI ) {

            if( V.bsfw != VERS_IMEC_BS ) {
                bs_bsc.append(
                    QString("    - BS(slot %1) Requires: %2")
                    .arg( slot ).arg( VERS_IMEC_BS ) );
            }
        }
#else
        V.bsfw = "0.0.0";
#endif
#if DBG
Log()<<"  np_bs_getFirmwareInfo done";
guiBreathe();
#endif

        slVers.append(
            QString("BS(slot %1) firmware version %2")
            .arg( slot ).arg( V.bsfw ) );

        // -----
        // BSCPN
        // -----

#ifdef HAVE_IMEC
        err = np_getBSCHardwareID( slot, &hID );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getBSCHardwareID(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) ) );
            return false;
        }

#if 0
//@OBX367 May be fixed in 3.62
        //@OBX367 part number fudge
        hID.ProductNumber[HARDWAREID_PN_LEN-1] = 0;
        if( strlen( hID.ProductNumber ) >= 40 )
            strcpy( hID.ProductNumber, "NP2_QBSC_01" );
#endif

        V.bscpn = hID.ProductNumber;
#else
        V.bscpn = "sim";
#endif
#if DBG
Log()<<"  np_getBSCHardwareID done";
guiBreathe();
#endif

        slVers.append(
            QString("BSC(slot %1) part number %2")
            .arg( slot ).arg( V.bscpn ) );

        // -----
        // BSCSN
        // -----

#ifdef HAVE_IMEC
        V.bscsn = QString::number( hID.SerialNumber );
#else
        V.bscsn = "0";
#endif

        slVers.append(
            QString("BSC(slot %1) serial number %2")
            .arg( slot ).arg( V.bscsn ) );

        // -----
        // BSCHW
        // -----

#ifdef HAVE_IMEC
        V.bschw = QString("%1.%2").arg( hID.version_Major ).arg( hID.version_Minor );
#else
        V.bschw = "0.0";
#endif

        slVers.append(
            QString("BSC(slot %1) hardware version %2")
            .arg( slot ).arg( V.bschw ) );

        // -----
        // BSCFW
        // -----

#ifdef HAVE_IMEC
        if( T.getSlotType( slot ) == NPPlatform_PXI ) {

            err = np_bsc_getFirmwareInfo( slot, &info );

            if( err != SUCCESS ) {
                slVers.append(
                    QString("IMEC bsc_getFirmwareInfo(slot %1)%2")
                    .arg( slot ).arg( makeErrorString( err ) ) );
                return false;
            }

            V.bscfw = QString("%1.%2.%3")
                        .arg( info.major ).arg( info.minor )
                        .arg( info.build );

            if( V.bscfw != VERS_IMEC_BSC ) {
                bs_bsc.append(
                    QString("    - BSC(slot %1) Requires: %2")
                    .arg( slot ).arg( VERS_IMEC_BSC ) );
            }
        }
        else
            V.bscfw = "0.0.0";
#else
        V.bscfw = "0.0.0";
#endif
#if DBG
Log()<<"  np_bsc_getFirmwareInfo done";
guiBreathe();
#endif

        slVers.append(
            QString("BSC(slot %1) firmware version %2")
            .arg( slot ).arg( V.bscfw ) );

        // -------------
        // Add map entry
        // -------------

        T.slot2Vers[slot] = V;
    }

    // ------------------
    // Required firmware?
    // ------------------

#ifdef HAVE_IMEC
    if( bs_bsc.size() ) {

        slVers.append("");
        slVers.append("ERROR: Wrong IMEC Firmware Version(s) ---");
        foreach( const QString &s, bs_bsc )
            slVers.append( s );
        slVers.append("(1) Select menu item 'Tools/Update Imec PXIe Firmware'.");
        slVers.append("(2) Read the help for that dialog (click '?' in title bar).");
        slVers.append("(3) Required files are in the download package 'Firmware' folder.");
        return false;
    }
#endif

    return true;
}


void CimCfg::detect_simSlot(
    QStringList     &slVers,
    ImProbeTable    &T,
    int             slot )
{
    ImSlotVers  V;

    V.bsfw  = "0.0.0";
    V.bscpn = "sim";
    V.bscsn = "0";
    V.bschw = "0.0";
    V.bscfw = "0.0.0";

    slVers.append(
        QString("BS(slot %1) firmware version %2")
        .arg( slot ).arg( V.bsfw ) );
    slVers.append(
        QString("BSC(slot %1) part number %2")
        .arg( slot ).arg( V.bscpn ) );
    slVers.append(
        QString("BSC(slot %1) serial number %2")
        .arg( slot ).arg( V.bscsn ) );
    slVers.append(
        QString("BSC(slot %1) hardware version %2")
        .arg( slot ).arg( V.bschw ) );
    slVers.append(
        QString("BSC(slot %1) firmware version %2")
        .arg( slot ).arg( V.bscfw ) );

    T.slot2Vers[slot] = V;
}


bool CimCfg::detect_Probes(
    QStringList     &slVers,
    QStringList     &slBIST,
    QVector<int>    &vHSpsv,
    QVector<int>    &vHS20,
    ImProbeTable    &T,
    bool            doBIST )
{
#ifdef HAVE_IMEC
    NP_ErrorCode    err;
    HardwareID      hID;
#else
    Q_UNUSED( slVers )
    Q_UNUSED( slBIST )
    Q_UNUSED( vHSpsv )
    Q_UNUSED( vHS20 )
    Q_UNUSED( doBIST )
#endif

    for( int ip = 0, np = T.nLogProbes(); ip < np; ++ip ) {

        ImProbeDat  &P = T.mod_iProbe( ip );
#ifdef HAVE_IMEC
        bool        isHSpsv = false;
#endif
#if DBG
Log()<<"start probe "<<ip;
guiBreathe();
#endif

        // ----------
        // Simulated?
        // ----------

        if( T.simprb.isSimProbe( P.slot, P.port, P.dock ) ) {

            if( !detect_simProbe( slVers, T, P ) )
                return false;

            continue;
        }

        // ----
        // HSPN
        // ----

#ifdef HAVE_IMEC
        err = np_getHeadstageHardwareID( P.slot, P.port, &hID );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getHeadstageHardwareID(slot %1, port %2)%3")
                .arg( P.slot ).arg( P.port )
                .arg( makeErrorString( err ) ) );
            if( err == NO_LINK ) {
                slVers.append("");
                slVers.append("Error 44 DOES NOT mean the headstage is bad. Rather, there is a poor connection somewhere");
                slVers.append("on the path from the port to the probe flex. Top things to try:");
                slVers.append(" 1. Reconnect flex to headstage firmly and squarely; try several times.");
                slVers.append(" 2. Try different pairing of probe/headstage for better mechanical fit.");
                slVers.append(" 3. Try different 5-meter cable.");
            }
            else if( err == TIMEOUT ) {
                slVers.append("");
                slVers.append("Error 8 will occur if you detect with a headstage tester attached.");
                slVers.append("Use the headstage test dongle only with command: Tools/HST.");
            }
            return false;
        }

        // -------------------------------
        // Test for NHP 128-channel analog
        // -------------------------------

        QString prod(hID.ProductNumber);

        if( prod == "NPNH_HS_30" || prod == "NPNH_HS_31" ) {

            isHSpsv = true;

            if( vHSpsv.isEmpty() )
                vHSpsv.push_back( ip );
            else {

                const ImProbeDat    &Z = T.get_iProbe( vHSpsv[vHSpsv.size() - 1] );

                if( Z.slot != P.slot || Z.port != P.port )
                    vHSpsv.push_back( ip );
            }
        }

        P.hspn = prod;
#else
        P.hspn = "sim";
#endif
#if DBG
Log()<<"  np_getHeadstageHardwareID done";
guiBreathe();
#endif

        slVers.append(
            QString("HS(slot %1, port %2) part number %3")
            .arg( P.slot ).arg( P.port ).arg( P.hspn ) );

        // ----
        // HSSN
        // ----

#ifdef HAVE_IMEC
        P.hssn = hID.SerialNumber;
#else
        P.hssn = 0;
#endif

        // ----
        // HSHW
        // ----

#ifdef HAVE_IMEC
        P.hshw = QString("%1.%2").arg( hID.version_Major ).arg( hID.version_Minor );

        // --------------------------
        // HS20 (tests for no EEPROM)
        // --------------------------

        if( !P.hssn && (hID.version_Major == 0 || hID.version_Major == 1) && !hID.version_Minor ) {

            if( vHS20.isEmpty() )
                vHS20.push_back( ip );
            else {

                const ImProbeDat    &Z = T.get_iProbe( vHS20[vHS20.size() - 1] );

                if( Z.slot != P.slot || Z.port != P.port )
                    vHS20.push_back( ip );
            }
        }

#else
        P.hshw = "0.0";
#endif
#if DBG
Log()<<"  vHS20 check done";
guiBreathe();
#endif

        slVers.append(
            QString("HS(slot %1, port %2) hardware version %3")
            .arg( P.slot ).arg( P.port ).arg( P.hshw ) );

        // ----
        // HSFW
        // ----
        
        P.hsfw = "0.0.0";

        slVers.append(
            QString("HS(slot %1, port %2) firmware version %3")
            .arg( P.slot ).arg( P.port ).arg( P.hsfw ) );

        // ----
        // FXPN
        // ----

#ifdef HAVE_IMEC
        if( !isHSpsv ) {

            err = np_getFlexHardwareID( P.slot, P.port, P.dock, &hID );

            if( err != SUCCESS ) {
                slVers.append(
                    QString("IMEC getFlexHardwareID(slot %1, port %2, dock %3)%4")
                    .arg( P.slot ).arg( P.port ).arg( P.dock )
                    .arg( makeErrorString( err ) ) );
                if( err == TIMEOUT ) {
                    slVers.append("");
                    slVers.append("Error 8 will occur if you detect with a headstage tester attached.");
                    slVers.append("Use the headstage test dongle only with command: Tools/HST.");
                }
                return false;
            }

            P.fxpn = hID.ProductNumber;
        }
        else
            P.fxpn = "NHP128A";
#else
        P.fxpn = "sim";
#endif
#if DBG
Log()<<"  np_getFlexHardwareID done";
guiBreathe();
#endif

        slVers.append(
            QString("FX(slot %1, port %2, dock %3) part number %4")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxpn ) );

        // ----
        // FXSN
        // ----

#ifdef HAVE_IMEC
        if( !isHSpsv )
            P.fxsn = QString::number( hID.SerialNumber );
        else
            P.fxsn = "0";
#else
        P.fxsn = "0";
#endif

        slVers.append(
            QString("FX(slot %1, port %2, dock %3) serial number %4")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxsn ) );

        // ----
        // FXHW
        // ----

#ifdef HAVE_IMEC
        if( !isHSpsv )
            P.fxhw = QString("%1.%2").arg( hID.version_Major ).arg( hID.version_Minor );
        else
            P.fxhw = "0.0";
#else
        P.fxhw = "0.0";
#endif

        slVers.append(
            QString("FX(slot %1, port %2, dock %3) hardware version %4")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxhw ) );

        // --
        // PN
        // --

#ifdef HAVE_IMEC
        if( !isHSpsv ) {

            err = np_getProbeHardwareID( P.slot, P.port, P.dock, &hID );

            if( err != SUCCESS ) {
                slVers.append(
                    QString("IMEC getProbeHardwareID(slot %1, port %2, dock %3)%4")
                    .arg( P.slot ).arg( P.port ).arg( P.dock )
                    .arg( makeErrorString( err ) ) );
                return false;
            }

            P.pn = hID.ProductNumber;
        }
        else
            P.pn = "NP1200";
#else
        P.pn = "sim";
#endif
#if DBG
Log()<<"  np_getProbeHardwareID done";
guiBreathe();
#endif

        // --
        // SN
        // --

#ifdef HAVE_IMEC
        if( !isHSpsv )
            P.sn = hID.SerialNumber;
#else
        P.sn = 10 * P.slot + P.port;
#endif

        // ----
        // Type
        // ----

        if( !P.setProbeType() ) {
            slVers.append(
                QString("SpikeGLX setProbeType(slot %1, port %2, dock %3)"
                " error 'Probe part number %4 unsupported'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( P.pn ) );
            slVers.append("Try updating to a newer SpikeGLX/API version.");
            return false;
        }
#if DBG
Log()<<"  setProbeType done";
guiBreathe();
#endif

        // -----------
        // Wrong dock?
        // -----------

        if( P.dock > 1 && P.nHSDocks() == 1 ) {
            slVers.append(
                QString("SpikeGLX nHSDocks(slot %1, port %2, dock %3)"
                " error 'Only select dock 1 with this head stage'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock ) );
            return false;
        }

        // ---
        // CAL
        // ---

#ifdef HAVE_IMEC
        P.cal = testFixCalPath( P.calSN() );
#else
        P.cal = 1;
#endif
#if DBG
Log()<<"  calibPath done";
guiBreathe();
#endif

        // ------------------------
        // BIST SR (shift register)
        // ------------------------

#if DBG
Log()<<"BIST checks "<<doBIST;
guiBreathe();
#endif
#ifdef HAVE_IMEC
        if( !doBIST )
            continue;

        IMROTbl *R      = IMROTbl::alloc( P.pn );
        bool    testSR  = (R->nBanks() > 1);
        delete R;

        if( testSR ) {
            err = np_bistSR( P.slot, P.port, P.dock );

            if( err != SUCCESS ) {
                slBIST.append(
                    QString("slot %1, port %2, dock %3: Shift Register")
                    .arg( P.slot ).arg( P.port ).arg( P.dock ) );
            }
        }
#if DBG
Log()<<"  np_bistSR done";
guiBreathe();
#endif
#endif

        // ------------------------------
        // BIST PSB (parallel serial bus)
        // ------------------------------

#ifdef HAVE_IMEC
        err = np_bistPSB( P.slot, P.port, P.dock );

        if( err != SUCCESS ) {
            slBIST.append(
                QString("slot %1, port %2, dock %3: Parallel Serial Bus")
                .arg( P.slot ).arg( P.port ).arg( P.dock ) );
        }
#if DBG
Log()<<"  np_bistPSB done";
guiBreathe();
#endif
#endif
    }

    return true;
}


bool CimCfg::detect_simProbe(
    QStringList     &slVers,
    ImProbeTable    &T,
    ImProbeDat      &P )
{
    QString name = T.simprb.file( P.slot, P.port, P.dock );
    QFile   f( name + ".ap.bin" );

    if( !f.exists() ) {
        slVers.append(
            QString("Probe bin file(slot %1, port %2, dock %3)"
            " missing '%4'.")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( name + ".ap.bin" ) );
        return false;
    }

    name += ".ap.meta";
    f.setFileName( name );

    if( !f.exists() ) {
        slVers.append(
            QString("Probe metafile(slot %1, port %2, dock %3)"
            " missing '%4'.")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( name ) );
        return false;
    }

    KVParams    kvp;

    if( !kvp.fromMetaFile( name ) ) {
        slVers.append(
            QString("Sim probe(slot %1, port %2, dock %3)"
            " corrupt metafile '%4'.")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( name ) );
        return false;
    }

    P.hspn = kvp["imDatHs_pn"].toString();
    P.hssn = kvp["imDatHs_sn"].toULongLong();
    P.hshw = kvp["imDatHs_hw"].toString();

    if( kvp.find( "imDatHs_fw" ) != kvp.end() )
        P.hsfw = kvp["imDatHs_fw"].toString();
    else
        P.hsfw = "0.0.0";

    P.fxpn = kvp["imDatFx_pn"].toString();
    P.fxsn = kvp["imDatFx_sn"].toString();
    P.fxhw = kvp["imDatFx_hw"].toString();
    P.pn   = kvp["imDatPrb_pn"].toString();
    P.sn   = kvp["imDatPrb_sn"].toULongLong();
    P.cal  = kvp["imCalibrated"].toBool();

    if( !IMROTbl::pnToType( P.type, P.pn ) ) {
        slVers.append(
            QString("Sim probe(slot %1, port %2, dock %3)"
            " unsupported type '%4'.")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.pn ) );
        slVers.append("Try updating to a newer SpikeGLX/API version.");
        return false;
    }

    slVers.append(
        QString("HS(slot %1, port %2) part number %3")
        .arg( P.slot ).arg( P.port ).arg( P.hspn ) );
    slVers.append(
        QString("HS(slot %1, port %2) hardware version %3")
        .arg( P.slot ).arg( P.port ).arg( P.hshw ) );
    slVers.append(
        QString("HS(slot %1, port %2) firmware version %3")
        .arg( P.slot ).arg( P.port ).arg( P.hsfw ) );
    slVers.append(
        QString("FX(slot %1, port %2, dock %3) part number %4")
        .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxpn ) );
    slVers.append(
        QString("FX(slot %1, port %2, dock %3) serial number %4")
        .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxsn ) );
    slVers.append(
        QString("FX(slot %1, port %2, dock %3) hardware version %4")
        .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxhw ) );

    return true;
}


void CimCfg::detect_OneBoxes( ImProbeTable &T )
{
    for( int ip = 0, np = T.nLogOneBox(); ip < np; ++ip ) {

        ImProbeDat  &P = T.mod_iOneBox( ip );

        P.pn    = "obx";
        P.obsn  = T.slot2Vers[P.slot].bscsn.toInt();
    }
}


bool CimCfg::testFixCalPath( quint64 sn )
{
    QString path = QString("%1/%2").arg( calibPath() ).arg( sn );

// Folder already exists?

    if( QDir( path ).exists() )
        return true;

// Folder does not exist; do any files exist?

    QVector<QString>    flist;
    QDirIterator        it( calibPath() );
    QString             ssn = QString("%1").arg( sn );

    while( it.hasNext() ) {

        it.next();

        QFileInfo   fi = it.fileInfo();

        if( fi.isFile() && fi.fileName().startsWith( ssn ) )
            flist.push_back( fi.filePath() );
    }

    if( !flist.size() )
        return false;

// Create folder

    if( !QDir().mkpath( path ) )
        return false;

// Move files

    bool    ok = true;

    for( int i = 0, n = flist.size(); i < n; ++i ) {
        QFile   fi( flist[i] );
        ok &= fi.rename( QString("%1/%2")
                            .arg( path )
                            .arg( QFileInfo( fi ).fileName() ) );
    }

    return ok;
}


void CimCfg::forceProbeData(
    int             slot,
    int             port,
    int             dock,
    const QString   &sn,
    const QString   &pn )
{
#ifdef HAVE_IMEC
    if( SUCCESS == np_openBS( slot ) &&
        SUCCESS == np_openProbe( slot, port, dock ) ) {

        HardwareID      hID;
        NP_ErrorCode    err;

        err = np_getProbeHardwareID( slot, port, dock, &hID );

        if( err != SUCCESS ) {
            Error() <<
            QString("IMEC getProbeHardwareID(slot %1, port %2, dock %3)%4")
            .arg( slot ).arg( port ).arg( dock )
            .arg( makeErrorString( err ) );
            goto close;
        }

        if( !sn.isEmpty() )
            hID.SerialNumber = sn.toULongLong();

        if( !pn.isEmpty() ) {
            strncpy( hID.ProductNumber, STR2CHR( pn ), HARDWAREID_PN_LEN );
            hID.ProductNumber[HARDWAREID_PN_LEN - 1] = 0;
        }

        err = np_setProbeHardwareID( slot, port, dock, &hID );

        if( err != SUCCESS ) {
            Error() <<
            QString("IMEC setProbeHardwareID(slot %1, port %2, dock %3)%4")
            .arg( slot ).arg( port ).arg( dock )
            .arg( makeErrorString( err ) );
        }
    }

close:
    np_closeBS( slot );
#else
    Q_UNUSED( slot )
    Q_UNUSED( port )
    Q_UNUSED( dock )
    Q_UNUSED( sn )
    Q_UNUSED( pn )
#endif
}


