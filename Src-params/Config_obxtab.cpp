
#include "ui_ObxTab.h"

#include "Util.h"
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
#define TBL_CHAN    5
#define TBL_SAVE    6

static const char *DEF_OBCHMP_LE = "*Default (acquired channel order)";


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_obxtab::Config_obxtab( ConfigCtl *cfg, QWidget *tab )
    : obxTabUI(0), cfg(cfg)
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


void Config_obxtab::regularizeSaveChans( CimCfg::ObxEach &E, int nC, int ip )
{
    SignalBlocker   b0(obxTabUI->obxTbl);
    QBitArray       &B  = E.sns.saveBits;

// Always add sync

    B.setBit( nC - 1 );

// Neaten text

    if( B.count( true ) == nC )
        E.sns.uiSaveChanStr = "all";
    else
        E.sns.uiSaveChanStr = Subset::bits2RngStr( B );

// Update GUI

    QTableWidget        *T  = obxTabUI->obxTbl;
    QTableWidgetItem    *ti = T->item( ip, TBL_SAVE );

    each[ip].sns.uiSaveChanStr = E.sns.uiSaveChanStr;
    ti->setText( E.sns.uiSaveChanStr );
}


void Config_obxtab::updateObx( const CimCfg::ObxEach &E, int ip )
{
    each[ip] = E;
}


void Config_obxtab::saveSettings()
{
// ----------------
// Xfer to database
// ----------------

    for( int ip = 0, np = each.size(); ip < np; ++ip )
        sn2set[cfg->prbTab.get_iOnebox( ip ).obsn] = each[ip];

// --------------
// Store database
// --------------

    QDateTime   now( QDateTime::currentDateTime() );
    QSettings   S( calibPath( "imec_onebox_settings" ), QSettings::IniFormat );
    S.remove( "SerialNumberToOnebox" );
    S.beginGroup( "SerialNumberToOnebox" );

    QMap<int,CimCfg::ObxEach>::const_iterator
        it  = sn2set.cbegin(),
        end = sn2set.cend();

    for( ; it != end; ++it ) {

        S.beginGroup( QString("SN%1").arg( it.key() ) );

        S.setValue( "__when", now.toString() );
        S.setValue( "obAiRangeMax", it->range.rmax );
        S.setValue( "obXAChans", it->uiXAStr );
        S.setValue( "obDigital", it->digital );
        S.setValue( "obSnsShankMapFile", it->sns.shankMapFile );
        S.setValue( "obSnsChanMapFile", it->sns.chanMapFile );
        S.setValue( "obSnsSaveChanSubset", it->sns.uiSaveChanStr );

        S.endGroup();
    }
}


QString Config_obxtab::remoteSetObxEach( const QString &s, int ip )
{
    CimCfg::ObxEach &E = each[ip];
    QTextStream     ts( s.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString         line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > 0 && eq < line.length() - 1 ) {

            QString k = line.left( eq ).trimmed(),
                    v = line.mid( eq + 1 ).trimmed();

            if( k == "obAiRangeMax" ) {
                if( v == "2" || v == "5" || v == "10" ) {
                    E.range.rmax = v.toDouble();
                    E.range.rmin = -E.range.rmax;
                }
                else
                    return "SETPARAMSOBX: obAiRangeMax is one of {2,5,10}.";
            }
            else if( k == "obXAChans" )
                E.uiXAStr = v;
            else if( k == "obDigital" ) {
                bool    ok;
                E.digital = v.toInt( &ok );
                if( !ok ) {
                    if( v.startsWith( "t", Qt::CaseInsensitive ) )
                        E.digital = true;
                    else if( v.startsWith( "f", Qt::CaseInsensitive ) )
                        E.digital = false;
                    else
                        return "SETPARAMSOBX: obDigital is one of {true,false}.";
                }
            }
            else if( k == "obSnsShankMapFile" )
                E.sns.shankMapFile = v;
            else if( k == "obSnsChanMapFile" )
                E.sns.chanMapFile = v;
            else if( k == "obSnsSaveChanSubset" )
                E.sns.uiSaveChanStr = v;
        }
    }

    toTbl( ip );

    return QString();
}


QString Config_obxtab::remoteGetObxEach( const DAQ::Params &p, int ip )
{
    QString                 s;
    const CimCfg::ObxEach   &E = p.im.obxj[ip];

    s  = QString("obAiRangeMax=%1\n").arg( E.range.rmax );
    s += QString("obXAChans=%1\n").arg( E.uiXAStr );
    s += QString("obDigital=%1\n").arg( E.digital );
    s += QString("obSnsShankMapFile=%1\n").arg( E.sns.shankMapFile );
    s += QString("obSnsChanMapFile=%1\n").arg( E.sns.chanMapFile );
    s += QString("obSnsSaveChanSubset=%1\n").arg( E.sns.uiSaveChanStr );

    return s;
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
                        np      = cfg->prbTab.nLogOnebox();
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

    const int   *type = E.obCumTypCnt;
    ChanMapOB   defMap(
        type[CimCfg::obTypeXA],
        type[CimCfg::obTypeXD] - type[CimCfg::obTypeXA],
        type[CimCfg::obTypeSY] - type[CimCfg::obTypeXD] );

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
            ok = E.sns.deriveSaveBits( err, DAQ::Params::jsip2stream( 1, ip ), nC );
        }

        if( ok )
            regularizeSaveChans( E, nC, ip );
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
    E.digital       = true;
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
    S.beginGroup( "SerialNumberToOnebox" );

    old = old.addYears( -3 );
    sn2set.clear();

    foreach( QString sn, S.childGroups() ) {

        S.beginGroup( sn );

        QDateTime   T = QDateTime::fromString(
                            S.value( "__when" ).toString() );

        if( T >= old ) {

            CimCfg::ObxEach E;
            E.range.rmax        = S.value( "obAiRangeMax", 5.0 ).toDouble();
            E.range.rmin        = -E.range.rmax;
            E.uiXAStr           = S.value( "obXAChans", "0:11" ).toString();
            E.digital           = S.value( "obDigital", true ).toBool();
            E.sns.shankMapFile  = S.value( "obSnsShankMapFile", QString() ).toString();
            E.sns.chanMapFile   = S.value( "obSnsChanMapFile", QString() ).toString();
            E.sns.uiSaveChanStr = S.value( "obSnsSaveChanSubset", "all" ).toString();
            sn2set[sn.remove( 0, 2 ).toInt()] = E;
        }

        S.endGroup();
    }
}


void Config_obxtab::onDetect()
{
    int np = cfg->prbTab.nLogOnebox();

    each.clear();

    QMap<int,CimCfg::ObxEach>::const_iterator
        it, end = sn2set.end();

    for( int ip = 0; ip < np; ++ip ) {

        const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iOnebox( ip );

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

        E.srate = cfg->prbTab.get_iOnebox_SRate( ip );
    }
}


void Config_obxtab::toTbl()
{
    QTableWidget    *T = obxTabUI->obxTbl;
    SignalBlocker   b0(T);
    int             np = cfg->prbTab.nLogOnebox();

    T->setRowCount( np );

    for( int ip = 0; ip < np; ++ip ) {

        QTableWidgetItem            *ti;
        const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iOnebox( ip );
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
    T->resizeColumnToContents( TBL_XD );
    T->resizeColumnToContents( TBL_V );
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

    ti->setCheckState( E.digital ? Qt::Checked : Qt::Unchecked );

// -
// V
// -

    if( !(ti = T->item( ip, TBL_V )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_V, ti );
        cb = new QComboBox;
        cb->addItem( "-2, 2" );
        cb->addItem( "-5, 5" );
        cb->addItem( "-10, 10" );
        T->setCellWidget( ip, TBL_V, cb );
        ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }

    cb = (QComboBox*)T->cellWidget( ip, TBL_V );
    if( E.range.rmax == 10.0 )
        cb->setCurrentIndex( 2 );
    else if( E.range.rmax == 5.0 )
        cb->setCurrentIndex( 1 );
    else
        cb->setCurrentIndex( 0 );

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

    ti          = T->item( ip, TBL_XD );
    E.digital   = (ti->checkState() == Qt::Checked);

// -
// V
// -

    cb = (QComboBox*)T->cellWidget( ip, TBL_V );
    switch( cb->currentIndex() ) {
        case 0: E.range.rmax = 2; break;
        case 1: E.range.rmax = 5; break;
        case 2: E.range.rmax = 10; break;
    }
    E.range.rmin = -E.range.rmax;

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
        each[idst].srate = cfg->prbTab.get_iOnebox_SRate( idst );
    }
}


int Config_obxtab::curObx()
{
    return obxTabUI->obxTbl->currentRow();
}


