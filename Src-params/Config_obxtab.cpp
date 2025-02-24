
#include "ui_ObxTab.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Config_obxtab.h"
#include "ChanMapCtl.h"
#include "Subset.h"
#include "SignalBlocker.h"

#include <QComboBox>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#define TBL_SN      0
#define TBL_RATE    1
#define TBL_XA      2
#define TBL_XD      3
#define TBL_V       4
#define TBL_AO      5
#define TBL_CHAN    6
#define TBL_SAVE    7

static const char *DEF_OBCHMP_LE = "*Default (acquired channel order)";


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_obxtab::Config_obxtab( ConfigCtl *cfg, QWidget *tab )
    :   QObject(0), obxTabUI(0), cfg(cfg)
{
    obxTabUI = new Ui::ObxTab;
    obxTabUI->setupUi( tab );
    ConnectUI( obxTabUI->obxTbl, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()) );
    ConnectUI( obxTabUI->obxTbl, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClicked(int,int)) );
    ConnectUI( obxTabUI->obxTbl, SIGNAL(cellChanged(int,int)), this, SLOT(cellChanged(int,int)) );
    ConnectUI( obxTabUI->defBut, SIGNAL(clicked()), this, SLOT(defBut()) );
    ConnectUI( obxTabUI->copyAllBut, SIGNAL(clicked()), this, SLOT(copyAllBut()) );
    ConnectUI( obxTabUI->copyToBut, SIGNAL(clicked()), this, SLOT(copyToBut()) );
}


Config_obxtab::~Config_obxtab()
{
    if( obxTabUI ) {
        delete obxTabUI;
        obxTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_obxtab::toGUI()
{
// ----
// Each
// ----

    if( !sn2set.size() )
        loadSettings();

    onDetect();
    toTbl();
    obxTabUI->obxTbl->setCurrentCell( 0, TBL_XA );

    obxTabUI->toSB->setMaximum( obxTabUI->obxTbl->rowCount() - 1 );

// --------------------
// Observe dependencies
// --------------------

    selectionChanged();
}


void Config_obxtab::fromGUI( DAQ::Params &q )
{
    fromTbl();

    q.im.set_cfg_nobx( each, obxTabUI->obxTbl->rowCount() );
}


void Config_obxtab::updateSaveChans( CimCfg::ObxEach &E, int isel )
{
    SignalBlocker   b0(obxTabUI->obxTbl);

// Update GUI

    QTableWidget        *T  = obxTabUI->obxTbl;
    QTableWidgetItem    *ti = T->item( isel, TBL_SAVE );

    each[isel].sns.uiSaveChanStr = E.sns.uiSaveChanStr;
    ti->setText( E.sns.uiSaveChanStr );
}


void Config_obxtab::updateObx( const CimCfg::ObxEach &E, int isel )
{
    each[isel] = E;
}


void Config_obxtab::saveSettings()
{
// ----------------
// Xfer to database
// ----------------

    QString   now( QDateTime::currentDateTime().toString() );

    for( int ip = 0, np = each.size(); ip < np; ++ip ) {
        CimCfg::ObxEach &E = each[ip];
        E.when = now;
        sn2set[cfg->prbTab.get_iOneBox( ip ).obsn] = E;
    }

// --------------
// Store database
// --------------

    QSettings   S( calibPath( "imec_onebox_settings" ), QSettings::IniFormat );
    S.remove( "SerialNumberToOneBox" );
    S.beginGroup( "SerialNumberToOneBox" );

    QMap<int,CimCfg::ObxEach>::const_iterator
        it  = sn2set.cbegin(),
        end = sn2set.cend();

    for( ; it != end; ++it ) {

        S.beginGroup( QString("SN%1").arg( it.key() ) );
            it->saveSettings( S );
        S.endGroup();
    }
}


QString Config_obxtab::remoteSetObxEach( const QString &s, int istr )
{
    ConfigCtl           *C      = mainApp()->cfgCtl();
    const DAQ::Params   &p      = C->acceptedParams;
    int                 isel    = p.im.obx_istr2isel( istr ),
                        slot    = C->prbTab.get_iOneBox( isel ).slot;
    CimCfg::ObxEach     &E      = each[isel];
    QTextStream         ts( s.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString             line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > 0 && eq < line.length() - 1 ) {

            QString k = line.left( eq ).trimmed(),
                    v = line.mid( eq + 1 ).trimmed();

            if( k == "obXAChans" )
                E.uiXAStr = v;
            else if( k == "obDigital" ) {
                bool    ok;
                E.isXD = v.toInt( &ok );
                if( !ok ) {
                    if( v.startsWith( "t", Qt::CaseInsensitive ) )
                        E.isXD = true;
                    else if( v.startsWith( "f", Qt::CaseInsensitive ) )
                        E.isXD = false;
                    else
                        return "SETPARAMSOBX: obDigital is one of {true,false}.";
                }
            }
            else if( k == "obAiRangeMax" ) {
                if( v == "2.5" || v == "5" || v == "10" ) {
                    E.range.rmax = v.toDouble();
                    E.range.rmin = -E.range.rmax;
                }
                else
                    return "SETPARAMSOBX: obAiRangeMax is one of {2.5,5,10}.";
            }
            else if( k == "obAOChans" )
                E.uiAOStr = v;
            else if( k == "obSnsChanMapFile" )
                E.sns.chanMapFile = v;
            else if( k == "obSnsSaveChanSubset" )
                E.sns.uiSaveChanStr = v;
        }
    }

    const CimCfg::ObxEach   &e = p.im.get_iStrOneBox( istr );

    if( e.isStream() && !E.isStream() ) {
        return
        QString("SETPARAMSOBX: Obx ip %1 slot %2 was a recording device"
        " at Detect; recording cannot be disabled remotely.")
        .arg( istr ).arg( slot );
    }
    else if( !e.isStream() && E.isStream() ) {
        return
        QString("SETPARAMSOBX: Obx slot %1 was not a recording device"
        " at Detect; recording cannot be enabled remotely.")
        .arg( slot );
    }

    toTbl( isel );

    return QString();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_obxtab::selectionChanged()
{
// get selection

    QTableWidget        *T      = obxTabUI->obxTbl;
    QTableWidgetItem    *ti     = T->currentItem();
    int                 ip      = -1,
                        np      = cfg->prbTab.nSelOneBox();
    bool                enab    = false;

    if( ti ) {

        ip = ti->row();

        if( ip < 0 || ip >= np )
            goto none;

        enab = true;
    }

none:
    obxTabUI->defBut->setEnabled( enab );

    enab &= np > 1;
    obxTabUI->copyToBut->setEnabled( enab );
    obxTabUI->toSB->setEnabled( enab );
    obxTabUI->copyAllBut->setEnabled( enab );
}


void Config_obxtab::cellDoubleClicked( int ip, int col )
{
    Q_UNUSED( ip )

    switch( col ) {
        case TBL_CHAN: QTimer::singleShot( 150, this, SLOT(editChan()) ); return;
    }
}


void Config_obxtab::editChan()
{
    int             ip = curObx();
    CimCfg::ObxEach &E = each[ip];

    fromTbl( ip );

// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    E.deriveChanCounts();

    const int   *cum    = E.obCumTypCnt;
    ChanMapOB   defMap(
        cum[CimCfg::obTypeXA],
        cum[CimCfg::obTypeXD] - cum[CimCfg::obTypeXA],
        cum[CimCfg::obTypeSY] - cum[CimCfg::obTypeXD] );

// -------------
// Launch editor
// -------------

    ChanMapCtl  CM( cfg->dialog(), defMap );

    E.sns.chanMapFile = CM.edit( E.sns.chanMapFile, ip );
    toTbl( ip );
}


void Config_obxtab::cellChanged( int ip, int col )
{
    if( col == TBL_SAVE ) {

        fromTbl( ip );

        CimCfg::ObxEach &E = each[ip];
        QVector<uint>   vc;
        QString         err;
        int             nC;
        bool            ok = true;

        if( !Subset::rngStr2Vec( vc, E.uiXAStr ) ) {
            err = "Obx: XA format error.";
            ok  = false;
        }

        if( ok ) {
            E.deriveChanCounts();
            nC = E.obCumTypCnt[CimCfg::obSumAll];
            ok = E.sns.deriveSaveData( err, DAQ::Params::jsip2stream( jsOB, ip ), nC );
        }

        if( ok )
            updateSaveChans( E, ip );
        else if( !err.isEmpty() )
            QMessageBox::critical( cfg->dialog(), "Obx Parameter Error", err );
    }
}


void Config_obxtab::defBut()
{
    int             ip = curObx();
    CimCfg::ObxEach &E = each[ip];

    E.range.rmin    = -5.0;
    E.range.rmax    = 5.0;
    E.uiXAStr       = "0:11";
    E.uiAOStr       = "";
    E.isXD          = true;
    E.sns.shankMapFile.clear();
    E.sns.chanMapFile.clear();
    E.sns.uiSaveChanStr.clear();

    toTbl( ip );
}


void Config_obxtab::copyAllBut()
{
    int ps = curObx(),
        np = obxTabUI->obxTbl->rowCount();

    fromTbl( ps );

    for( int pd = 0; pd < np; ++pd ) {
        if( pd != ps ) {
            copy( pd, ps );
            toTbl( pd );
        }
    }
}


void Config_obxtab::copyToBut()
{
    int ps = curObx(),
        pd = obxTabUI->toSB->value();

    if( pd != ps ) {
        fromTbl( ps );
        copy( pd, ps );
        toTbl( pd );
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_obxtab::loadSettings()
{
    QDateTime   old( QDateTime::currentDateTime() );
    QSettings   S( calibPath( "imec_onebox_settings" ), QSettings::IniFormat );
    S.beginGroup( "SerialNumberToOneBox" );

    old = old.addYears( -3 );
    sn2set.clear();

    foreach( QString sn, S.childGroups() ) {

        S.beginGroup( sn );

            CimCfg::ObxEach E;
            E.loadSettings( S );

            if( QDateTime::fromString( E.when ) >= old )
                sn2set[sn.remove( 0, 2 ).toInt()] = E;

        S.endGroup();
    }
}


void Config_obxtab::onDetect()
{
    int np = cfg->prbTab.nSelOneBox();

    each.clear();

    QMap<int,CimCfg::ObxEach>::const_iterator
        it, end = sn2set.end();

    for( int ip = 0; ip < np; ++ip ) {

        const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iOneBox( ip );

        // -----------------------
        // Existing SN, or default
        // -----------------------

        it = sn2set.find( P.obsn );

        if( it != end )
            each.push_back( *it );
        else
            each.push_back( CimCfg::ObxEach() );

        CimCfg::ObxEach &E = each[ip];

        // ----------
        // True srate
        // ----------

        E.srate = cfg->prbTab.get_iOneBox_SRate( ip );
    }
}


void Config_obxtab::toTbl()
{
    QTableWidget    *T = obxTabUI->obxTbl;
    SignalBlocker   b0(T);
    int             np = cfg->prbTab.nSelOneBox();

    T->setRowCount( np );

    for( int ip = 0; ip < np; ++ip ) {

        QTableWidgetItem            *ti;
        const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iOneBox( ip );
        const CimCfg::ObxEach       &E = each[ip];

        // ---------
        // Row label
        // ---------

        if( !(ti = T->verticalHeaderItem( ip )) ) {
            ti = new QTableWidgetItem;
            T->setVerticalHeaderItem( ip, ti );
        }

        ti->setText( QString::number( ip ) );

        // --
        // SN
        // --

        if( !(ti = T->item( ip, TBL_SN )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ip, TBL_SN, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        ti->setText( QString::number( P.obsn ) );

        // ----
        // Rate
        // ----

        if( !(ti = T->item( ip, TBL_RATE )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ip, TBL_RATE, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        ti->setText( QString("%1").arg( E.srate, 0, 'f', 6 ) );

        // -------------
        // User editable
        // -------------

        toTbl( ip );
    }

    T->resizeColumnToContents( TBL_SN );
    T->resizeColumnToContents( TBL_RATE );
    T->resizeColumnToContents( TBL_XA );
    T->resizeColumnToContents( TBL_XD );
    T->resizeColumnToContents( TBL_V );
    T->resizeColumnToContents( TBL_AO );
}


void Config_obxtab::fromTbl()
{
    for( int ip = 0, np = obxTabUI->obxTbl->rowCount(); ip < np; ++ip )
        fromTbl( ip );
}


void Config_obxtab::toTbl( int ip )
{
    QTableWidget            *T = obxTabUI->obxTbl;
    QTableWidgetItem        *ti;
    QComboBox               *cb;
    const CimCfg::ObxEach   &E = each[ip];

// --
// XA
// --

    if( !(ti = T->item( ip, TBL_XA )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_XA, ti );
        ti->setFlags( Qt::ItemIsSelectable
                        | Qt::ItemIsEnabled
                        | Qt::ItemIsEditable );
    }

    ti->setText( E.uiXAStr );

// --
// XD
// --

    if( !(ti = T->item( ip, TBL_XD )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_XD, ti );
        ti->setFlags( Qt::ItemIsSelectable
                        | Qt::ItemIsEnabled
                        | Qt::ItemIsUserCheckable );
    }

    ti->setCheckState( E.isXD ? Qt::Checked : Qt::Unchecked );

// -
// V
// -

    if( !(ti = T->item( ip, TBL_V )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_V, ti );
        cb = new QComboBox;
        cb->addItem( "-2.5, 2.5" );
        cb->addItem( "-5, 5" );
        cb->addItem( "-10, 10" );
        T->setCellWidget( ip, TBL_V, cb );
        ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }

    cb = (QComboBox*)T->cellWidget( ip, TBL_V );
    if( E.range.rmax > 6.0 )
        cb->setCurrentIndex( 2 );
    else if( E.range.rmax > 4.0 )
        cb->setCurrentIndex( 1 );
    else
        cb->setCurrentIndex( 0 );

// --
// AO
// --

    if( !(ti = T->item( ip, TBL_AO )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_AO, ti );
        ti->setFlags( Qt::ItemIsSelectable
                        | Qt::ItemIsEnabled
                        | Qt::ItemIsEditable );
    }

    ti->setText( E.uiAOStr );

// --------
// Chan map
// --------

    if( !(ti = T->item( ip, TBL_CHAN )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_CHAN, ti );
        ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }

    if( E.sns.chanMapFile.isEmpty() || E.sns.chanMapFile.contains( "*" ) )
        ti->setText( DEF_OBCHMP_LE );
    else
        ti->setText( E.sns.chanMapFile );

// ----------
// Save chans
// ----------

    if( !(ti = T->item( ip, TBL_SAVE )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_SAVE, ti );
        ti->setFlags( Qt::ItemIsSelectable
                        | Qt::ItemIsEnabled
                        | Qt::ItemIsEditable );
    }

    if( E.sns.uiSaveChanStr.isEmpty() || E.sns.uiSaveChanStr.contains( "*" ) )
        ti->setText( "all" );
    else
        ti->setText( E.sns.uiSaveChanStr );
}


void Config_obxtab::fromTbl( int ip )
{
    QTableWidget        *T = obxTabUI->obxTbl;
    QTableWidgetItem    *ti;
    QComboBox           *cb;
    CimCfg::ObxEach     &E = each[ip];

// --
// XA
// --

    ti          = T->item( ip, TBL_XA );
    E.uiXAStr   = ti->text().trimmed();

// --
// XD
// --

    ti      = T->item( ip, TBL_XD );
    E.isXD  = (ti->checkState() == Qt::Checked);

// -
// V
// -

    cb = (QComboBox*)T->cellWidget( ip, TBL_V );
    switch( cb->currentIndex() ) {
        case 0: E.range.rmax = 2.5; break;
        case 1: E.range.rmax = 5;   break;
        case 2: E.range.rmax = 10;  break;
    }
    E.range.rmin = -E.range.rmax;

// --
// AO
// --

    ti          = T->item( ip, TBL_AO );
    E.uiAOStr   = ti->text().trimmed();

// --------
// Chan map
// --------

    ti                  = T->item( ip, TBL_CHAN );
    E.sns.chanMapFile   = ti->text();
    if( E.sns.chanMapFile.contains( "*" ) )
        E.sns.chanMapFile.clear();

// ----------
// Save chans
// ----------

    ti                  = T->item( ip, TBL_SAVE );
    E.sns.uiSaveChanStr = ti->text().trimmed();
}


// Copy those settings that make sense.
//
void Config_obxtab::copy( int idst, int isrc )
{
    if( idst != isrc ) {
        each[idst] = each[isrc];
        each[idst].srate = cfg->prbTab.get_iOneBox_SRate( idst );
    }
}


int Config_obxtab::curObx()
{
    return obxTabUI->obxTbl->currentRow();
}


