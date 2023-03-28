
#include "ui_IMTab.h"
#include "ui_IMForceDlg.h"
#include "ui_IMSaveChansDlg.h"

#include "Util.h"
#include "ConfigCtl.h"
#include "Config_imtab.h"
#include "ShankCtlBase.h"
#include "ChanMapCtl.h"
#include "ShankMapCtl.h"
#include "Subset.h"
#include "SignalBlocker.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>

#define TBL_SN      0
#define TBL_PN      1
#define TBL_RATE    2
#define TBL_LED     3
#define TBL_IMRO    4
#define TBL_STBY    5
#define TBL_CHAN    6
#define TBL_SAVE    7

static const char *DEF_IMCHMP_LE = "*Default (shank by shank; tip to base)";


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_imtab::Config_imtab( ConfigCtl *cfg, QWidget *tab )
    :   QObject(0), imTabUI(0), cfg(cfg)
{
    imTabUI = new Ui::IMTab;
    imTabUI->setupUi( tab );
    ConnectUI( imTabUI->calCB, SIGNAL(currentIndexChanged(int)), cfg, SLOT(updateCalWarning()) );
    ConnectUI( imTabUI->svyChk, SIGNAL(clicked(bool)), this, SLOT(svyChkClicked()) );
    ConnectUI( imTabUI->prbTbl, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()) );
    ConnectUI( imTabUI->prbTbl, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClicked(int,int)) );
    ConnectUI( imTabUI->forceBut, SIGNAL(clicked()), this, SLOT(forceBut()) );
    ConnectUI( imTabUI->defBut, SIGNAL(clicked()), this, SLOT(defBut()) );
    ConnectUI( imTabUI->copyAllBut, SIGNAL(clicked()), this, SLOT(copyAllBut()) );
    ConnectUI( imTabUI->copyToBut, SIGNAL(clicked()), this, SLOT(copyToBut()) );
}


Config_imtab::~Config_imtab()
{
    if( imTabUI ) {
        delete imTabUI;
        imTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_imtab::toGUI( const DAQ::Params &p )
{
// ---
// All
// ---

    imTabUI->calCB->setCurrentIndex( p.im.prbAll.calPolicy );
    imTabUI->trgSrcCB->setCurrentIndex( p.im.prbAll.trgSource );
    imTabUI->trgEdgeCB->setCurrentIndex( p.im.prbAll.trgRising );

// @@@ FIX For now, only offering software triggering.
    imTabUI->trgSrcCB->setEnabled( false );
    imTabUI->trgEdgeCB->setEnabled( false );

    imTabUI->secsSB->setValue( p.im.prbAll.svySecPerBnk );
    imTabUI->secsSB->setEnabled( false );

    pairChk = p.sns.pairChk;

// ----
// Each
// ----

    if( !sn2set.size() )
        loadSettings();

    onDetect();
    toTbl();
    imTabUI->prbTbl->setCurrentCell( 0, TBL_LED );

    imTabUI->toSB->setMaximum( imTabUI->prbTbl->rowCount() - 1 );

// --------------------
// Observe dependencies
// --------------------

    selectionChanged();
    svyChkClicked( false );
}


void Config_imtab::fromGUI( DAQ::Params &q )
{
    q.im.prbAll.calPolicy       = imTabUI->calCB->currentIndex();
    q.im.prbAll.trgSource       = imTabUI->trgSrcCB->currentIndex();
    q.im.prbAll.trgRising       = imTabUI->trgEdgeCB->currentIndex();
    q.im.prbAll.svySecPerBnk    = imTabUI->secsSB->value();
    q.im.prbAll.isSvyRun        = imTabUI->svyChk->isChecked();

    q.sns.pairChk               = pairChk;

    fromTbl();

    q.im.set_cfg_nprb( each, imTabUI->prbTbl->rowCount() );
}


void Config_imtab::reset( const DAQ::Params &p )
{
    SignalBlocker   b0(imTabUI->calCB);
    imTabUI->calCB->setCurrentIndex( p.im.prbAll.calPolicy );
    imTabUI->svyChk->setChecked( false );
}


bool Config_imtab::calPolicyIsNever() const
{
    return imTabUI->calCB->currentIndex() == 2;
}


void Config_imtab::regularizeSaveChans( CimCfg::PrbEach &E, int nC, int ip )
{
    SignalBlocker   b0(imTabUI->prbTbl);
    QBitArray       &B  = E.sns.saveBits;
    int             nAP = E.roTbl->nAP();

// Always add sync

    B.setBit( nC - 1 );

// Pair LF to AP

    if( E.roTbl->nLF() == nAP && pairChk ) {

        bool    isAP = false;

        for( int b = 0; b < nAP; ++b ) {
            if( (isAP = B.testBit( b )) )
                break;
        }

        if( isAP ) {

            B.fill( 0, nAP, 2 * nAP );

            for( int b = 0; b < nAP; ++b ) {

                if( B.testBit( b ) )
                    B.setBit( nAP + b );
            }
        }
    }

// Neaten text

    if( B.count( true ) == nC )
        E.sns.uiSaveChanStr = "all";
    else
        E.sns.uiSaveChanStr = Subset::bits2RngStr( B );

// Update GUI

    each[ip].sns.uiSaveChanStr = E.sns.uiSaveChanStr;

    if( !imTabUI->svyChk->isChecked() ) {
        QTableWidget        *T  = imTabUI->prbTbl;
        QTableWidgetItem    *ti = T->item( ip, TBL_SAVE );
        ti->setText( E.sns.uiSaveChanStr );
    }
}


void Config_imtab::updateProbe( const CimCfg::PrbEach &E, int ip )
{
    each[ip] = E;
}


void Config_imtab::saveSettings()
{
// ----------------
// Xfer to database
// ----------------

    for( int ip = 0, np = each.size(); ip < np; ++ip )
        sn2set[cfg->prbTab.get_iProbe( ip ).sn] = each[ip];

// --------------
// Store database
// --------------

    QSettings   S( calibPath( "imec_probe_settings" ), QSettings::IniFormat );
    S.remove( "SerialNumberToProbe" );
    S.beginGroup( "SerialNumberToProbe" );

    QMap<quint64,CimCfg::PrbEach>::const_iterator
        it  = sn2set.cbegin(),
        end = sn2set.cend();

    for( ; it != end; ++it ) {

        S.beginGroup( QString("SN%1").arg( it.key() ) );
            it->saveSettings( S );
        S.endGroup();
    }
}


QString Config_imtab::remoteVfyPrbAll( const CimCfg &next, const CimCfg &prev )
{
// can't change enabled or num devices

    if( next.get_nProbes() != prev.get_nProbes() ) {

        return QString("SETPARAMSIMALL: Remote cannot change the logical probe count [%1].")
                .arg( prev.get_nProbes() );
    }

    if( next.get_nOneBox() != prev.get_nOneBox() ) {

        return QString("SETPARAMSIMALL: Remote cannot change the logical OneBox count [%1].")
                .arg( prev.get_nOneBox() );
    }

    if( next.prbAll.calPolicy < 0 || next.prbAll.calPolicy > 2 )
        return "SETPARAMSIMALL: Calibration file policy is one of {0=required,1=avail,2=never}.";

    if( next.prbAll.trgSource != 0 )
        return "SETPARAMSIMALL: Only software triggering is supported (trgSource=0).";

    return QString();
}


QString Config_imtab::remoteSetPrbEach( const QString &s, int ip )
{
    CimCfg::PrbEach &E = each[ip];
    QTextStream     ts( s.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString         line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > 0 && eq < line.length() - 1 ) {

            QString k = line.left( eq ).trimmed(),
                    v = line.mid( eq + 1 ).trimmed();

            if( k == "imroFile" )
                E.imroFile = v;
            else if( k == "imStdby" )
                E.stdbyStr = v;
            else if( k == "imSvyMaxBnk" )
                E.svyMaxBnk = v.toInt();
            else if( k == "imLEDEnable" ) {
                bool    ok;
                E.LEDEnable = v.toInt( &ok );
                if( !ok ) {
                    if( v.startsWith( "t", Qt::CaseInsensitive ) )
                        E.LEDEnable = true;
                    else if( v.startsWith( "f", Qt::CaseInsensitive ) )
                        E.LEDEnable = false;
                    else
                        return "SETPARAMSIMPRB: LEDEnable is one of {true,false}.";
                }
            }
            else if( k == "imSnsChanMapFile" )
                E.sns.chanMapFile = v;
            else if( k == "imSnsSaveChanSubset" )
                E.sns.uiSaveChanStr = v;
        }
    }

    toTbl( ip );

    return QString();
}

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_imtab::imro_done( ShankCtlBase *editor, QString fn, bool ok )
{
    if( ok || fn != each[imro_ip].imroFile ) {
        each[imro_ip].imroFile = fn;
        toTbl( imro_ip );
    }

    delete editor;
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_imtab::svyChkClicked( bool scroll )
{
    QString s;
    bool    enab;

    if( imTabUI->svyChk->isChecked() ) {
        s       = "Max bank";
        enab    = true;
    }
    else {
        s       = "Save chans";
        enab    = false;
    }

    imTabUI->secsSB->setEnabled( enab );
    selectionChanged();

    imTabUI->prbTbl->horizontalHeaderItem( TBL_SAVE )->setText( s );

    toTbl();

    if( scroll ) {
        imTabUI->prbTbl->horizontalScrollBar()->setValue(
            imTabUI->prbTbl->horizontalScrollBar()->maximum() );
    }
}


void Config_imtab::selectionChanged()
{
// get selection

    QTableWidget        *T      = imTabUI->prbTbl;
    QTableWidgetItem    *ti     = T->currentItem();
    int                 ip      = -1,
                        np      = cfg->prbTab.nLogProbes();
    bool                enab    = false;

    if( ti ) {

        ip = ti->row();

        if( ip < 0 || ip >= np )
            goto none;

        enab = true;
    }

none:
    imTabUI->forceBut->setEnabled( enab );
    imTabUI->defBut->setEnabled( enab );

    enab &= np > 1 && !imTabUI->svyChk->isChecked();
    imTabUI->copyAllBut->setEnabled( enab );
    imTabUI->copyToBut->setEnabled( enab );
    imTabUI->toSB->setEnabled( enab );
}


void Config_imtab::cellDoubleClicked( int ip, int col )
{
    Q_UNUSED( ip )

    switch( col ) {
        case TBL_IMRO: QTimer::singleShot( 150, this, SLOT(editIMRO()) ); return;
        case TBL_CHAN: QTimer::singleShot( 150, this, SLOT(editChan()) ); return;
        case TBL_SAVE:
            if( !imTabUI->svyChk->isChecked() )
                QTimer::singleShot( 150, this, SLOT(editSave()) );
            return;
    }
}


void Config_imtab::editIMRO()
{
    int             ip      = curProbe();
    CimCfg::PrbEach &E      = each[ip];
    QString         name0   = E.imroFile,
                    err;

    fromTbl( ip );

// -------------
// Launch editor
// -------------

// Logic a little tricky here. If for some reason the starting file name0
// is a bad match to this probe type we ammend it by clearing filename so
// shows up as default in editor (othewise would crash the editor). But
// editor's internal change detector might not see default as a change
// (default in and out) so post-updater imro_done() won't register change
// from name0 to default. We force imro_done() to detect change by poking
// name0 back in and looking for name change explicitly.

    if( !cfg->validImROTbl( err, E, ip ) && !err.isEmpty() ) {

        err += "\r\n\r\nReverting to default imro.";
        QMessageBox::critical( cfg->dialog(), "IMRO File Error", err );

        E.imroFile.clear(); // cleared to default for dialog
        cfg->validImROTbl( err, E, ip );
    }

    imro_ip = ip;
    ShankCtlBase*   shankCtl = new ShankCtlBase( cfg->dialog(), true );
    ConnectUI( shankCtl, SIGNAL(modal_done(ShankCtlBase*,QString,bool)), this, SLOT(imro_done(ShankCtlBase*,QString,bool)) );
    shankCtl->baseInit( E.roTbl, false );
    shankCtl->setOriginal( E.imroFile );
    E.imroFile = name0;     // keep old name for change detection
    shankCtl->showDialog();
}


void Config_imtab::editChan()
{
    int             ip = curProbe();
    CimCfg::PrbEach &E = each[ip];

    fromTbl( ip );

// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    E.deriveChanCounts();

    const int   *cum    = E.imCumTypCnt;
    ChanMapIM   defMap(
        cum[CimCfg::imTypeAP],
        cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP],
        cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );

// -------------
// Launch editor
// -------------

    ChanMapCtl  CM( cfg->dialog(), defMap );

    E.sns.chanMapFile = CM.edit( E.sns.chanMapFile, ip );
    toTbl( ip );
}


// Here we don't do a fromTbl/toTbl cycle for the whole row...
// Rather, we act only on saveChans cell via regularizeSaveChans().
//
void Config_imtab::editSave()
{
    QDialog             D;
    Ui::IMSaveChansDlg  ui;
    int                 ip      = curProbe(),
                        nAP     = 384,
                        nLF     = 384,
                        nSY     = 1;
    CimCfg::PrbEach     &E      = each[ip];
    QString             sOrig   = E.sns.uiSaveChanStr;

    D.setWindowFlags( D.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &D );

    if( E.roTbl ) {

        QString s;

        nAP = E.roTbl->nAP();
        nLF = E.roTbl->nLF();
        nSY = E.roTbl->nSY();

        s = QString("Ranges: AP[0:%1]").arg( nAP - 1 );

        if( nLF )
            s += QString(", LF[%1:%2]").arg( nAP ).arg( nAP + nLF - 1 );

        if( nSY )
            s += QString(", SY[%1]").arg( nAP + nLF );

        ui.rngLbl->setText( s );
        ui.saveChansLE->setText( E.sns.uiSaveChanStr );
        ui.pairChk->setChecked( pairChk );
        ui.pairChk->setEnabled( nLF > 0 );
    }
    else {
        QMessageBox::critical( cfg->dialog(),
            "IMTab Error",
            "ROTBL not allocated!" );
        return;
    }

// Run dialog until ok or cancel

    for(;;) {

        if( QDialog::Accepted == D.exec() ) {

            QString err;

            E.sns.uiSaveChanStr = ui.saveChansLE->text().trimmed();

            if( E.sns.deriveSaveBits(
                        err, DAQ::Params::jsip2stream( jsIM, ip ),
                        nAP+nLF+nSY ) ) {

                pairChk = ui.pairChk->isChecked();
                regularizeSaveChans( E, nAP+nLF+nSY, ip );
                break;
            }
            else
                QMessageBox::critical( cfg->dialog(), "Save Channels Error", err );
        }
        else {
            E.sns.uiSaveChanStr = sOrig;
            break;
        }
    }
}


void Config_imtab::forceBut()
{
    fromTbl();

    QDialog                     D;
    Ui::IMForceDlg              ui;
    int                         ip  = curProbe();
    const CimCfg::ImProbeDat    &P  = cfg->prbTab.get_iProbe( ip );

    D.setWindowFlags( D.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &D );
    ui.snOldLE->setText( QString::number( P.sn ) );
    ui.pnOldLE->setText( P.pn );

    QPushButton *B;

    B = ui.buttonBox->button( QDialogButtonBox::Ok );
    B->setText( "Apply" );
    B->setDefault( false );

    B = ui.buttonBox->button( QDialogButtonBox::Cancel );
    B->setDefault( true );
    B->setFocus();

    if( QDialog::Accepted == D.exec() ) {

        QString sn  = ui.snNewLE->text().trimmed(),
                pn  = ui.pnNewLE->text().trimmed();

        if( !sn.isEmpty() || !pn.isEmpty() ) {

            CimCfg::forceProbeData( P.slot, P.port, P.dock, sn, pn );
            cfg->initUsing_im_ob();
        }
    }
}


void Config_imtab::defBut()
{
    int             ip = curProbe();
    CimCfg::PrbEach &E = each[ip];

// Don't clear stdby "bad" channels

    if( !imTabUI->svyChk->isChecked() ) {
        E.imroFile.clear();
        E.LEDEnable = false;
        E.sns.chanMapFile.clear();
        E.sns.uiSaveChanStr.clear();
    }
    else
        E.svyMaxBnk = -1;

    toTbl( ip );
}


void Config_imtab::copyAllBut()
{
    int ps = curProbe(),
        np = imTabUI->prbTbl->rowCount();

    fromTbl( ps );

    for( int pd = 0; pd < np; ++pd ) {
        if( pd != ps ) {
            copy( pd, ps );
            toTbl( pd );
        }
    }
}


void Config_imtab::copyToBut()
{
    int ps = curProbe(),
        pd = imTabUI->toSB->value();

    if( pd != ps ) {
        fromTbl( ps );
        copy( pd, ps );
        toTbl( pd );
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_imtab::loadSettings()
{
    QDateTime   old( QDateTime::currentDateTime() );
    QSettings   S( calibPath( "imec_probe_settings" ), QSettings::IniFormat );
    S.beginGroup( "SerialNumberToProbe" );

    old = old.addYears( -3 );
    sn2set.clear();

    foreach( QString sn, S.childGroups() ) {

        S.beginGroup( sn );

        QDateTime   T = QDateTime::fromString(
                            S.value( "__when" ).toString() );

        if( T >= old ) {

            CimCfg::PrbEach E;
            E.loadSettings( S );
            sn2set[sn.remove( 0, 2 ).toULongLong()] = E;
        }

        S.endGroup();
    }
}


void Config_imtab::onDetect()
{
    int np = cfg->prbTab.nLogProbes();

    each.clear();

    QMap<quint64,CimCfg::PrbEach>::const_iterator
        it, end = sn2set.end();

    for( int ip = 0; ip < np; ++ip ) {

        const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iProbe( ip );

        // -----------------------
        // Existing SN, or default
        // -----------------------

        it = sn2set.find( P.sn );

        if( it != end )
            each.push_back( *it );
        else
            each.push_back( CimCfg::PrbEach() );

        CimCfg::PrbEach &E = each[ip];

        // ----------
        // True srate
        // ----------

        E.srate = cfg->prbTab.get_iProbe_SRate( ip );

        // ----------
        // Alloc imro
        // ----------

        if( E.roTbl && E.roTbl->pn != P.pn ) {
            delete E.roTbl;
            E.roTbl = 0;
        }

        if( !E.roTbl )
            E.roTbl = IMROTbl::alloc( P.pn );
    }
}


void Config_imtab::toTbl()
{
    QTableWidget    *T = imTabUI->prbTbl;
    SignalBlocker   b0(T);
    int             np = cfg->prbTab.nLogProbes();

    T->setRowCount( np );

    for( int ip = 0; ip < np; ++ip ) {

        QTableWidgetItem            *ti;
        const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iProbe( ip );
        const CimCfg::PrbEach       &E = each[ip];

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

        ti->setText( QString::number( P.sn ) );

        // --
        // PN
        // --

        if( !(ti = T->item( ip, TBL_PN )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ip, TBL_PN, ti );
            ti->setFlags( Qt::NoItemFlags );
        }

        ti->setText( P.pn );

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
    T->resizeColumnToContents( TBL_PN );
    T->resizeColumnToContents( TBL_RATE );
    T->resizeColumnToContents( TBL_LED );
}


void Config_imtab::fromTbl()
{
    for( int ip = 0, np = imTabUI->prbTbl->rowCount(); ip < np; ++ip )
        fromTbl( ip );
}


void Config_imtab::toTbl( int ip )
{
    QTableWidget                *T = imTabUI->prbTbl;
    QTableWidgetItem            *ti;
    const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iProbe( ip );
    const CimCfg::PrbEach       &E = each[ip];

// ---
// LED
// ---

    if( !(ti = T->item( ip, TBL_LED )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_LED, ti );
        ti->setFlags( Qt::ItemIsSelectable
                        | Qt::ItemIsEnabled
                        | Qt::ItemIsUserCheckable );
    }

    ti->setCheckState( E.LEDEnable ? Qt::Checked : Qt::Unchecked );

// ----
// IMRO
// ----

    if( !(ti = T->item( ip, TBL_IMRO )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_IMRO, ti );
        ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }

    if( E.imroFile.isEmpty() || E.imroFile.contains( "*" ) )
        ti->setText( IMROTbl::default_imroLE( P.type ) );
    else
        ti->setText( E.imroFile );

// ----
// Stby
// ----

    if( !(ti = T->item( ip, TBL_STBY )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_STBY, ti );
        ti->setFlags( Qt::ItemIsSelectable
                        | Qt::ItemIsEnabled
                        | Qt::ItemIsEditable );
    }

    ti->setText( E.stdbyStr );

// --------
// Chan map
// --------

    if( !(ti = T->item( ip, TBL_CHAN )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_CHAN, ti );
        ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }

    if( E.sns.chanMapFile.isEmpty() || E.sns.chanMapFile.contains( "*" ) )
        ti->setText( DEF_IMCHMP_LE );
    else
        ti->setText( E.sns.chanMapFile );

// ----------
// Save chans
// ----------

    if( !(ti = T->item( ip, TBL_SAVE )) ) {
        ti = new QTableWidgetItem;
        T->setItem( ip, TBL_SAVE, ti );
    }

    if( imTabUI->svyChk->isChecked() ) {

        int maxb    = E.roTbl->nBanks() - 1,
            v       = E.svyMaxBnk;

        if( v < 0 || v > maxb )
            v = maxb;

        ti->setFlags( Qt::ItemIsSelectable
                        | Qt::ItemIsEnabled
                        | Qt::ItemIsEditable );
        ti->setText( QString("%1  [0..%2]").arg( v ).arg( maxb ) );
    }
    else {
        ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        if( E.sns.uiSaveChanStr.isEmpty() || E.sns.uiSaveChanStr.contains( "*" ) )
            ti->setText( "all" );
        else
            ti->setText( E.sns.uiSaveChanStr );
    }
}


void Config_imtab::fromTbl( int ip )
{
    QTableWidget        *T = imTabUI->prbTbl;
    QTableWidgetItem    *ti;
    CimCfg::PrbEach     &E = each[ip];

// ---
// LED
// ---

    ti  = T->item( ip, TBL_LED );
    E.LEDEnable = (ti->checkState() == Qt::Checked);

// ----
// IMRO
// ----

    ti          = T->item( ip, TBL_IMRO );
    E.imroFile  = ti->text();
    if( E.imroFile.contains( "*" ) )
        E.imroFile.clear();

// ----
// Stby
// ----

    ti          = T->item( ip, TBL_STBY );
    E.stdbyStr  = ti->text().trimmed();

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

    ti = T->item( ip, TBL_SAVE );

    if( imTabUI->svyChk->isChecked() ) {

        int     maxb = E.roTbl->nBanks() - 1,
                v;
        bool    ok;

        v = ti->text()
                .split( QRegExp("\\s+"), QString::SkipEmptyParts )
                .at( 0 ).toInt( &ok );

        if( !ok || v < 0 || v > maxb )
            v = maxb;

        E.svyMaxBnk = v;
        ti->setText( QString("%1  [0..%2]").arg( v ).arg( maxb ) );
    }
    else
        E.sns.uiSaveChanStr = ti->text().trimmed();
}


// Copy those settings that make sense.
//
void Config_imtab::copy( int idst, int isrc )
{
    if( idst != isrc ) {

        const CimCfg::ImProbeDat    &P = cfg->prbTab.get_iProbe( idst );

        if( P.type == cfg->prbTab.get_iProbe( isrc ).type ) {

            CimCfg::PrbEach &D = each[idst];

            D = each[isrc];

            D.srate = cfg->prbTab.get_iProbe_SRate( idst );

            QMap<quint64,CimCfg::PrbEach>::const_iterator
                it  = sn2set.find( P.sn ),
                end = sn2set.end();

            if( it != end ) {
                D.stdbyStr  = it->stdbyStr;
                D.svyMaxBnk = it->svyMaxBnk;
            }
            else {
                D.stdbyStr.clear();
                D.svyMaxBnk = -1;
            }
        }
    }
}


int Config_imtab::curProbe()
{
    return imTabUI->prbTbl->currentRow();
}


