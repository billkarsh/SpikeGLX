
#include "Util.h"
#include "CimCfg.h"
#include "Subset.h"
#include "SignalBlocker.h"

#ifdef HAVE_IMEC
#include "IMEC/NeuropixAPI.h"
#else
#pragma message("*** Message to self: Building simulated IMEC version ***")
#endif

#include <QBitArray>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTableWidget>
#include <QThread>


/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Pattern: "chn bank refid apgn lfgn apflt"
//
QString IMRODesc::toString( int chn ) const
{
    return QString("%1 %2 %3 %4 %5 %6")
            .arg( chn )
            .arg( bank ).arg( refid )
            .arg( apgn ).arg( lfgn )
            .arg( apflt );
}


// Pattern: "chn bank refid apgn lfgn apflt"
//
// Note: The chn field is discarded.
//
IMRODesc IMRODesc::fromString( const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return IMRODesc(
            sl.at( 1 ).toInt(), sl.at( 2 ).toInt(),
            sl.at( 3 ).toInt(), sl.at( 4 ).toInt(),
            sl.at( 5 ).toInt() );
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl::fillDefault( int type )
{
    this->type = type;

    e.clear();
    e.resize( imType0Chan );
}


// Return true if two tables are same w.r.t banks.
//
bool IMROTbl::banksSame( const IMROTbl &rhs ) const
{
    int n = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].bank != rhs.e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn bank refid apgn lfgn apflt)()()...
//
QString IMROTbl::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = nChan();

    ts << "(" << type << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << "(" << e[i].toString( i ) << ")";

    return s;
}


// Pattern: (type,nchan)(chn bank refid apgn lfgn apflt)()()...
//
void IMROTbl::fromString( const QString &s )
{
    QStringList sl = s.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    type = hl[0].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( IMRODesc::fromString( sl[i] ) );
}


bool IMROTbl::loadFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( !fi.exists() ) {

        msg = QString("Can't find '%1'").arg( fi.fileName() );
        return false;
    }
    else if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        fromString( f.readAll() );

        if( type == 0 && nChan() == imType0Chan ) {

            msg = QString("Loaded (type=%1) file '%2'")
                    .arg( type )
                    .arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error reading '%1'").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening '%1'").arg( fi.fileName() );
        return false;
    }
}


bool IMROTbl::saveFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        int n = f.write( STR2CHR( toString() ) );

        if( n > 0 ) {

            msg = QString("Saved (type=%1) file '%2'")
                    .arg( type )
                    .arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error writing '%1'").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening '%1'").arg( fi.fileName() );
        return false;
    }
}

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static int i2gn[IMROTbl::imNGains]  = {50,125,250,500,1000,1500,2000,3000};


int IMROTbl::typeToNElec( int type )
{
    Q_UNUSED( type )

    return imType0Elec;
}


int IMROTbl::chToEl384( int ch, int bank )
{
    return (ch >= 0 ? (ch + 1) + bank*imType0Chan : 0);
}


bool IMROTbl::chIsRef( int ch )
{
    return ch == 191;
}


int IMROTbl::idxToGain( int idx )
{
    return (idx >= 0 && idx < 8 ? i2gn[idx] : i2gn[3]);
}


int IMROTbl::gainToIdx( int gain )
{
    switch( gain ) {
        case 50:
            return 0;
        case 125:
            return 1;
        case 250:
            return 2;
        case 500:
            return 3;
        case 1000:
            return 4;
        case 1500:
            return 5;
        case 2000:
            return 6;
        case 3000:
            return 7;
        default:
            break;
    }

    return 3;
}

/* ---------------------------------------------------------------- */
/* struct IMProbeDat ---------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimCfg::ImProbeDat::load( QSettings &S, int i )
{
    QString defRow =
        QString("slot:%1 port:%2 enab:1")
        .arg( 2 + i/4 ).arg( 1 + i%4 );

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

    enab = s.at( 1 ).toUInt();
}


void CimCfg::ImProbeDat::save( QSettings &S, int i ) const
{
    S.setValue(
        QString("row%1").arg( i ),
        QString("slot:%1 port:%2 enab:%3")
            .arg( slot ).arg( port ).arg( enab ) );
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
    foreach( const ImProbeDat &P, probes ) {

        if( P.slot == slot )
            return false;
    }

    for( int iport = 1; iport <= 4; ++iport )
        probes.push_back( ImProbeDat( slot, iport ) );

    qSort( probes.begin(), probes.end() );
    toGUI( T );

    return true;
}


// Return true if slot found (success).
//
bool CimCfg::ImProbeTable::rmvSlot( QTableWidget *T, int slot )
{
    bool    found = false;

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

// MS: Need real sn -> type extraction here
            P.type  = 0;
            P.ip    = nProbes++;

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


int CimCfg::ImProbeTable::nQualPortsThisSlot( int slot ) const
{
    int ports = 0;

    for( int i = 0, n = id2dat.size(); i < n; ++i ) {

        if( get_iProbe( i ).slot == slot )
            ++ports;
    }

    return ports;
}


void CimCfg::ImProbeTable::loadSettings()
{
// Load from ini file

    STDSETTINGS( settings, "improbetable" );
    settings.beginGroup( "ImPrbTabUserInput" );

    int np = settings.value( "nrows", 0 ).toInt();

    probes.resize( np );

    for( int i = 0; i < np; ++i )
        probes[i].load( settings, i );

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
        probes[i].save( settings, i );
}


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

        if( !(ti = T->item( i, 0 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 0, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        ti->setText( QString::number( P.slot ) );

        // ----
        // Port
        // ----

        if( !(ti = T->item( i, 1 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 1, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        ti->setText( QString::number( P.port ) );

        // ----
        // HSSN
        // ----

        if( !(ti = T->item( i, 2 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 2, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.hssn == UNSET64 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.hssn ) );

        // --
        // SN
        // --

        if( !(ti = T->item( i, 3 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 3, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.sn == UNSET64 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.sn ) );

        // --
        // PN
        // --

        if( !(ti = T->item( i, 4 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 4, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.pn.isEmpty() )
            ti->setText( "???" );
        else
            ti->setText( P.pn );

        // ----
        // Type
        // ----

        if( !(ti = T->item( i, 5 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 5, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.type == (quint16)-1 )
            ti->setText( "???" );
        else
            ti->setText( QString::number( P.type ) );

        // ----
        // Enab
        // ----

        if( !(ti = T->item( i, 6 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 6, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
        }

        ti->setCheckState( P.enab ? Qt::Checked : Qt::Unchecked );

        // ---
        // Cal
        // ---

        if( !(ti = T->item( i, 7 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 7, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        if( P.cal == (quint16)-1 )
            ti->setText( "???" );
        else
            ti->setText( P.cal > 0 ? "Y" : "N" );

        // --
        // Id
        // --

        if( !(ti = T->item( i, 8 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 8, ti );
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

        ti  = T->item( i, 0 );
        val = ti->text().toUInt( &ok );

        P.slot = (ok ? val : -1);

        // ----
        // Port
        // ----

        ti  = T->item( i, 1 );
        val = ti->text().toUInt( &ok );

        P.port = (ok ? val : -1);

        // ----
        // HSSN
        // ----

        ti  = T->item( i, 2 );
        v64 = ti->text().toULongLong( &ok );

        P.hssn = (ok ? v64 : UNSET64);

        // --
        // SN
        // --

        ti  = T->item( i, 3 );
        v64 = ti->text().toULongLong( &ok );

        P.sn = (ok ? v64 : UNSET64);

        // ----
        // HSFW
        // ----

        ti      = T->item( i, 4 );
        P.pn    = ti->text();

        if( P.pn.contains( "?" ) )
            P.pn.clear();

        // ----
        // Type
        // ----

        ti  = T->item( i, 5 );
        val = ti->text().toUInt( &ok );

        P.type = (ok ? val : -1);

        // ----
        // Enab
        // ----

        ti  = T->item( i, 6 );

        P.enab = (ti->checkState() == Qt::Checked);

        // ---
        // Cal
        // ---

        ti  = T->item( i, 7 );

        if( ti->text().contains( "?" ) )
            P.cal = -1;
        else
            P.cal = ti->text().contains( "Y" );

        // --
        // Id
        // --

        ti  = T->item( i, 8 );
        val = ti->text().toUInt( &ok );

        P.ip = (ok ? val : -1);
    }
}

/* ---------------------------------------------------------------- */
/* struct AttrEach ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Given input fields:
// - probe type parameter
//
// Derive:
// - imCumTypCnt[]
//
// IMPORTANT:
// This is only called if Imec is enabled on the Devices tab,
// and then only for defined probes. That's OK because clients
// shouldn't access these data for an invalid probe.
//
void CimCfg::AttrEach::deriveChanCounts( int type )
{
// --------------------------------
// First count each type separately
// --------------------------------

// MS: Analog and digital aux may be redefined in phase 3B2

    Q_UNUSED( type )

    imCumTypCnt[imTypeAP] = IMROTbl::imType0Chan;
    imCumTypCnt[imTypeLF] = imCumTypCnt[imTypeAP];
    imCumTypCnt[imTypeSY] = 1;

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


double CimCfg::AttrEach::chanGain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = imCumTypCnt[imTypeAP];

        if( ic < nAP )
            g = roTbl.e[ic].apgn;
        else if( ic < imCumTypCnt[imTypeLF] )
            g = roTbl.e[ic-nAP].lfgn;
        else
            return 1.0;

        if( g < 50.0 )
            g = 50.0;
    }

    return g;
}

/* ---------------------------------------------------------------- */
/* class CimCfg --------------------------------------------------- */
/* ---------------------------------------------------------------- */

int CimCfg::vToInt10( double v, int ip, int ic ) const
{
    return 1023 * all.range.voltsToUnity( v * each[ip].chanGain( ic ) ) - 512;
}


double CimCfg::int10ToV( int i10, int ip, int ic ) const
{
    return all.range.unityToVolts( (i10 + 512) / 1024.0 )
            / each[ip].chanGain( ic );
}


void CimCfg::loadSettings( QSettings &S )
{
// ---
// ALL
// ---

    S.beginGroup( "DAQ_Imec_All" );

//    all.range.rmin =
//    S.value( "imAiRangeMin", -0.6 ).toDouble();

//    all.range.rmax =
//    S.value( "imAiRangeMax", 0.6 ).toDouble();

    all.calPolicy =
    S.value( "imCalPolicy", 0 ).toInt();

    all.trgSource =
    S.value( "imTrgSource", 0 ).toInt();

    all.trgRising =
    S.value( "imTrgRising", true ).toBool();

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

    for( int ip = 0; ip < nProbes; ++ip ) {

        S.beginGroup( QString("Probe%1").arg( ip ) );

        AttrEach    &E = each[ip];

        E.srate     = S.value( "imSampRate", 30000.0 ).toDouble();
        E.imroFile  = S.value( "imRoFile", QString() ).toString();
        E.stdbyStr  = S.value( "imStdby", QString() ).toString();
        E.LEDEnable = S.value( "imLEDEnable", false ).toBool();

        E.sns.shankMapFile =
        S.value( "imSnsShankMapFile", QString() ).toString();

        E.sns.chanMapFile =
        S.value( "imSnsChanMapFile", QString() ).toString();

        E.sns.uiSaveChanStr =
        S.value( "ImSnsSaveChanSubset", "all" ).toString();

        S.endGroup();
    }

    S.endGroup();
}


void CimCfg::saveSettings( QSettings &S ) const
{
// ---
// ALL
// ---

    S.beginGroup( "DAQ_Imec_All" );

    S.setValue( "imAiRangeMin", all.range.rmin );
    S.setValue( "imAiRangeMax", all.range.rmax );
    S.setValue( "imCalPolicy", all.calPolicy );
    S.setValue( "imTrgSource", all.trgSource );
    S.setValue( "imTrgRising", all.trgRising );
    S.setValue( "imNProbes", nProbes );
    S.setValue( "imEnabled", enabled );

    S.endGroup();

// ----
// Each
// ----

    S.remove( "DAQ_Imec_Probes" );
    S.beginGroup( "DAQ_Imec_Probes" );

    for( int ip = 0; ip < nProbes; ++ip ) {

        S.beginGroup( QString("Probe%1").arg( ip ) );

        const AttrEach  &E = each[ip];

        S.setValue( "imSampRate", E.srate );
        S.setValue( "imRoFile", E.imroFile );
        S.setValue( "imStdby", E.stdbyStr );
        S.setValue( "imLEDEnable", E.LEDEnable );
        S.setValue( "imSnsShankMapFile", E.sns.shankMapFile );
        S.setValue( "imSnsChanMapFile", E.sns.chanMapFile );
        S.setValue( "imSnsSaveChanSubset", E.sns.uiSaveChanStr );

        S.endGroup();
    }

    S.endGroup();
}


void CimCfg::closeAllBS()
{
#ifdef HAVE_IMEC
    QString s = "Manually closing hardware; about 8 seconds...";

    Systray() << s;
    Log() << s;

    guiBreathe();
    guiBreathe();
    guiBreathe();

    for( int is = 2; is <= 8; ++is )
        openBS( is );

    QThread::msleep( 3000 );    // post openBS

    for( int is = 2; is <= 8; ++is )
        close( is, -1 );

    QThread::msleep( 2000 );    // post openBS

    for( int is = 2; is <= 8; ++is )
        closeBS( is );

    QThread::msleep( 3000 );    // post closeBS

    s = "Done closing hardware";
    Systray() << s;
    Log() << s;
#endif
}


bool CimCfg::detect( QStringList &sl, ImProbeTable &T )
{
// ---------
// Close all
// ---------

// @@@ FIX Verify best closing practice.
#ifdef HAVE_IMEC
    for( int is = 2; is <= 8; ++is )
        closeBS( is );

    QThread::msleep( 3000 );    // post closeBS
#endif

// ----------
// Local vars
// ----------

    int     nProbes;
    bool    ok = false;

    T.init();
    sl.clear();

    nProbes = T.buildEnabIndexTables();

#ifdef HAVE_IMEC
    char            s32[32];
    quint64         u64;
    NP_ErrorCode    err;
    quint16         build;
    quint8          maj8, min8;
#endif

// -------
// APIVers
// -------

#ifdef HAVE_IMEC
    getAPIVersion( &maj8, &min8 );

    T.api = QString("%1.%2").arg( maj8 ).arg( min8 );
#else
    T.api = "0.0";
#endif

    sl.append( QString("API version %1").arg( T.api ) );

// ---------------
// Loop over slots
// ---------------

    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        ImSlotVers  V;
        int         slot = T.getEnumSlot( is );

        // ------
        // OpenBS
        // ------

// @@@ FIX Don't know how long really need to wait after openBS.

#ifdef HAVE_IMEC
        err = openBS( slot );
        QThread::msleep( 2000 );    // post openBS

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC openBS( %1 ) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            sl.append(
                "Check {slot,port} assignments, connections and power." );
            goto exit;
        }
#endif

        // ----
        // BSFW
        // ----

#ifdef HAVE_IMEC
        err = getBSBootVersion( slot, &maj8, &min8, &build );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC getBSBootVersion(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bsfw = QString("%1.%2.%3").arg( maj8 ).arg( min8 ).arg( build );
#else
        V.bsfw = "0.0.0";
#endif

        sl.append(
            QString("BS(slot %1) firmware version %2")
            .arg( slot ).arg( V.bsfw ) );

        // -----
        // BSCPN
        // -----

#ifdef HAVE_IMEC
        err = readBSCPN( slot, s32, 31 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC readBSCPN(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bscpn = s32;
#else
        V.bscpn = "sim";
#endif

        sl.append(
            QString("BSC(slot %1) part number %2")
            .arg( slot ).arg( V.bscpn ) );

        // -----
        // BSCSN
        // -----

#ifdef HAVE_IMEC
        err = readBSCSN( slot, &u64 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC readBSCSN(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bscsn = QString::number( u64 );
#else
        V.bscsn = "0";
#endif

        sl.append(
            QString("BSC(slot %1) serial number %2")
            .arg( slot ).arg( V.bscsn ) );

        // -----
        // BSCHW
        // -----

#ifdef HAVE_IMEC
        err = getBSCVersion( slot, &maj8, &min8 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC getBSCVersion(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bschw = QString("%1.%2").arg( maj8 ).arg( min8 );
#else
        V.bschw = "0.0";
#endif

        sl.append(
            QString("BSC(slot %1) hardware version %2")
            .arg( slot ).arg( V.bschw ) );

        // -----
        // BSCFW
        // -----

#ifdef HAVE_IMEC
        err = getBSCBootVersion( slot, &maj8, &min8, &build );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC getBSCBootVersion(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        V.bscfw = QString("%1.%2.%3").arg( maj8 ).arg( min8 ).arg( build );
#else
        V.bscfw = "0.0.0";
#endif

        sl.append(
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
        err = openProbe( P.slot, P.port );
        QThread::msleep( 10 );  // post openProbe

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC openProbe(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }
#endif

        // ----
        // HSPN
        // ----

#ifdef HAVE_IMEC
        err = readHSPN( P.slot, P.port, s32, 31 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC readHSPN(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.hspn = s32;
#else
        P.hspn = "sim";
#endif

        sl.append(
            QString("HS(slot %1, port %2) part number %3")
            .arg( P.slot ).arg( P.port ).arg( P.hspn ) );

        // ----
        // HSSN
        // ----

#ifdef HAVE_IMEC
        err = readHSSN( P.slot, P.port, &u64 );

        if( err != SUCCESS ) {
            sl.append(
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
        err = getHSVersion( P.slot, P.port, &maj8, &min8 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC getHSVersion(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.hsfw = QString("%1.%2").arg( maj8 ).arg( min8 );
#else
        P.hsfw = "0.0";
#endif

        sl.append(
            QString("HS(slot %1, port %2) firmware version %3")
            .arg( P.slot ).arg( P.port ).arg( P.hsfw ) );

        // ----
        // FXPN
        // ----

#ifdef HAVE_IMEC
        err = readFlexPN( P.slot, P.port, s32, 31 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC readFlexPN(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.fxpn = s32;
#else
        P.fxpn = "sim";
#endif

        sl.append(
            QString("FX(slot %1, port %2) part number %3")
            .arg( P.slot ).arg( P.port ).arg( P.fxpn ) );

        // ----
        // FXHW
        // ----

#ifdef HAVE_IMEC
        err = getFlexVersion( P.slot, P.port, &maj8, &min8 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC getFlexVersion(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.fxhw = QString("%1.%2").arg( maj8 ).arg( min8 );
#else
        P.fxhw = "0.0";
#endif

        sl.append(
            QString("FX(slot %1, port %2) hardware version %3")
            .arg( P.slot ).arg( P.port ).arg( P.fxhw ) );

        // --
        // PN
        // --

#ifdef HAVE_IMEC
        err = readProbePN( P.slot, P.port, s32, 31 );

        if( err == SUCCESS )
            P.pn = s32;
// @@@ FIX What happens with dummy probe???
// @@@ FIX Dummy and others failing readProbePN() fail more generally.
//        else if( err == EEPROM_CONTENT_ERROR )
//            P.pn = "NP2_PRB_X0";
        else {
            sl.append(
                QString("IMEC readProbePN(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }
#else
        P.pn = "sim";
#endif

        // --
        // SN
        // --

#ifdef HAVE_IMEC
        err = readId( P.slot, P.port, &u64 );

        if( err != SUCCESS ) {
            sl.append(
                QString("IMEC readId(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }

        P.sn = u64;
#else
        P.sn = 0;
#endif

        // ---
        // CAL
        // ---

#ifdef HAVE_IMEC
        QString path = QString("%1/ImecProbeData/%2")
                        .arg( appPath() ).arg( P.sn );

        P.cal = QDir( path ).exists();
#else
        P.cal = 1;
#endif
    }

// ----
// Exit
// ----

    ok = true;

// @@@ FIX Verify best closing practice.
#ifdef HAVE_IMEC
exit:
    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        int slot = T.getEnumSlot( is );

//        close( slot, -1 );
        closeBS( slot );
    }

    QThread::msleep( 2000 );    // post closeBS
#endif

    return ok;
}


