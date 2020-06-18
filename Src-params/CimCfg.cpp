
#include "Util.h"
#include "CimCfg.h"
#include "Subset.h"
#include "SignalBlocker.h"

#ifdef HAVE_IMEC
#include "IMEC/NeuropixAPI.h"
#include "IMEC/NeuropixAPI_configuration.h"
#else
#pragma message("*** Message to self: Building simulated IMEC version ***")
#endif

#include <QBitArray>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTableWidget>


/* ---------------------------------------------------------------- */
/* struct IMProbeDat ---------------------------------------------- */
/* ---------------------------------------------------------------- */

// How type value, per se, is consulted in the code:
// - Supported probe? (this function).
// - IMROTbl::alloc().
// - IMROTbl::defaultString().
// - IMROEditorLaunch().
//
// Type codes:
//  0:   NP 1.0 SS 960
// 21:   NP 2.0 SS scrambled 1280
// 24:   NP 2.0 MS 1280
//
// Return true if supported.
//
bool CimCfg::ImProbeDat::setProbeType()
{
    type = 0;   // NP 1.0

    if( pn.startsWith( "PRB2_1" ) )
        type = 21;
    else if( pn.startsWith( "PRB2_4" ) )
        type = 24;

    return true;
}


int CimCfg::ImProbeDat::nHSDocks()
{
    if( type == 21 || type == 24 )
        return 2;

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

    id2dat.clear();
    slotsUsed.clear();
    slot2zIdx.clear();

    api.clear();
    slot2Vers.clear();
}


// Return true if slot unique (success).
//
bool CimCfg::ImProbeTable::addSlot( QTableWidget *T, int slot )
{
    fromGUI( T );

    foreach( const ImProbeDat &P, probes ) {

        if( P.slot == slot )
            return false;
    }

    for( int iport = 1; iport <= 4; ++iport ) {
        for( int idock = 1; idock <= 2; ++idock )
            probes.push_back( ImProbeDat( slot, iport, idock ) );
    }

    qSort( probes.begin(), probes.end() );
    toGUI( T );

    return true;
}


// Return true if slot found (success).
//
bool CimCfg::ImProbeTable::rmvSlot( QTableWidget *T, int slot )
{
    bool    found = false;

    fromGUI( T );

    for( int ip = probes.size() - 1; ip >= 0; --ip ) {

        if( probes[ip].slot == slot ) {
            probes.remove( ip );
            found = true;
        }
    }

    if( found )
        toGUI( T );

    return found;
}


// Build {id2dat[], slot[]} index arrays based on:
// probe enabled.
//
// Return nProbes.
//
int CimCfg::ImProbeTable::buildEnabIndexTables()
{
    QMap<int,int>   mapSlots;   // ordered keys, arbitrary value

    id2dat.clear();
    slotsUsed.clear();
    slot2zIdx.clear();

    for( int i = 0, n = probes.size(); i < n; ++i ) {

        CimCfg::ImProbeDat  &P = probes[i];

        if( P.enab ) {

            id2dat.push_back( i );
            mapSlots[P.slot] = 0;
        }
    }

    foreach( int key, mapSlots.keys() ) {

        slot2zIdx[key] = slotsUsed.size();
        slotsUsed.push_back( key );
    }

    return id2dat.size();
}


// Build {id2dat[], slot[]} index arrays based on:
// probe enabled AND version data valid.
//
// Return nProbes.
//
int CimCfg::ImProbeTable::buildQualIndexTables()
{
    QMap<int,int>   mapSlots;   // ordered keys, arbitrary value
    int             nProbes = 0;

    id2dat.clear();
    slotsUsed.clear();
    slot2zIdx.clear();

    for( int i = 0, n = probes.size(); i < n; ++i ) {

        CimCfg::ImProbeDat  &P = probes[i];

        if( P.enab && P.hssn != UNSET64 && P.sn != UNSET64 ) {

            P.ip = nProbes++;

            id2dat.push_back( i );
            mapSlots[P.slot] = 0;
        }
    }

    foreach( int key, mapSlots.keys() ) {

        slot2zIdx[key] = slotsUsed.size();
        slotsUsed.push_back( key );
    }

    return nProbes;
}


bool CimCfg::ImProbeTable::haveQualCalFiles() const
{
    for( int i = 0, n = probes.size(); i < n; ++i ) {

        const CimCfg::ImProbeDat    &P = probes[i];

        if( P.enab && P.hssn != UNSET64 && P.sn != UNSET64 ) {

            if( P.cal != 1 )
                return false;
        }
    }

    return true;
}


int CimCfg::ImProbeTable::nQualDocksThisSlot( int slot ) const
{
    int docks = 0;

    for( int i = 0, n = id2dat.size(); i < n; ++i ) {

        if( get_iProbe( i ).slot == slot )
            ++docks;
    }

    return docks;
}


double CimCfg::ImProbeTable::getSRate( int i ) const
{
    if( i < id2dat.size() )
        return srateTable.value( probes[id2dat[i]].hssn, 30000.0 );

    return 30000.0;
}


// Calibrated probe (really, HS) sample rates.
// Maintained in _Calibration folder.
//
void CimCfg::ImProbeTable::loadSRateTable()
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_imec" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedHeadStages" );

    srateTable.clear();

    foreach( const QString &hssn, settings.childKeys() ) {

        srateTable[hssn.toULongLong()] =
            settings.value( hssn, 30000.0 ).toDouble();
    }
}


// Calibrated probe (really, HS) sample rates.
// Maintained in _Calibration folder.
//
void CimCfg::ImProbeTable::saveSRateTable() const
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_imec" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedHeadStages" );

    QMap<quint64,double>::const_iterator    it;

    for( it = srateTable.begin(); it != srateTable.end(); ++it )
        settings.setValue( QString("%1").arg( it.key() ), it.value() );
}


void CimCfg::ImProbeTable::loadSettings()
{
// Load from ini file

    STDSETTINGS( settings, "improbetable" );
    settings.beginGroup( "ImPrbTabUserInput" );

    int np = settings.value( "nrows", 0 ).toInt();

    probes.resize( np );

    for( int i = 0; i < np; ++i )
        probes[i].loadSettings( settings, i );

    qSort( probes.begin(), probes.end() );
}


void CimCfg::ImProbeTable::saveSettings() const
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

        ti->setText( QString::number( P.port ) );

        // ----
        // HSSN
        // ----

        if( !(ti = T->item( i, TBL_HSSN )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_HSSN, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.hssn == UNSET64 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.hssn ) );

        // ----
        // Dock
        // ----

        if( !(ti = T->item( i, TBL_DOCK )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_DOCK, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        ti->setText( QString::number( P.dock ) );

        // --
        // SN
        // --

        if( !(ti = T->item( i, TBL_PRSN )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_PRSN, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.sn == UNSET64 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.sn ) );

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

        if( P.type == (quint16)-1 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.type ) );

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

        if( P.cal == (quint16)-1 )
            ti->setText( "???" );
        else
            ti->setText( P.cal > 0 ? "Y" : "N" );

        // --
        // Id
        // --

        if( !(ti = T->item( i, TBL_ID )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_ID, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.ip == (quint16)-1 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.ip ) );
    }
}


void CimCfg::ImProbeTable::fromGUI( QTableWidget *T )
{
    int np = T->rowCount();

    probes.resize( np );

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

        ti  = T->item( i, TBL_PORT );
        val = ti->text().toUInt( &ok );

        P.port = (ok ? val : -1);

        // ----
        // HSSN
        // ----

        ti  = T->item( i, TBL_HSSN );
        v64 = ti->text().toULongLong( &ok );

        P.hssn = (ok ? v64 : UNSET64);

        // ----
        // Dock
        // ----

        ti  = T->item( i, TBL_DOCK );
        val = ti->text().toUInt( &ok );

        P.dock = (ok ? val : -1);

        // --
        // SN
        // --

        ti  = T->item( i, TBL_PRSN );
        v64 = ti->text().toULongLong( &ok );

        P.sn = (ok ? v64 : UNSET64);

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

        if( ti->text().contains( "?" ) )
            P.cal = -1;
        else
            P.cal = ti->text().contains( "Y" );

        // --
        // Id
        // --

        ti  = T->item( i, TBL_ID );
        val = ti->text().toUInt( &ok );

        P.ip = (ok ? val : -1);
    }
}


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
    SignalBlocker   b0(T);

    QString         slot = T->item( row, TBL_SLOT )->text(),
                    dock = T->item( row, TBL_DOCK )->text();
    Qt::CheckState  enab = T->item( row, TBL_ENAB )->checkState();

    for( int i = 0, np = T->rowCount(); i < np; ++i ) {

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
void CimCfg::ImProbeTable::whosChecked( QString &s, QTableWidget *T ) const
{
// Gather counts

    std::vector<int>    slotSum( 1 + 16, 0 );   // 1-based slotID -> count

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

            if( ok )
                ++slotSum[val];
        }
    }

// Compose string

    s = "(slot,N):";

    for( int is = 2, ns = slotSum.size(); is < ns; ++is ) {

        if( slotSum[is] )
            s += QString("  (%1,%2)").arg( is ).arg( slotSum[is] );
    }
}

/* ---------------------------------------------------------------- */
/* struct AttrAll ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimCfg::AttrAll::loadSettings( QSettings &S )
{
    calPolicy       = S.value( "imCalPolicy", 0 ).toInt();
    trgSource       = S.value( "imTrgSource", 0 ).toInt();
    trgRising       = S.value( "imTrgRising", true ).toBool();
    bistAtDetect    = S.value( "imBistAtDetect", true ).toBool();
}


void CimCfg::AttrAll::saveSettings( QSettings &S ) const
{
    S.setValue( "imCalPolicy", calPolicy );
    S.setValue( "imTrgSource", trgSource );
    S.setValue( "imTrgRising", bistAtDetect );
    S.setValue( "imBistAtDetect", bistAtDetect );
}

/* ---------------------------------------------------------------- */
/* struct AttrEach ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CimCfg::AttrEach::loadSettings( QSettings &S, int ip )
{
    S.beginGroup( QString("Probe%1").arg( ip ) );

    imroFile            = S.value( "imRoFile", QString() ).toString();
    stdbyStr            = S.value( "imStdby", QString() ).toString();
    LEDEnable           = S.value( "imLEDEnable", false ).toBool();
    sns.shankMapFile    = S.value( "imSnsShankMapFile", QString() ).toString();
    sns.chanMapFile     = S.value( "imSnsChanMapFile", QString() ).toString();
    sns.uiSaveChanStr   = S.value( "ImSnsSaveChanSubset", "all" ).toString();

    S.endGroup();
}


void CimCfg::AttrEach::saveSettings( QSettings &S, int ip ) const
{
    S.beginGroup( QString("Probe%1").arg( ip ) );

    S.setValue( "imRoFile", imroFile );
    S.setValue( "imStdby", stdbyStr );
    S.setValue( "imLEDEnable", LEDEnable );
    S.setValue( "imSnsShankMapFile", sns.shankMapFile );
    S.setValue( "imSnsChanMapFile", sns.chanMapFile );
    S.setValue( "imSnsSaveChanSubset", sns.uiSaveChanStr );

    S.endGroup();
}


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
void CimCfg::AttrEach::deriveChanCounts()
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
bool CimCfg::AttrEach::deriveStdbyBits( QString &err, int nAP )
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
                    "Imec off-channel string includes channels"
                    " higher than maximum [%1].")
                    .arg( nAP - 1 );
            return false;
        }

        // in case smaller
        stdbyBits.resize( nAP );
    }
    else {
        err = "Imec off-channel string has incorrect format.";
        return false;
    }

    return true;
}


void CimCfg::AttrEach::justAPBits(
    QBitArray       &apBits,
    const QBitArray &saveBits ) const
{
    apBits = saveBits;
    apBits.fill( 0, imCumTypCnt[imTypeAP], imCumTypCnt[imTypeLF] );
}


void CimCfg::AttrEach::justLFBits(
    QBitArray       &lfBits,
    const QBitArray &saveBits ) const
{
    lfBits = saveBits;
    lfBits.fill( 0, 0, imCumTypCnt[imTypeAP] );
}


void CimCfg::AttrEach::apSaveBits( QBitArray &apBits ) const
{
    justAPBits( apBits, sns.saveBits );
}


void CimCfg::AttrEach::lfSaveBits( QBitArray &lfBits ) const
{
    justLFBits( lfBits, sns.saveBits );
}


int CimCfg::AttrEach::apSaveChanCount() const
{
    QBitArray   apBits;
    apSaveBits( apBits );
    return apBits.count( true );
}


int CimCfg::AttrEach::lfSaveChanCount() const
{
    QBitArray   lfBits;
    lfSaveBits( lfBits );
    return lfBits.count( true );
}


bool CimCfg::AttrEach::lfIsSaving() const
{
    QBitArray   lfBits;
    lfSaveBits( lfBits );
    lfBits.clearBit( imCumTypCnt[imSumAll] - 1 );
    return lfBits.count( true );
}


double CimCfg::AttrEach::chanGain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = imCumTypCnt[CimCfg::imTypeAP],
            nNu = imCumTypCnt[CimCfg::imTypeLF];

        if( ic < nAP )
            g = roTbl->apGain( ic );
        else if( ic < nNu || nNu == nAP )
            g = roTbl->lfGain( ic - nAP );
        else
            return 1.0;

        if( g < 50.0 )
            g = 50.0;
    }

    return g;
}


int CimCfg::AttrEach::vToInt( double v, int ic ) const
{
    return (roTbl->maxInt() - 1) * v * chanGain( ic ) / roTbl->maxVolts();
}


double CimCfg::AttrEach::intToV( int i, int ic ) const
{
    return roTbl->maxVolts() * i / (roTbl->maxInt() * chanGain( ic ));
}

/* ---------------------------------------------------------------- */
/* class CimCfg --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimCfg::loadSettings( QSettings &S )
{
// ---
// ALL
// ---

    S.beginGroup( "DAQ_Imec_All" );

    all.loadSettings( S );

    nProbes =
    S.value( "imNProbes", 1 ).toInt();

    enabled =
    S.value( "imEnabled", false ).toBool();

    S.endGroup();

// ----
// Each
// ----

    S.beginGroup( "DAQ_Imec_Probes" );

    if( nProbes < 1 )
        nProbes = 1;

    each.resize( nProbes );

    for( int ip = 0; ip < nProbes; ++ip )
        each[ip].loadSettings( S, ip );

    S.endGroup();
}


void CimCfg::saveSettings( QSettings &S ) const
{
// ---
// ALL
// ---

    S.beginGroup( "DAQ_Imec_All" );

    all.saveSettings( S );

    S.setValue( "imNProbes", nProbes );
    S.setValue( "imEnabled", enabled );

    S.endGroup();

// ----
// Each
// ----

    S.remove( "DAQ_Imec_Probes" );
    S.beginGroup( "DAQ_Imec_Probes" );

    for( int ip = 0; ip < nProbes; ++ip )
        each[ip].saveSettings( S, ip );

    S.endGroup();
}


void CimCfg::closeAllBS()
{
#ifdef HAVE_IMEC
    QString s = "Manually closing hardware; about 4 seconds...";

    Systray() << s;
    Log() << s;

    guiBreathe();
    guiBreathe();
    guiBreathe();

    for( int is = 2; is <= 8; ++is ) {

        closeBS( is );
        openBS( is );
        closeBS( is );
    }

    s = "Done closing hardware";
    Systray() << s;
    Log() << s;
#endif
}


bool CimCfg::detect(
        QStringList     &slVers,
        QStringList     &slBIST,
        QVector<int>    &vHS20,
        ImProbeTable    &T,
        bool            doBIST )
{
// ---------
// Close all
// ---------

#ifdef HAVE_IMEC
    for( int is = 2; is <= 8; ++is )
        closeBS( is );
#endif

// ----------
// Local vars
// ----------

    int     nProbes;
    bool    ok = false;

    T.init();
    slVers.clear();
    slBIST.clear();
    vHS20.clear();

    nProbes = T.buildEnabIndexTables();

#ifdef HAVE_IMEC
    char            strPN[64];
#define StrPNWid    (sizeof(strPN) - 1)
    quint64         u64;
    NP_ErrorCode    err;
    quint16         build;
    quint8          verMaj, verMin;
#endif

// -------
// APIVers
// -------

#ifdef HAVE_IMEC
    getAPIVersion( &verMaj, &verMin );

    T.api = QString("%1.%2").arg( verMaj ).arg( verMin );
#else
    T.api = "0.0";
#endif

    slVers.append( QString("API version %1").arg( T.api ) );

// ---------------
// Loop over slots
// ---------------

    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        ImSlotVers  V;
        int         slot = T.getEnumSlot( is );

        // ------
        // OpenBS
        // ------

#ifdef HAVE_IMEC
        err = openBS( slot );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC openBS( %1 ) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            slVers.append(
                "Check {slot,port} assignments, connections and power." );
            goto exit;
        }
#endif

        // ----
        // BSFW
        // ----

#ifdef HAVE_IMEC
        err = getBSBootVersion( slot, &verMaj, &verMin, &build );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getBSBootVersion(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bsfw = QString("%1.%2.%3").arg( verMaj ).arg( verMin ).arg( build );
#else
        V.bsfw = "0.0.0";
#endif

        slVers.append(
            QString("BS(slot %1) firmware version %2")
            .arg( slot ).arg( V.bsfw ) );

        // -----
        // BSCPN
        // -----

#ifdef HAVE_IMEC
        err = readBSCPN( slot, strPN, StrPNWid );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC readBSCPN(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bscpn = strPN;
#else
        V.bscpn = "sim";
#endif

        slVers.append(
            QString("BSC(slot %1) part number %2")
            .arg( slot ).arg( V.bscpn ) );

        // -----
        // BSCSN
        // -----

#ifdef HAVE_IMEC
        err = readBSCSN( slot, &u64 );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC readBSCSN(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bscsn = QString::number( u64 );
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
        err = getBSCVersion( slot, &verMaj, &verMin );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getBSCVersion(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bschw = QString("%1.%2").arg( verMaj ).arg( verMin );
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
        err = getBSCBootVersion( slot, &verMaj, &verMin, &build );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getBSCBootVersion(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bscfw = QString("%1.%2.%3").arg( verMaj ).arg( verMin ).arg( build );
#else
        V.bscfw = "0.0.0";
#endif

        slVers.append(
            QString("BSC(slot %1) firmware version %2")
            .arg( slot ).arg( V.bscfw ) );

        // -------------
        // Add map entry
        // -------------

        T.slot2Vers[slot] = V;
    }

// --------------------
// Individual HS/probes
// --------------------

    for( int ip = 0; ip < nProbes; ++ip ) {

        ImProbeDat  &P = T.mod_iProbe( ip );

        // --------------------
        // Connect to that port
        // --------------------

#ifdef HAVE_IMEC
        err = openProbe( P.slot, P.port, P.dock );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC openProbe(slot %1, port %2, dock %3) error %4 '%5'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }
#endif

        // ----
        // HSPN
        // ----

#ifdef HAVE_IMEC
        err = readHSPN( P.slot, P.port, strPN, StrPNWid );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC readHSPN(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.hspn = strPN;
#else
        P.hspn = "sim";
#endif

        slVers.append(
            QString("HS(slot %1, port %2) part number %3")
            .arg( P.slot ).arg( P.port ).arg( P.hspn ) );

        // ----
        // HSSN
        // ----

#ifdef HAVE_IMEC
        err = readHSSN( P.slot, P.port, &u64 );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC readHSSN(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.hssn = u64;
#else
        P.hssn = 0;
#endif

        // ----
        // HSFW
        // ----

#ifdef HAVE_IMEC
        err = getHSVersion( P.slot, P.port, &verMaj, &verMin );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getHSVersion(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.hsfw = QString("%1.%2").arg( verMaj ).arg( verMin );

#else
        P.hsfw = "0.0";
#endif

        slVers.append(
            QString("HS(slot %1, port %2) firmware version %3")
            .arg( P.slot ).arg( P.port ).arg( P.hsfw ) );

        // ----
        // FXPN
        // ----

#ifdef HAVE_IMEC
        err = readFlexPN( P.slot, P.port, P.dock, strPN, StrPNWid );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC readFlexPN(slot %1, port %2, dock %3) error %4 '%5'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.fxpn = strPN;
#else
        P.fxpn = "sim";
#endif

        slVers.append(
            QString("FX(slot %1, port %2, dock %3) part number %4")
            .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxpn ) );

        // ----
        // FXHW
        // ----

#ifdef HAVE_IMEC
        err = getFlexVersion( P.slot, P.port, P.dock, &verMaj, &verMin );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC getFlexVersion(slot %1, port %2, dock %3) error %4 '%5'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.fxhw = QString("%1.%2").arg( verMaj ).arg( verMin );
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
        err = readProbePN( P.slot, P.port, P.dock, strPN, StrPNWid );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC readProbePN(slot %1, port %2, dock %3) error %4 '%5'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.pn = strPN;
#else
        P.pn = "sim";
#endif

        // --
        // SN
        // --

#ifdef HAVE_IMEC
        err = readProbeSN( P.slot, P.port, P.dock, &u64 );

        if( err != SUCCESS ) {
            slVers.append(
                QString("IMEC readProbeSN(slot %1, port %2, dock %3) error %4 '%5'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.sn = u64;
#else
        P.sn = 0;
#endif

        // ----
        // Type
        // ----

        if( !P.setProbeType() ) {
            slVers.append(
                QString("SpikeGLX setProbeType(slot %1, port %2, dock %3)"
                " error 'Probe type %4 unsupported'.")
                .arg( P.slot ).arg( P.port ).arg( P.dock )
                .arg( P.sn ) );
            goto exit;
        }

        // ----
        // HS20
        // ----

// @@@ FIX v2.0 Consider basing test on HS SN rather than probe.

        if( P.type == 21 || P.type == 24 ) {

            if( vHS20.isEmpty() )
                vHS20.push_back( ip );
            else {

                ImProbeDat  &Z = T.mod_iProbe( vHS20[vHS20.size() - 1] );

                if( Z.slot != P.slot || Z.port != P.port )
                    vHS20.push_back( ip );
            }
        }

        // ---
        // CAL
        // ---

#ifdef HAVE_IMEC
        QString path = QString("%1/%2").arg( calibPath() ).arg( P.sn );

        P.cal = QDir( path ).exists();
#else
        P.cal = 1;
#endif

        // ------------------------
        // BIST SR (shift register)
        // ------------------------

#ifdef HAVE_IMEC
        if( !doBIST )
            continue;

        err = bistSR( P.slot, P.port, P.dock );

        if( err != SUCCESS ) {
            slBIST.append(
                QString("slot %1, port %2, dock %3: Shift Register")
                .arg( P.slot ).arg( P.port ).arg( P.dock ) );
        }
#endif

        // ------------------------------
        // BIST PSB (parallel serial bus)
        // ------------------------------

#ifdef HAVE_IMEC
        err = bistPSB( P.slot, P.port, P.dock );

        if( err != SUCCESS ) {
            slBIST.append(
                QString("slot %1, port %2, dock %3: Parallel Serial Bus")
                .arg( P.slot ).arg( P.port ).arg( P.dock ) );
        }

        closeBS( P.slot );
#endif
    }

// ----
// Exit
// ----

    ok = true;

exit:
#ifdef HAVE_IMEC
    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is )
        closeBS( T.getEnumSlot( is ) );
#endif

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
    if( SUCCESS == openBS( slot ) &&
        SUCCESS == openProbe( slot, port, dock ) ) {

        HardwareID      D;
        NP_ErrorCode    err;

        err = getProbeHardwareID( slot, port, dock, &D );

        if( err != SUCCESS ) {
            Error() <<
            QString("IMEC getProbeHardwareID(slot %1, port %2, dock %3) error %4 '%5'.")
            .arg( slot ).arg( port ).arg( dock )
            .arg( err ).arg( np_GetErrorMessage( err ) );
            goto close;
        }

        if( !sn.isEmpty() )
            D.SerialNumber = sn.toULongLong();

        if( !pn.isEmpty() ) {
            strncpy( D.ProductNumber, STR2CHR( pn ), HARDWAREID_PN_LEN );
            D.ProductNumber[HARDWAREID_PN_LEN - 1] = 0;
        }

        err = setProbeHardwareID( slot, port, dock, &D );

        if( err != SUCCESS ) {
            Error() <<
            QString("IMEC setProbeHardwareID(slot %1, port %2, dock %3) error %4 '%5'.")
            .arg( slot ).arg( port ).arg( dock )
            .arg( err ).arg( np_GetErrorMessage( err ) );
        }
    }

close:
    closeBS( slot );
#else
    Q_UNUSED( slot )
    Q_UNUSED( port )
    Q_UNUSED( dock )
    Q_UNUSED( sn )
    Q_UNUSED( pn )
#endif
}


