
#include "ui_IMTab.h"
#include "ui_IMForceDlg.h"

#include "Util.h"
#include "ConfigCtl.h"
#include "Config_imtab.h"
#include "KVParams.h"
#include "SaveChansCtl.h"
#include "ShankCtlBase.h"
#include "ChanMapCtl.h"
#include "ShankMapCtl.h"
#include "SignalBlocker.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
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

//@OBX367 np_setProbeHardwareID hidden in API 3.67
    imTabUI->forceBut->setDisabled( true );

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
    imTabUI->lowLatChk->setChecked( p.im.prbAll.lowLatency );
    imTabUI->trgSrcCB->setCurrentIndex( p.im.prbAll.trgSource );
    imTabUI->trgEdgeCB->setCurrentIndex( p.im.prbAll.trgRising );

// @@@ FIX For now, only offering software triggering.
    imTabUI->trgSrcCB->setEnabled( false );
    imTabUI->trgEdgeCB->setEnabled( false );

    imTabUI->svySettleSB->setValue( p.im.prbAll.svySettleSec );
    imTabUI->svySettleSB->setEnabled( false );

    imTabUI->svySecsSB->setValue( p.im.prbAll.svySecPerBnk );
    imTabUI->svySecsSB->setEnabled( false );

    imTabUI->qfGB->setChecked( p.im.prbAll.qf_on );

// default .5, index = 2
    int sel = imTabUI->qfSecsCB->findText( p.im.prbAll.qf_secsStr );
    imTabUI->qfSecsCB->setCurrentIndex( sel > -1 ? sel : 2 );

// default 0 = first
    sel = imTabUI->qfLoCB->findText( p.im.prbAll.qf_loCutStr );
    imTabUI->qfLoCB->setCurrentIndex( sel > -1 ? sel : 0 );

// default INF = last
    sel = imTabUI->qfHiCB->findText( p.im.prbAll.qf_hiCutStr );
    imTabUI->qfHiCB->setCurrentIndex( sel > -1 ? sel : imTabUI->qfHiCB->count()-1 );

    lfPairChk = p.sns.lfPairChk;

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
    q.im.prbAll.qf_secsStr      = imTabUI->qfSecsCB->currentText();
    q.im.prbAll.qf_loCutStr     = imTabUI->qfLoCB->currentText();
    q.im.prbAll.qf_hiCutStr     = imTabUI->qfHiCB->currentText();
    q.im.prbAll.calPolicy       = imTabUI->calCB->currentIndex();
    q.im.prbAll.lowLatency      = imTabUI->lowLatChk->isChecked();
    q.im.prbAll.trgSource       = imTabUI->trgSrcCB->currentIndex();
    q.im.prbAll.trgRising       = imTabUI->trgEdgeCB->currentIndex();
    q.im.prbAll.svySettleSec    = imTabUI->svySettleSB->value();
    q.im.prbAll.svySecPerBnk    = imTabUI->svySecsSB->value();
    q.im.prbAll.isSvyRun        = imTabUI->svyChk->isChecked();
    q.im.prbAll.qf_on           = imTabUI->qfGB->isChecked();

    q.sns.lfPairChk             = lfPairChk;

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


void Config_imtab::updateSaveChans( CimCfg::PrbEach &E, int ip )
{
    SignalBlocker   b0(imTabUI->prbTbl);

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

    QString   now( QDateTime::currentDateTime().toString() );

    for( int ip = 0, np = each.size(); ip < np; ++ip ) {
        CimCfg::PrbEach &E = each[ip];
        E.when = now;
        sn2set[cfg->prbTab.get_iProbe( ip ).sn] = E;
        sn2set[cfg->prbTab.get_iProbe( ip ).sn].when = now;
    }

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

    if( next.get_nObxStr() != prev.get_nObxStr() ) {

        return QString("SETPARAMSIMALL: Remote cannot change count of recording OneBoxes [%1].")
                .arg( prev.get_nObxStr() );
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
    // These needed to run dialog using exec()
    //
    // if( ok )
    //     editor->accept();
    // else
    //     editor->reject();

    if( !ok && !imro_cancelName.isEmpty() ) {
        ok = true;
        fn = imro_cancelName;
    }

    if( ok ) {
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

    imTabUI->svySettleSB->setEnabled( enab );
    imTabUI->svySecsSB->setEnabled( enab );
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
                        np      = cfg->prbTab.nSelProbes();
    bool                enab    = false;

    if( ti ) {

        ip = ti->row();

        if( ip < 0 || ip >= np )
            goto none;

        enab = true;
    }

none:
//@OBX367 np_setProbeHardwareID hidden in API 3.67
//    imTabUI->forceBut->setEnabled( enab );
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
    int             ip  = curProbe();
    CimCfg::PrbEach &E  = each[ip];
    QString         err;

// Validate IMRO

    fromTbl( ip );
    imro_cancelName.clear();
    imro_ip = ip;

    if( !cfg->validImROTbl( err, E, ip ) && !err.isEmpty() ) {

        err += "\r\n\r\nReverting to default imro.";
        QMessageBox::critical( cfg->dialog(), "IMRO File Error", err );

        imro_cancelName = E.imroFile;
        E.imroFile.clear();
        cfg->validImROTbl( err, E, ip );
    }

// -------------
// Launch editor
// -------------

    ShankCtlBase*   shankCtl = new ShankCtlBase( cfg->dialog(), true );
    ConnectUI( shankCtl, SIGNAL(runSaveChansDlg(QString)), this, SLOT(editSave(QString)) );
    ConnectUI( shankCtl, SIGNAL(modal_done(ShankCtlBase*,QString,bool)), this, SLOT(imro_done(ShankCtlBase*,QString,bool)) );
    shankCtl->baseInit( E.roTbl, false );
    shankCtl->setOriginal( E.imroFile );
    shankCtl->showDialog();
}


void Config_imtab::editChan()
{
    int             ip = curProbe();
    CimCfg::PrbEach &E = each[ip];
    QString         err;

// Validate IMRO

    fromTbl( ip );
    imro_cancelName.clear();
    imro_ip = ip;

    if( !cfg->validImROTbl( err, E, ip ) && !err.isEmpty() ) {

        err += "\r\n\r\nReverting to default imro.";
        QMessageBox::critical( cfg->dialog(), "IMRO File Error", err );

        imro_cancelName = E.imroFile;
        E.imroFile.clear();
        cfg->validImROTbl( err, E, ip );
    }

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
// Rather, we act only on saveChans cell via updateSaveChans().
//
void Config_imtab::editSave( QString sInit )
{
    int             ip = curProbe();
    CimCfg::PrbEach &E = each[ip];
    SaveChansCtl    SV( cfg->dialog(), E, ip );
    QString         err, saveStr = sInit;

// Validate IMRO

    if( sInit.isEmpty() ) {

        fromTbl( ip );
        imro_cancelName.clear();
        imro_ip = ip;

        if( !cfg->validImROTbl( err, E, ip ) && !err.isEmpty() ) {

            err += "\r\n\r\nReverting to default imro.";
            QMessageBox::critical( cfg->dialog(), "IMRO File Error", err );

            imro_cancelName = E.imroFile;
            E.imroFile.clear();
            cfg->validImROTbl( err, E, ip );
        }
    }

// Save dialog

    if( SV.edit( saveStr, lfPairChk ) ) {
        E.sns.uiSaveChanStr = saveStr;
        updateSaveChans( E, ip );
    }
}


void Config_imtab::forceBut()
{
    fromTbl();

    QDialog             D;
    Ui::IMForceDlg      ui;
    int                 ip  = curProbe();
    CimCfg::ImProbeDat  &P  = cfg->prbTab.mod_iProbe( ip );

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

        if( !sn.isEmpty() )
            P.sn = sn.toULongLong();

        if( !pn.isEmpty() )
            P.pn = pn;

        if( !sn.isEmpty() || !pn.isEmpty() )
            CimCfg::forceProbeData( P.slot, P.port, P.dock, sn, pn );
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

            CimCfg::PrbEach E;
            E.loadSettings( S );

            if( QDateTime::fromString( E.when ) >= old )
                sn2set[sn.remove( 0, 2 ).toULongLong()] = E;

        S.endGroup();
    }
}


void Config_imtab::onDetect()
{
    int np = cfg->prbTab.nSelProbes();

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

        // -----------------------
        // Recover imro from meta?
        // -----------------------

        if( it == end &&
            cfg->prbTab.simprb.isSimProbe( P.slot, P.port, P.dock ) ) {

            QString file = cfg->prbTab.simprb.file( P.slot, P.port, P.dock )
                            + ".ap.meta";

            KVParams    kvp;
            kvp.fromMetaFile( file );
            file = kvp["imroFile"].toString();

            if( file.isEmpty() )
                continue;

            QFile   f( file );
            if( f.exists() )
                continue;

            int yesNo = QMessageBox::question(
                cfg->dialog(),
                "Save IMRO from Metadata?",
                QString(
                "Probe %1 (part-num: %2) has never been run on this computer."
                " It is a simulated probe that originally had a custom imro"
                " path/file that does not exist on this computer:\n\n"
                "[%3]\n\n"
                "Do you want me to re-create the file here?" )
                .arg( ip ).arg( P.pn ).arg( file ),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes );

            if( yesNo != QMessageBox::Yes )
                continue;

            QFileInfo   fi( file ); // previous file name (no path)
            QString     fn = QFileDialog::getSaveFileName(
                                cfg->dialog(),
                                "Save IMEC readout table",
                                fi.fileName(),
                                "Imro files (*.imro)" );

            if( fn.length() ) {

                E.roTbl->fromString( 0, kvp["~imroTbl"].toString() );

                QString msg;

                if( E.roTbl->saveFile( msg, fn ) )
                    E.imroFile = fn;
                else
                    QMessageBox::critical( cfg->dialog(), "Save Error", msg );
            }
        }
    }
}


void Config_imtab::toTbl()
{
    QTableWidget    *T = imTabUI->prbTbl;
    SignalBlocker   b0(T);
    int             np = cfg->prbTab.nSelProbes();

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

        int maxb    = E.roTbl->nSvyBanks() - 1,
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

        int     maxb = E.roTbl->nSvyBanks() - 1,
                v;
        bool    ok;

        v = ti->text()
                .split( QRegularExpression("\\s+"), Qt::SkipEmptyParts )
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


