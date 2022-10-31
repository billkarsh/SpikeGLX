
#include "ui_DevTab.h"
#include "ui_HSSNDialog.h"

#include "Util.h"
#include "ConfigCtl.h"
#include "Config_devtab.h"
#include "ConfigSlotsCtl.h"
#include "SignalBlocker.h"
#include "Version.h"

#include <QCommonStyle>
#include <QMessageBox>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_devtab::Config_devtab( ConfigCtl *cfg, QWidget *tab )
    :   QObject(0), devTabUI(0), cfg(cfg)
{
    QIcon   warnIcon =
            QCommonStyle().standardIcon( QStyle::SP_MessageBoxWarning );

    devTabUI = new Ui::DevTab;
    devTabUI->setupUi( tab );
    devTabUI->warnIcon->setPixmap( warnIcon.pixmap( 24, 24 ) );
    devTabUI->warnIcon->setStyleSheet( "padding-bottom: 1px; padding-left: 20px" );
    devTabUI->warnIcon->hide();
    devTabUI->warnLbl->hide();
    ConnectUI( devTabUI->cfgSlotsBut, SIGNAL(clicked()), this, SLOT(cfgSlotsBut()) );
    ConnectUI( devTabUI->imecGB, SIGNAL(clicked()), cfg, SLOT(initUsing_im_ob()) );
    ConnectUI( devTabUI->imPrbTbl, SIGNAL(cellChanged(int,int)), this, SLOT(imPrbTabCellChng(int,int)) );
    ConnectUI( devTabUI->nidqGB, SIGNAL(clicked()), cfg, SLOT(initUsing_ni()) );
    ConnectUI( devTabUI->detectBut, SIGNAL(clicked()), this, SLOT(detectBut()) );
}


Config_devtab::~Config_devtab()
{
    if( devTabUI ) {
        delete devTabUI;
        devTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_devtab::toGUI( const DAQ::Params &p, bool loadProbes )
{
    SignalBlocker   b0(devTabUI->imecGB),
                    b1(devTabUI->nidqGB);

    devTabUI->warnIcon->hide();
    devTabUI->warnLbl->hide();

    devTabUI->imecGB->setChecked( p.im.enabled );
    devTabUI->nidqGB->setChecked( p.ni.enabled );

    devTabUI->bistChk->setChecked( p.im.prbAll.bistAtDetect );
    devTabUI->bistChk->setEnabled( p.im.enabled );

    if( loadProbes )
        prbTabToGUI();

// --------------------
// Observe dependencies
// --------------------
}


void Config_devtab::fromGUI( DAQ::Params &q )
{
    q.im.prbAll.bistAtDetect = devTabUI->bistChk->isChecked();
}


void Config_devtab::setNoDialogAccess( bool clearNi )
{
// Imec text

    devTabUI->imTE->clear();

    if( cfg->prbTab.nTblEntries() ) {

        imWrite( cfg->prbTab.whosChecked( devTabUI->imPrbTbl ) );
        imWrite( "\nFor probes:" );
        imWrite( "Alt-click toggles same docks this slot" );
        imWrite( "Ctrl-click toggles all docks this slot" );
        imWrite( "Shift-click toggles whole table" );
    }
    else {
        imWrite(
            "\n\n\n    Click 'Configure Slots'"
            " to add PXI/Onebox devices to the table" );
    }

// NI text

    if( clearNi )
        devTabUI->niTE->clear();

// BIST at detect

    devTabUI->bistChk->setEnabled( devTabUI->imecGB->isChecked() );

// Highlight Detect button

    devTabUI->detectBut->setDefault( true );
    devTabUI->detectBut->setFocus();
}


void Config_devtab::showCalWarning( bool show )
{
    if( show ) {
        devTabUI->warnIcon->show();
        devTabUI->warnLbl->show();
    }
    else {
        devTabUI->warnIcon->hide();
        devTabUI->warnLbl->hide();
    }
}


void Config_devtab::updateIMParams()
{
    prbTabToGUI();
    imWriteCurrent();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_devtab::cfgSlotsBut()
{
    CimCfg::ImProbeTable    &T = cfg->prbTab;

    T.fromGUI( devTabUI->imPrbTbl );

    ConfigSlotsCtl  CS( cfg->dialog(), T );

    if( CS.run() ) {
        prbTabToGUI();
        cfg->initUsing_im_ob();
    }
}


// subset 0: shift -> toggle ALL
// subset 1: ctrl  -> toggle this slot
// subset 2: alt   -> toggle this (slot, dock)
//
void Config_devtab::imPrbTabCellChng( int row, int col )
{
    Q_UNUSED( col )

    bool    shift = QApplication::keyboardModifiers() & Qt::ShiftModifier,
            ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier,
            alt   = QApplication::keyboardModifiers() & Qt::AltModifier;

    if( shift || ctrl || alt ) {

        int subset = 0;

        if( ctrl )
            subset = 1;
        else if( alt )
            subset = 2;

        cfg->prbTab.toggleAll( devTabUI->imPrbTbl, row, subset );
    }

    cfg->initUsing_im_ob();
}


void Config_devtab::hssnSaveSettings( const QString &key, const QString &val )
{
    STDSETTINGS( settings, "np20hssn" );
    settings.beginGroup( "HSSN" );
    settings.setValue( "key", key );
    settings.setValue( "val", val );
}


// Access Policy
// -------------
// (1) On entry to a dialog session, the checks (p.im.enable)
// set user intention and enable the possibility of setting
// the corresponding flag imecOK through the 'Detect' button.
//
// (2) It is the flag imecOK that governs access to tabs, and
// other dialog controls that require {hardware, config data}.
// That is, the check does not control access.
//
// (3) The user may revisit the devTab and uncheck a box, even
// after its flag is set. The doingImec() function, which looks
// at both check and flag is used as the final test of intent,
// and the test of what we need to strictly validate.
//
void Config_devtab::detectBut()
{
    bool    availIM = false,
            availNI = false;

    cfg->initUsing_all();

    if( !devTabUI->nidqGB->isChecked() )
        cfg->niSingletonRelease();

    if( !devTabUI->imecGB->isChecked()
        && !devTabUI->nidqGB->isChecked() ) {

        QMessageBox::information(
        cfg->dialog(),
        "No Hardware Selected",
        "'Enable' the hardware types {imec, nidq} you want use...\n\n"
        "Then click 'Detect' to see what's installed/working." );
        return;
    }

    if( devTabUI->imecGB->isChecked() )
        availIM = imDetect();

    if( devTabUI->nidqGB->isChecked() )
        availNI = niDetect();

    if( (devTabUI->imecGB->isChecked() && !availIM) ||
        (devTabUI->nidqGB->isChecked() && !availNI) ) {

        return;
    }

    cfg->setSelectiveAccess( availIM, availNI );
    prbTabToGUI();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_devtab::prbTabToGUI()
{
    cfg->prbTab.toGUI( devTabUI->imPrbTbl );
}


void Config_devtab::imWrite( const QString &s )
{
    QTextEdit   *te = devTabUI->imTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


void Config_devtab::imWriteCurrent()
{
    const CimCfg::ImProbeTable  &T = cfg->prbTab;

    QTextEdit   *te = devTabUI->imTE;
    te->clear();
    imWrite( "Previous data:" );
    imWrite( "-----------------------------------" );
    imWrite( QString("API version %1").arg( T.api ) );

    QMap<int,CimCfg::ImSlotVers>::const_iterator    it;

    for(
        it  = T.slot2Vers.begin();
        it != T.slot2Vers.end();
        ++it ) {

        imWrite(
            QString("BS(slot %1) firmware version %2")
            .arg( it.key() ).arg( it.value().bsfw ) );

        imWrite(
            QString("BSC(slot %1) part number %2")
            .arg( it.key() ).arg( it.value().bscpn ) );

        imWrite(
            QString("BSC(slot %1) serial number %2")
            .arg( it.key() ).arg( it.value().bscsn ) );

        imWrite(
            QString("BSC(slot %1) hardware version %2")
            .arg( it.key() ).arg( it.value().bschw ) );

        imWrite(
            QString("BSC(slot %1) firmware version %2")
            .arg( it.key() ).arg( it.value().bscfw ) );

        for( int k = 0, n = T.nTblEntries(); k < n; ++k ) {

            const CimCfg::ImProbeDat    &P = T.get_kTblEntry( k );

            if( !P.enab || P.slot != it.key() )
                continue;

            imWrite(
                QString("HS(slot %1, port %2) part number %3")
                .arg( P.slot ).arg( P.port ).arg( P.hspn ) );

            imWrite(
                QString("HS(slot %1, port %2) hardware version %3")
                .arg( P.slot ).arg( P.port ).arg( P.hshw ) );

            imWrite(
                QString("FX(slot %1, port %2, dock %3) part number %4")
                .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxpn ) );

            imWrite(
                QString("FX(slot %1, port %2, dock %3) serial number %4")
                .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxsn ) );

            imWrite(
                QString("FX(slot %1, port %2, dock %3) hardware version %4")
                .arg( P.slot ).arg( P.port ).arg( P.dock ).arg( P.fxhw ) );
        }
    }

    imWrite(
        QString("\nOK  %1").arg( T.whosChecked( devTabUI->imPrbTbl ) ) );
}


bool Config_devtab::imDetect()
{
    CimCfg::ImProbeTable    &T = cfg->prbTab;

// ------------------------
// Fill out the probe table
// ------------------------

    devTabUI->warnIcon->hide();
    devTabUI->warnLbl->hide();

    T.fromGUI( devTabUI->imPrbTbl );

// --------------------
// Error if table empty
// --------------------

    if( !T.nTblEntries() ) {

        QMessageBox::information(
        cfg->dialog(),
        "No Imec Slots Defined",
        "1. Click 'Configure Slots' to add PXI/Onebox devices to the table.\n"
        "2. Click 'Enable' for each probe or Onebox you want to run.\n"
        "3. Click 'Detect' to check if they're working." );

        imWrite( "\nFAIL - Cannot be used" );
        return false;
    }

    if( !T.buildEnabIndexTables() ) {

        QMessageBox::information(
        cfg->dialog(),
        "No Imec Streams Enabled",
        "Enable one or more probes/Oneboxes...\n\n"
        "Then click 'Detect' to check if they're working." );

        imWrite( "\nFAIL - Cannot be used" );
        return false;
    }

// --------------
// Query hardware
// --------------

    QStringList     slVers,
                    slBIST;
    QVector<int>    vHS20;
    bool            availIM;

    imWrite( "\nConnecting...allow several seconds." );
    guiBreathe();

    availIM = CimCfg::detect(
                slVers, slBIST, vHS20, T,
                devTabUI->bistChk->isChecked() );

// -------
// Reports
// -------

    QTextEdit   *te = devTabUI->imTE;

    te->clear();

    foreach( const QString &s, slVers )
        imWrite( s );

    if( availIM ) {

        imWrite(
            QString("\nOK  %1").arg( T.whosChecked( devTabUI->imPrbTbl ) ) );

        cfg->updateCalWarning();
    }
    else {
        imWrite( "\nFAIL - Cannot be used" );
        return false;
    }

// ----
// HS20
// ----

    if( !vHS20.isEmpty() )
        HSSNDialog( vHS20 );

// ------------
// BIST results
// ------------

    if( !slBIST.isEmpty() ) {

        QString msg("These probes failed BISTs (self tests):\n");

        foreach( const QString &s, slBIST )
            msg += "\n    " + s;

        msg += "\n\n\nTools and help are available here:\n"
               "    Tools/Imec BIST Diagnostics...\n";

        QMessageBox::warning( cfg->dialog(),
            "Probe Self Test Failures",
            msg );
    }

    return true;
}


void Config_devtab::HSSNDialog( QVector<int> &vP )
{
    CimCfg::ImProbeTable    &T = cfg->prbTab;

    QDialog         dlg;
    Ui::HSSNDialog  ui;

    dlg.setWindowFlags( dlg.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );

// Set key

    QString key, val, inikey, inival;

    foreach( int ip, vP ) {

        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
        key += QString("(%1,%2) ").arg( P.slot ).arg( P.port );
    }

    ui.keyLbl->setText( key = key.trimmed() );

// Set val

    {
        STDSETTINGS( settings, "np20hssn" );
        settings.beginGroup( "HSSN" );
        inikey = settings.value( "key", "" ).toString();
        inival = settings.value( "val", "" ).toString();

        if( key == inikey )
            ui.snLE->setText( inival );
    }

// Run dialog

runDialog:
    if( QDialog::Accepted == dlg.exec() ) {

        // Check count

        val = ui.snLE->text().trimmed();

        const QStringList   sl = val.split(
                                    QRegExp("^\\s+|\\s*,\\s*"),
                                    QString::SkipEmptyParts );

        if( sl.size() != vP.size() ) {

            QMessageBox::critical(
                cfg->dialog(),
                "Parameter Count Error",
                "Count of SN values and ports not equal." );
            goto runDialog;
        }

        // Store to table

        int nSN     = vP.size(),
            nTbl    = T.nLogProbes();

        for( int isn = 0; isn < nSN; ++isn ) {

            const CimCfg::ImProbeDat  &Psn = T.get_iProbe( vP[isn] );

            for( int k = 0; k < nTbl; ++k ) {

                CimCfg::ImProbeDat  &Ptb = T.mod_iProbe( k );

                if( Ptb.slot == Psn.slot && Ptb.port == Psn.port )
                    Ptb.hssn = sl.at( isn ).toInt();
            }
        }

        prbTabToGUI();
    }

// Save key, val

    QMetaObject::invokeMethod(
        this, "hssnSaveSettings",
        Qt::QueuedConnection,
        Q_ARG(QString, key),
        Q_ARG(QString, val) );
}


void Config_devtab::niWrite( const QString &s )
{
    QTextEdit   *te = devTabUI->niTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


QColor Config_devtab::niSetColor( const QColor &c )
{
    QTextEdit   *te     = devTabUI->niTE;
    QColor      cPrev   = te->textColor();

    te->setTextColor( c );

    return cPrev;
}


bool Config_devtab::niDetect()
{
    bool    availNI = false;

// -------------
// Report header
// -------------

    niWrite( "Input Devices:" );
    niWrite( "---------------------" );

// --------------------------------
// Error if hardware already in use
// --------------------------------

    if( !cfg->niSingletonReserve() ) {
        niWrite(
            "Another instance of " APPNAME " already owns"
            " the NI hardware." );
        goto exit;
    }

// --------------------
// Query input hardware
// --------------------

    if( !CniCfg::isHardware() ) {
        niWrite( "None" );
        goto exit;
    }

    CniCfg::probeAIHardware();
    CniCfg::probeAllDILines();

// ----------------------------------
// Report devs having either [AI, DI]
// ----------------------------------

    foreach( const QString &D, CniCfg::niDevNames ) {

        int nAI = CniCfg::aiDevChanCount.value( D, 0 ),
            nDI = CniCfg::diDevLineCount.value( D, 0 );

        if( nAI || nDI ) {
            niWrite(
                QString("%1 (%2)  AI[%3]  DI[%4]")
                .arg( D )
                .arg( CniCfg::getProductName( D ) )
                .arg( nAI )
                .arg( nDI ) );
            availNI = true;
        }
        else {
            QColor  c = niSetColor( Qt::darkRed );
            niWrite(
                QString("%1 (%2)  [--OFF--]")
                .arg( D )
                .arg( CniCfg::getProductName( D ) ) );
            niSetColor( c );
        }
    }

    if( !availNI )
        niWrite( "None" );

// ---------------------
// Query output hardware
// ---------------------

    CniCfg::probeAOHardware();
    CniCfg::probeAllDOLines();

// --------------------
// Output report header
// --------------------

    niWrite( "\nOutput Devices:" );
    niWrite( "---------------------" );

// ----------------------------------
// Report devs having either [AO, DO]
// ----------------------------------

    foreach( const QString &D, CniCfg::niDevNames ) {

        int nAO = CniCfg::aoDevChanCount.value( D, 0 ),
            nDO = CniCfg::doDevLineCount.value( D, 0 );

        if( nAO || nDO ) {
            niWrite(
                QString("%1 (%2)  AO[%3]  DO[%4]")
                .arg( D )
                .arg( CniCfg::getProductName( D ) )
                .arg( nAO )
                .arg( nDO ) );
            availNI = true;
        }
        else {
            QColor  c = niSetColor( Qt::darkRed );
            niWrite(
                QString("%1 (%2)  [--OFF--]")
                .arg( D )
                .arg( CniCfg::getProductName( D ) ) );
            niSetColor( c );
        }
    }

    if( !CniCfg::aoDevChanCount.size() && !CniCfg::doDevLineCount.size() )
        niWrite( "None" );

// -------------
// Finish report
// -------------

exit:
    niWrite( "-- end --" );

    if( availNI )
        niWrite( "\nOK" );
    else
        niWrite( "\nFAIL - Cannot be used" );

    return availNI;
}


