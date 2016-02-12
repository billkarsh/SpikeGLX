
#include "ui_ConfigureDialog.h"
#include "ui_NICfgTab.h"
#include "ui_GateTab.h"
#include "ui_GateImmedPanel.h"
#include "ui_GateTCPPanel.h"
#include "ui_TrigTab.h"
#include "ui_TrigImmedPanel.h"
#include "ui_TrigTimedPanel.h"
#include "ui_TrigTTLPanel.h"
#include "ui_TrigSpikePanel.h"
#include "ui_TrigTCPPanel.h"
#include "ui_SeeNSaveTab.h"

#include "Pixmaps/Icon-Config.xpm"

#include "Util.h"
#include "ConfigCtl.h"
#include "Subset.h"
#include "MainApp.h"
#include "ChanMapCtl.h"
#include "ConsoleWindow.h"

#include <QMessageBox>
#include <QDirIterator>


#define CURDEV1     niCfgTabUI->device1CB->currentIndex()
#define CURDEV2     niCfgTabUI->device2CB->currentIndex()


/* ---------------------------------------------------------------- */
/* ConfigCtl ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Note:
// -----
// QueuedConnection is needed for QComboBox signals because they
// may fire during dialog initialization when those controls are
// populated. We wish to defer that reaction.
//
ConfigCtl::ConfigCtl( QObject *parent )
    :   QObject(parent),
        cfgUI(0),
        niCfgTabUI(0),
        gateTabUI(0),
            gateImmPanelUI(0),
            gateTCPPanelUI(0),
        trigTabUI(0),
            trigImmPanelUI(0), trigTimPanelUI(0),
            trigTTLPanelUI(0), trigSpkPanelUI(0),
            trigTCPPanelUI(0),
        snsTabUI(0),
        cfgDlg(0)
{
    QWidget *panel;

// -----------
// Main dialog
// -----------

    cfgDlg = new QDialog;
    cfgDlg->setWindowIcon( QIcon(QPixmap(Icon_Config_xpm)) );

    cfgUI = new Ui::ConfigureDialog;
    cfgUI->setupUi( cfgDlg );
    cfgUI->tabsW->setCurrentIndex( 0 );
    ConnectUI( cfgUI->resetBut, SIGNAL(clicked()), this, SLOT(reset()) );
    ConnectUI( cfgUI->verifyBut, SIGNAL(clicked()), this, SLOT(verify()) );
    ConnectUI( cfgUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );

// Make OK default button

    QPushButton *B;

    B = cfgUI->buttonBox->button( QDialogButtonBox::Ok );
    B->setAutoDefault( true );
    B->setDefault( true );

    B = cfgUI->buttonBox->button( QDialogButtonBox::Cancel );
    B->setAutoDefault( false );
    B->setDefault( false );

// --------
// NICfgTab
// --------

    niCfgTabUI = new Ui::NICfgTab;
    niCfgTabUI->setupUi( cfgUI->niAITab );
    ConnectUI( niCfgTabUI->device1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(device1CBChanged()) );
    ConnectUI( niCfgTabUI->device2CB, SIGNAL(currentIndexChanged(int)), this, SLOT(device2CBChanged()) );
    ConnectUI( niCfgTabUI->mn1LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niCfgTabUI->ma1LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niCfgTabUI->mn2LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niCfgTabUI->ma2LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niCfgTabUI->dev2GB, SIGNAL(clicked()), this, SLOT(muxingChanged()) );
    ConnectUI( niCfgTabUI->aiRangeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(aiRangeChanged()) );
    ConnectUI( niCfgTabUI->clk1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(clk1CBChanged()) );
    ConnectUI( niCfgTabUI->freqBut, SIGNAL(clicked()), this, SLOT(freqButClicked()) );
    ConnectUI( niCfgTabUI->syncEnabChk, SIGNAL(clicked(bool)), this, SLOT(syncEnableClicked(bool)) );

// -------
// GateTab
// -------

    gateTabUI = new Ui::GateTab;
    gateTabUI->setupUi( cfgUI->gateTab );
    ConnectUI( gateTabUI->gateModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(gateModeChanged()) );

// Immediate
    panel = new QWidget( gateTabUI->gateFrame );
    panel->setObjectName( QString("panel_%1").arg( DAQ::eGateImmed ) );
    gateImmPanelUI = new Ui::GateImmedPanel;
    gateImmPanelUI->setupUi( panel );

// TCP
    panel = new QWidget( gateTabUI->gateFrame );
    panel->setObjectName( QString("panel_%1").arg( DAQ::eGateTCP ) );
    gateTCPPanelUI = new Ui::GateTCPPanel;
    gateTCPPanelUI->setupUi( panel );

// -------
// TrigTab
// -------

    trigTabUI = new Ui::TrigTab;
    trigTabUI->setupUi( cfgUI->trigTab );
    ConnectUI( trigTabUI->trigModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(trigModeChanged()) );

// Immediate
    panel = new QWidget( trigTabUI->trigFrame );
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigImmed ) );
    trigImmPanelUI = new Ui::TrigImmedPanel;
    trigImmPanelUI->setupUi( panel );

// Timed
    panel = new QWidget( trigTabUI->trigFrame );
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTimed ) );
    trigTimPanelUI = new Ui::TrigTimedPanel;
    trigTimPanelUI->setupUi( panel );
    ConnectUI( trigTimPanelUI->HInfRadio, SIGNAL(clicked()), this, SLOT(trigTimHInfClicked()) );
    ConnectUI( trigTimPanelUI->cyclesRadio, SIGNAL(clicked()), this, SLOT(trigTimHInfClicked()) );
    ConnectUI( trigTimPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigTimNInfClicked(bool)) );

    QButtonGroup    *bgTim = new QButtonGroup( panel );
    bgTim->addButton( trigTimPanelUI->HInfRadio );
    bgTim->addButton( trigTimPanelUI->cyclesRadio );

// TTL
    panel = new QWidget( trigTabUI->trigFrame );
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTTL ) );
    trigTTLPanelUI = new Ui::TrigTTLPanel;
    trigTTLPanelUI->setupUi( panel );
    ConnectUI( trigTTLPanelUI->modeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(trigTTLModeChanged(int)) );
    ConnectUI( trigTTLPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigTTLNInfClicked(bool)) );

// Spike
    panel = new QWidget( trigTabUI->trigFrame );
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigSpike ) );
    trigSpkPanelUI = new Ui::TrigSpikePanel;
    trigSpkPanelUI->setupUi( panel );
    ConnectUI( trigSpkPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigSpkNInfClicked(bool)) );

// TCP
    panel = new QWidget( trigTabUI->trigFrame );
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTCP ) );
    trigTCPPanelUI = new Ui::TrigTCPPanel;
    trigTCPPanelUI->setupUi( panel );

// -----------
// SeeNSaveTab
// -----------

    snsTabUI = new Ui::SeeNSaveTab;
    snsTabUI->setupUi( cfgUI->snsTab );
    ConnectUI( snsTabUI->chnMapBut, SIGNAL(clicked()), this, SLOT(chnMapButClicked()) );
    ConnectUI( snsTabUI->runDirBut, SIGNAL(clicked()), this, SLOT(runDirButClicked()) );

// ----------
// Initialize
// ----------

    if( CniCfg::isHardware() ) {

        CniCfg::probeAIHardware();
        CniCfg::probeAllDILines();

        reset();
        valid( startupErr );
    }
}


ConfigCtl::~ConfigCtl()
{
    if( snsTabUI ) {
        delete snsTabUI;
        snsTabUI = 0;
    }

    if( trigTCPPanelUI ) {
        delete trigTCPPanelUI;
        trigTCPPanelUI = 0;
    }

    if( trigSpkPanelUI ) {
        delete trigSpkPanelUI;
        trigSpkPanelUI = 0;
    }

    if( trigTTLPanelUI ) {
        delete trigTTLPanelUI;
        trigTTLPanelUI = 0;
    }

    if( trigTimPanelUI ) {
        delete trigTimPanelUI;
        trigTimPanelUI = 0;
    }

    if( trigImmPanelUI ) {
        delete trigImmPanelUI;
        trigImmPanelUI = 0;
    }

    if( trigTabUI ) {
        delete trigTabUI;
        trigTabUI = 0;
    }

    if( gateTCPPanelUI ) {
        delete gateTCPPanelUI;
        gateTCPPanelUI = 0;
    }

    if( gateImmPanelUI ) {
        delete gateImmPanelUI;
        gateImmPanelUI = 0;
    }

    if( gateTabUI ) {
        delete gateTabUI;
        gateTabUI = 0;
    }

    if( niCfgTabUI ) {
        delete niCfgTabUI;
        niCfgTabUI = 0;
    }

    if( cfgUI ) {
        delete cfgUI;
        cfgUI = 0;
    }

    if( cfgDlg ) {
        delete cfgDlg;
        cfgDlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ConfigCtl::showStartupMessage()
{
    if( !startupErr.isEmpty() ) {
        Error() << "DAQ.ini settings error: " << startupErr;
        QMessageBox::warning( 0, "DAQ.ini Settings Error", startupErr );
    }
}


bool ConfigCtl::showDialog()
{
    if( !CniCfg::aiDevRanges.size() ) {

        QMessageBox::critical(
            cfgDlg,
            "NIDAQ Setup Error",
            "No NIDAQ analog input devices installed.\n\n"
            "Resolve issues in NI 'Measurement & Automation Explorer'." );

       return false;
    }

    reset();

    return QDialog::Accepted == cfgDlg->exec();
}


void ConfigCtl::setRunName( const QString &name )
{
    QString strippedName = name;
    QRegExp re("(.*)_[gG](\\d+)_[tT](\\d+)$");

    if( strippedName.contains( re ) )
        strippedName = re.cap(1);

    acceptedParams.sns.runName = strippedName;
    acceptedParams.saveSettings();

    if( cfgDlg->isVisible() )
        snsTabUI->runNameLE->setText( strippedName );
}


void ConfigCtl::graphSetsImSaveBit( int chan, bool setOn )
{
}


void ConfigCtl::graphSetsNiSaveBit( int chan, bool setOn )
{
    DAQ::Params &p = acceptedParams;

    if( chan >= 0 && chan < p.ni.niCumTypCnt[CniCfg::niSumAll] ) {

        p.sns.niChans.saveBits.setBit( chan, setOn );

        p.sns.niChans.uiSaveChanStr =
            Subset::bits2RngStr( p.sns.niChans.saveBits );

        Debug()
            << "New nidq subset string: "
            << p.sns.niChans.uiSaveChanStr;

        p.saveSettings();
    }
}


// Return true if file or folder name contains any illegal
// characters: {/\[]<>*":;,.?|=}.
//
static int FILEHasIllegals( const char *name )
{
    char    c;

    while( (c = *name++) ) {

        if( c == '/' || c == ':' ||
            c == '*' || c == '[' || c == ']' ||
            c == '<' || c == '>' || c == '=' ||
            c == '?' || c == ';' || c == ',' ||
            c == '"' || c == '|' || c == '\\' ) {

            return true;
        }
    }

    return false;
}


static bool runNameExists( const QString &runName )
{
// --------------
// Seek any match
// --------------

    QRegExp         re( QString("%1_.*").arg( runName ) );
    QDirIterator    it( mainApp()->runDir() );

    re.setCaseSensitivity( Qt::CaseInsensitive );

    while( it.hasNext() ) {

        it.next();

        if( it.fileInfo().isFile()
            && re.indexIn( it.fileName() ) == 0 ) {

            return true;
        }
    }

    return false;
}


// The filenaming policy:
// Names (bin, meta) have pattern: runDir/runName_gN_tM.nidq.bin.
// The run name must be unique in runDir for formal usage.
// We will, however, warn and offer to overwrite existing
// file(s) because it is so useful for test and development.
//
bool ConfigCtl::validRunName(
    QString         &err,
    const QString   &runName,
    QWidget         *parent,
    bool            isGUI )
{
    if( runName.isEmpty() ) {
        err = "A non-empty run name is required.";
        return false;
    }

    if( FILEHasIllegals( STR2CHR( runName ) ) ) {
        err = "Run names may not contain any of {/\\[]<>*\":;,?|=}";
        return false;
    }

    QRegExp re("_[gG]\\d+_[tT]\\d+");

    if( runName.contains( re ) ) {
        err = "Run names cannot contain '_gN_tM' style indices.";
        return false;
    }

    if( !isGUI )
        return true;

    if( !runNameExists( runName ) )
        return true;

    int yesNo = QMessageBox::question(
        parent,
        "Run Name Already Exists",
        QString(
        "File set with run name '%1' already exists, overwrite it?")
        .arg( runName ),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    if( yesNo != QMessageBox::Yes )
        return false;

    return true;
}


// Space-separated list of current saved chans.
// Used for remote GETCHANNELSUBSET command.
//
QString ConfigCtl::cmdSrvGetsSaveChans()
{
// BK: Case out for imec/nidq

    QString         s;
    QTextStream     ts( &s, QIODevice::WriteOnly );
    const QBitArray &B = acceptedParams.sns.niChans.saveBits;
    int             nb = B.size();

    for( int i = 0; i < nb; ++i ) {

        if( B.testBit( i ) )
            ts << i << " ";
    }

    ts << "\n";

    return s;
}


// Used for remote GETPARAMS command.
//
QString ConfigCtl::cmdSrvGetsParamStr()
{
    return DAQ::Params::settings2Str();
}


// Return QString::null or error string.
// Used for remote SETPARAMS command.
//
QString ConfigCtl::cmdSrvSetsParamStr( const QString &str )
{
// -------------------------------
// Save settings to "_remote" file
// -------------------------------

// first write current set

    acceptedParams.saveSettings( true );

// then overwrite entries

    DAQ::Params::str2RemoteSettings( str );

// -----------------------
// Transfer them to dialog
// -----------------------

    DAQ::Params p;

    reset( &p );

// --------------------------
// Remote-specific validation
// --------------------------

// With a dialog, user is constrained to choose items
// we've put into CB controls. Remote case lacks that
// constraint, so we check existence of CB items here.

    if( p.ni.dev1 != devNames[niCfgTabUI->device1CB->currentIndex()] ) {

        return QString("Device [%1] does not support AI.")
                .arg( p.ni.dev1 );
    }

    if( p.ni.dev2 != devNames[niCfgTabUI->device2CB->currentIndex()] ) {

        return QString("Device [%1] does not support AI.")
                .arg( p.ni.dev2 );
    }

    if( p.ni.clockStr1 != niCfgTabUI->clk1CB->currentText() ) {

        return QString("Clock [%1] not supported on device [%2].")
                .arg( p.ni.clockStr1 )
                .arg( p.ni.dev1 );
    }

    if( p.ni.clockStr2 != niCfgTabUI->clk2CB->currentText() ) {

        return QString("Clock [%1] not supported on device [%2].")
                .arg( p.ni.clockStr2 )
                .arg( p.ni.dev2 );
    }

    QString rng = QString("[%1, %2]")
                    .arg( p.ni.range.rmin )
                    .arg( p.ni.range.rmax );

    if( rng != niCfgTabUI->aiRangeCB->currentText() ) {

        return QString("Range %1 not supported on device [%2].")
                .arg( rng )
                .arg( p.ni.dev1 );
    }

// -------------------
// Standard validation
// -------------------

    QString err;

    if( !valid( err ) ) {

        err = QString("ACQ Parameter Error [%1]")
                .arg( err.replace( "\n", " " ) );
    }

    return err;
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ConfigCtl::device1CBChanged()
{
    if( !niCfgTabUI->device1CB->count() )
        return;

    QString devStr = devNames[CURDEV1];

// --------
// AI range
// --------

    {
        QComboBox       *CB     = niCfgTabUI->aiRangeCB;
        QString         rngCur;
        QList<VRange>   rngL    = CniCfg::aiDevRanges.values( devStr );
        int             nL      = rngL.size(),
                        sel     = 0;

        // If rangeCB is non-empty, that is, if user has ever
        // seen it before, then we will reselect the current
        // choice. Otherwise, we'll try to select last saved.

        if( CB->count() )
            rngCur = CB->currentText();
        else {
            rngCur = QString("[%1, %2]")
                    .arg( acceptedParams.ni.range.rmin )
                    .arg( acceptedParams.ni.range.rmax );
        }

        CB->clear();

        for( int i = 0; i < nL; ++i ) {

            const VRange    &r  = rngL[i];
            QString         s   = QString("[%1, %2]")
                                    .arg( r.rmin )
                                    .arg( r.rmax );

            if( s == rngCur )
                sel = i;

            CB->insertItem( i, s );
        }

        CB->setCurrentIndex( sel );
    }

// --------------------
// Set up Dev1 clock CB
// --------------------

    {
        niCfgTabUI->clk1CB->clear();
        niCfgTabUI->clk1CB->addItem( "Internal" );

        QStringList pfiStrL = CniCfg::getPFIChans( devStr );
        int         npfi    = pfiStrL.count(),
                    pfiSel  = 0;

        // Note on QString::section() params:
        //
        // ("/dev/PFI").section('/',-1,-1)
        // -> ('/'=sep, -1=last field on right, -1=to end)
        // -> "PFI"

        for( int i = 0; i < npfi; ++i ) {

            QString	s = pfiStrL[i].section( '/', -1, -1 );

            niCfgTabUI->clk1CB->addItem( s );

            if( s == acceptedParams.ni.clockStr1 )
                pfiSel = i + 1;
        }

        niCfgTabUI->clk1CB->setCurrentIndex( pfiSel );
    }

// ----------------------
// AI sample rate spinner
// ----------------------

    double  minRate =
                std::max( CniCfg::minSampleRate( devStr ), 100.0 ),
            maxRate =
                std::min( CniCfg::maxSampleRate( devStr ), 100000.0 );

    niCfgTabUI->srateSB->setMinimum( minRate );
    niCfgTabUI->srateSB->setMaximum( maxRate );

// ----
// Sync
// ----

    if( isMuxingFromDlg() ) {

        niCfgTabUI->syncCB->setCurrentIndex(
            niCfgTabUI->syncCB->findText(
                QString("%1/port0/line0").arg( devStr ) ) );
    }
}


void ConfigCtl::device2CBChanged()
{
// --------------------
// Set up Dev2 clock CB
// --------------------

    niCfgTabUI->clk2CB->clear();

    if( !niCfgTabUI->device2CB->count() ) {

noPFI:
        niCfgTabUI->clk2CB->addItem( "PFI2" );
        niCfgTabUI->clk2CB->setCurrentIndex( 0 );
        return;
    }

    QStringList pfiStrL = CniCfg::getPFIChans( devNames[CURDEV2] );
    int         npfi    = pfiStrL.count(),
                pfiSel  = 0;

    if( !npfi )
        goto noPFI;

// Note on QString::section() params:
//
// ("/dev/PFI").section('/',-1,-1)
// -> ('/'=sep, -1=last field on right, -1=to end)
// -> "PFI"

    for( int i = 0; i < npfi; ++i ) {

        QString	s = pfiStrL[i].section( '/', -1, -1 );

        niCfgTabUI->clk2CB->addItem( s );

        if( s == acceptedParams.ni.clockStr2 )
            pfiSel = i;
    }

    niCfgTabUI->clk2CB->setCurrentIndex( pfiSel );
}


void ConfigCtl::muxingChanged()
{
    bool    isMux = isMuxingFromDlg();

    if( isMux ) {

        int ci = niCfgTabUI->clk1CB->findText( "PFI2", Qt::MatchExactly );
        niCfgTabUI->clk1CB->setCurrentIndex( ci > -1 ? ci : 0 );

        niCfgTabUI->syncEnabChk->setChecked( true );

        ci = niCfgTabUI->syncCB->findText(
                QString("%1/port0/line0").arg( devNames[CURDEV1] ) );
        niCfgTabUI->syncCB->setCurrentIndex( ci > -1 ? ci : 0 );
    }
    else {

        bool    wasMux = acceptedParams.ni.isMuxingMode();

        if( wasMux )
            snsTabUI->saveChansLE->setText( "all" );
    }

    niCfgTabUI->clk1CB->setDisabled( isMux );
    niCfgTabUI->syncEnabChk->setDisabled( isMux );
    niCfgTabUI->syncCB->setDisabled( isMux );
}


void ConfigCtl::aiRangeChanged()
{
    QString             devStr  = devNames[CURDEV1];
    const QList<VRange> rngL    = CniCfg::aiDevRanges.values( devStr );

    if( !rngL.count() ) {

        QMessageBox::critical(
            cfgDlg,
            "NI Unknown Error",
            "Error with your NIDAQ setup."
            "  Please make sure all ghost/phantom/unused devices"
            " are deleted from NI Measurement & Autiomation Explorer",
            QMessageBox::Abort );

        QApplication::exit( 1 );
        return;
    }

    VRange  r = rngL[niCfgTabUI->aiRangeCB->currentIndex()];

    trigTTLPanelUI->TSB->setMinimum( r.rmin );
    trigTTLPanelUI->TSB->setMaximum( r.rmax );

    trigSpkPanelUI->TSB->setMinimum( r.rmin );
    trigSpkPanelUI->TSB->setMaximum( r.rmax );
}


void ConfigCtl::clk1CBChanged()
{
    niCfgTabUI->freqBut->setEnabled( niCfgTabUI->clk1CB->currentIndex() != 0 );
}


void ConfigCtl::freqButClicked()
{
    QString txt = niCfgTabUI->freqBut->text();

    niCfgTabUI->freqBut->setText( "Sampling; hold on..." );
    niCfgTabUI->freqBut->repaint();

    double  f = CniCfg::sampleFreq(
                    devNames[CURDEV1],
                    niCfgTabUI->clk1CB->currentText(),
                    niCfgTabUI->syncCB->currentText() );

    niCfgTabUI->freqBut->setText( txt );

    if( !f ) {

        QMessageBox::critical(
            cfgDlg,
            "Frequency Measurement Failed",
            "The measured sample rate is zero...check power supply and cables." );
        return;
    }

    if( isMuxingFromDlg() )
        f /= niCfgTabUI->muxFactorSB->value();

    double  vMin = niCfgTabUI->srateSB->minimum(),
            vMax = niCfgTabUI->srateSB->maximum();

    if( f < vMin || f > vMax ) {

        QMessageBox::warning(
            cfgDlg,
            "Value Outside Range",
            QString("The measured sample rate is [%1].\n\n"
            "The current system is limited to range [%2..%3],\n"
            "so you must use a different clock source or rate.")
            .arg( f ).arg( vMin ).arg( vMax ) );
    }

    niCfgTabUI->srateSB->setValue( f );
}


void ConfigCtl::syncEnableClicked( bool checked )
{
    niCfgTabUI->syncCB->setEnabled( checked && !isMuxingFromDlg() );
}


void ConfigCtl::gateModeChanged()
{
    int     mode    = gateTabUI->gateModeCB->currentIndex();
    QString wName   = QString("panel_%1").arg( mode );

#if QT_VERSION >= 0x050300
    QList<QWidget*> wL =
        gateTabUI->gateFrame->findChildren<QWidget*>(
            QRegExp("panel_*"),
            Qt::FindDirectChildrenOnly );
#else
    QList<QWidget*> wL =
        gateTabUI->gateFrame->findChildren<QWidget*>(
            QRegExp("panel_*") );
#endif

    foreach( QWidget* w, wL ) {

        if( w->objectName() == wName )
            w->show();
        else
            w->hide();
    }
}


void ConfigCtl::trigModeChanged()
{
    int     mode    = trigTabUI->trigModeCB->currentIndex();
    QString wName   = QString("panel_%1").arg( mode );

#if QT_VERSION >= 0x050300
    QList<QWidget*> wL =
        trigTabUI->trigFrame->findChildren<QWidget*>(
            QRegExp("panel_*"),
            Qt::FindDirectChildrenOnly );
#else
    QList<QWidget*> wL =
        trigTabUI->trigFrame->findChildren<QWidget*>(
            QRegExp("panel_*") );
#endif

    foreach( QWidget* w, wL ) {

        if( w->objectName() == wName )
            w->show();
        else
            w->hide();
    }
}


void ConfigCtl::chnMapButClicked()
{
// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    QVector<uint>   vcMN1, vcMA1, vcXA1, vcXD1,
                    vcMN2, vcMA2, vcXA2, vcXD2;
    CniCfg          ni;

    if( !Subset::rngStr2Vec( vcMN1, niCfgTabUI->mn1LE->text() )
        || !Subset::rngStr2Vec( vcMA1, niCfgTabUI->ma1LE->text() )
        || !Subset::rngStr2Vec( vcXA1, niCfgTabUI->xa1LE->text() )
        || !Subset::rngStr2Vec( vcXD1, niCfgTabUI->xd1LE->text() )
        || !Subset::rngStr2Vec( vcMN2, uiMNStr2FromDlg() )
        || !Subset::rngStr2Vec( vcMA2, uiMAStr2FromDlg() )
        || !Subset::rngStr2Vec( vcXA2, uiXAStr2FromDlg() )
        || !Subset::rngStr2Vec( vcXD2, uiXDStr2FromDlg() ) ) {

        QMessageBox::critical(
            cfgDlg,
            "ChanMap Parameter Error",
            "Bad format in one or more NI-DAQ channel strings." );
        return;
    }

    ni.uiMNStr1         = Subset::vec2RngStr( vcMN1 );
    ni.uiMAStr1         = Subset::vec2RngStr( vcMA1 );
    ni.uiXAStr1         = Subset::vec2RngStr( vcXA1 );
    ni.uiXDStr1         = Subset::vec2RngStr( vcXD1 );
    ni.setUIMNStr2( Subset::vec2RngStr( vcMN2 ) );
    ni.setUIMAStr2( Subset::vec2RngStr( vcMA2 ) );
    ni.setUIXAStr2( Subset::vec2RngStr( vcXA2 ) );
    ni.setUIXDStr2( Subset::vec2RngStr( vcXD2 ) );
    ni.muxFactor        = niCfgTabUI->muxFactorSB->value();
    ni.isDualDevMode    = niCfgTabUI->dev2GB->isChecked();

    ni.deriveChanCounts();

    const int   *type = ni.niCumTypCnt;

    ChanMapNI defMap(
        0, 0,
        type[CniCfg::niTypeMN] / ni.muxFactor,
        (type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN]) / ni.muxFactor,
        ni.muxFactor,
        type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA],
        type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

// -------------
// Launch editor
// -------------

    ChanMapCtl  CM( cfgDlg, defMap );

    QString mapFile = CM.Edit( snsTabUI->chnMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        snsTabUI->chnMapLE->setText( "*Default (Acquired order)" );
    else
        snsTabUI->chnMapLE->setText( mapFile );
}


void ConfigCtl::runDirButClicked()
{
    MainApp *app = mainApp();

    app->options_PickRunDir();
    snsTabUI->runDirLbl->setText( app->runDir() );
}


void ConfigCtl::reset( DAQ::Params *pRemote )
{
// ---------
// Shortcuts
// ---------

    QComboBox   *CB1, *CB2;

// ---------
// Get state
// ---------

    DAQ::Params &p = (pRemote ? *pRemote : acceptedParams);

    p.loadSettings( pRemote != 0 );

// -------
// NIInput
// -------

    niCfgTabUI->srateSB->setValue( p.ni.srate );
    niCfgTabUI->mnGainSB->setValue( p.ni.mnGain );
    niCfgTabUI->maGainSB->setValue( p.ni.maGain );

// Devices

    CB1 = niCfgTabUI->device1CB;
    CB2 = niCfgTabUI->device2CB;

    devNames.clear();
    CB1->clear();
    CB2->clear();

    {
        QList<QString>  devs    = CniCfg::aiDevChanCount.uniqueKeys();
        int             sel     = 0,
                        sel2    = 0;

        foreach( const QString &D, devs ) {

            QString s = QString("%1 (%2)")
                        .arg( D )
                        .arg( CniCfg::getProductName( D ) );

            CB1->addItem( s );
            CB2->addItem( s );

            devNames.push_back( D );

            if( D == p.ni.dev1 )
                sel = CB1->count() - 1;

            if( D == p.ni.dev2 )
                sel2 = CB2->count() - 1;
        }

        if( CB1->count() )
            CB1->setCurrentIndex( sel );

        if( CB2->count() )
            CB2->setCurrentIndex( sel2 );
    }

// Clocks (See device1CBChanged & device2CBChanged)

// Channels

    niCfgTabUI->mn1LE->setText( p.ni.uiMNStr1 );
    niCfgTabUI->ma1LE->setText( p.ni.uiMAStr1 );
    niCfgTabUI->xa1LE->setText( p.ni.uiXAStr1 );
    niCfgTabUI->xd1LE->setText( p.ni.uiXDStr1 );
    niCfgTabUI->mn2LE->setText( p.ni.uiMNStr2Bare() );
    niCfgTabUI->ma2LE->setText( p.ni.uiMAStr2Bare() );
    niCfgTabUI->xa2LE->setText( p.ni.uiXAStr2Bare() );
    niCfgTabUI->xd2LE->setText( p.ni.uiXDStr2Bare() );

// Termination choices loaded in form data

    {
        int ci = niCfgTabUI->aiTerminationCB->findText(
                    CniCfg::termConfigToString( p.ni.termCfg ),
                    Qt::MatchExactly );

        niCfgTabUI->aiTerminationCB->setCurrentIndex( ci > -1 ? ci : 0 );
    }

    niCfgTabUI->muxFactorSB->setValue( p.ni.muxFactor );

    if( CB2->count() > 1 ) {
        niCfgTabUI->dev2GB->setChecked( p.ni.isDualDevMode );
        niCfgTabUI->dev2GB->setEnabled( true );
    }
    else {
        niCfgTabUI->dev2GB->setChecked( false );
        niCfgTabUI->dev2GB->setEnabled( false );
        p.ni.isDualDevMode = false;
    }

// Sync

    niCfgTabUI->syncEnabChk->setChecked( p.ni.syncEnable );
    niCfgTabUI->syncCB->clear();

    {
        QStringList L  = CniCfg::getAllDOLines();
        int         sel;

        foreach( QString s, L )
            niCfgTabUI->syncCB->addItem( s );

        sel = niCfgTabUI->syncCB->findText( p.ni.syncLine );

        if( sel < 0 ) {
            sel = niCfgTabUI->syncCB->findText(
                    QString("%1/port0/line0").arg( devNames[CURDEV1] ) );
        }

        niCfgTabUI->syncCB->setCurrentIndex( sel );
    }

// --------
// DOParams
// --------

// ------------
// TrgTimParams
// ------------

    trigTimPanelUI->L0SB->setValue( p.trgTim.tL0 );
    trigTimPanelUI->HSB->setValue( p.trgTim.tH );
    trigTimPanelUI->LSB->setValue( p.trgTim.tL );
    trigTimPanelUI->NSB->setValue( p.trgTim.nH );
    trigTimPanelUI->HInfRadio->setChecked( p.trgTim.isHInf );
    trigTimPanelUI->cyclesRadio->setChecked( !p.trgTim.isHInf );
    trigTimPanelUI->NInfChk->setChecked( p.trgTim.isNInf );

// ------------
// TrgTTLParams
// ------------

    trigTTLPanelUI->marginSB->setValue( p.trgTTL.marginSecs );
    trigTTLPanelUI->refracSB->setValue( p.trgTTL.refractSecs );
    trigTTLPanelUI->HSB->setValue( p.trgTTL.tH );
    trigTTLPanelUI->modeCB->setCurrentIndex( p.trgTTL.mode );
    trigTTLPanelUI->chanSB->setValue( p.trgTTL.aiChan );
    trigTTLPanelUI->inarowSB->setValue( p.trgTTL.inarow );
    trigTTLPanelUI->NSB->setValue( p.trgTTL.nH );
    trigTTLPanelUI->NInfChk->setChecked( p.trgTTL.isNInf );

    // Voltage V in range [L,U] and with gain G, is scaled to
    // a signed 16-bit stored value T as follows:
    //
    // T = 65K * (G*V - L)/(U - L) - 32K, so,
    //
    // V = [(T + 32K)/65K * (U - L) + L] / G
    //

    {
        double  V;

        V  = (p.trgTTL.T + 32768.0) / 65535.0;
        V  = p.ni.range.unityToVolts( V );
        V /= p.ni.chanGain( p.trgTTL.aiChan );

        trigTTLPanelUI->TSB->setValue( V );
    }

// --------------
// TrgSpikeParams
// --------------

    trigSpkPanelUI->periSB->setValue( p.trgSpike.periEvtSecs );
    trigSpkPanelUI->refracSB->setValue( p.trgSpike.refractSecs );
    trigSpkPanelUI->chanSB->setValue( p.trgSpike.aiChan );
    trigSpkPanelUI->inarowSB->setValue( p.trgSpike.inarow );
    trigSpkPanelUI->NSB->setValue( p.trgSpike.nS );
    trigSpkPanelUI->NInfChk->setChecked( p.trgSpike.isNInf );

    // Voltage V in range [L,U] and with gain G, is scaled to
    // a signed 16-bit stored value T as follows:
    //
    // T = 65K * (G*V - L)/(U - L) - 32K, so,
    //
    // V = [(T + 32K)/65K * (U - L) + L] / G
    //

    {
        double  V;

        V  = (p.trgSpike.T + 32768.0) / 65535.0;
        V  = p.ni.range.unityToVolts( V );
        V /= p.ni.chanGain( p.trgSpike.aiChan );

        trigSpkPanelUI->TSB->setValue( V );
    }

// ----------
// ModeParams
// ----------

    gateTabUI->gateModeCB->setCurrentIndex( (int)p.mode.mGate );
    trigTabUI->trigModeCB->setCurrentIndex( (int)p.mode.mTrig );
    trigTabUI->trgInitDisabChk->setChecked( p.mode.trgInitiallyOff );

// --------
// SeeNSave
// --------

// Nidq

    if( p.sns.niChans.chanMapFile.contains( "*" ) )
        p.sns.niChans.chanMapFile.clear();

    if( p.sns.niChans.chanMapFile.isEmpty() )
        snsTabUI->chnMapLE->setText( "*Default (Acquired order)" );
    else
        snsTabUI->chnMapLE->setText( p.sns.niChans.chanMapFile );

// Imec

// BK: Need imec complement when have GUI

    CB1 = snsTabUI->graphsPerTabCB;

    {
        int n   = CB1->count(),
            sel = 0;

        for( int i = 1; i < n; ++i ) {

            if( CB1->itemText( i ).toInt() == p.sns.maxGrfPerTab ) {
                sel = i;
                break;
            }
        }

        CB1->setCurrentIndex( sel );
    }

    snsTabUI->disableGraphsChk->setChecked( p.sns.hideGraphs );

 // BK: Need imec complement when have GUI
    snsTabUI->saveChansLE->setText( p.sns.niChans.uiSaveChanStr );

    snsTabUI->runDirLbl->setText( mainApp()->runDir() );
    snsTabUI->runNameLE->setText( p.sns.runName );

// --------------------
// Observe dependencies
// --------------------

    device1CBChanged(); // <-- Call This First!! - Fills in other CBs
    device2CBChanged();
    muxingChanged();
    aiRangeChanged();
    clk1CBChanged();
    syncEnableClicked( p.ni.syncEnable );
    gateModeChanged();
    trigModeChanged();
    trigTimHInfClicked();
    trigTimNInfClicked( p.trgTim.isNInf );
    trigTTLModeChanged( p.trgTTL.mode );
    trigTTLNInfClicked( p.trgTTL.isNInf );
    trigSpkNInfClicked( p.trgSpike.isNInf );
}


void ConfigCtl::verify()
{
    QString err;

    if( valid( err, true ) )
        ;
    else if( !err.isEmpty() )
        QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
}


void ConfigCtl::okBut()
{
    QString err;

    if( valid( err, true ) )
        cfgDlg->accept();
    else if( !err.isEmpty() )
        QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
}


void ConfigCtl::trigTimHInfClicked()
{
    trigTimPanelUI->cyclesGB->setEnabled(
        trigTimPanelUI->cyclesRadio->isChecked() );
}


void ConfigCtl::trigTimNInfClicked( bool checked )
{
    trigTimPanelUI->NLabel->setDisabled( checked );
    trigTimPanelUI->NSB->setDisabled( checked );
}


void ConfigCtl::trigTTLModeChanged( int _mode )
{
    DAQ::TrgTTLMode mode    = DAQ::TrgTTLMode(_mode);
    QString         txt;

    switch( mode ) {

        case DAQ::TrgTTLLatch:
            txt = "writing continues until gate closes.";
            break;
        case DAQ::TrgTTLTimed:
            txt = "writing continues this many seconds";
            break;
        case DAQ::TrgTTLFollowAI:
            txt = "writing continues while voltage is high.";
            break;
    }

    trigTTLPanelUI->HLabel->setText( txt );
    trigTTLPanelUI->HSB->setVisible( mode == DAQ::TrgTTLTimed );
    trigTTLPanelUI->repeatGB->setHidden( mode == DAQ::TrgTTLLatch );
}


void ConfigCtl::trigTTLNInfClicked( bool checked )
{
    trigTTLPanelUI->NLabel->setDisabled( checked );
    trigTTLPanelUI->NSB->setDisabled( checked );
}


void ConfigCtl::trigSpkNInfClicked( bool checked )
{
    trigSpkPanelUI->NLabel->setDisabled( checked );
    trigSpkPanelUI->NSB->setDisabled( checked );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString ConfigCtl::uiMNStr2FromDlg()
{
    return (niCfgTabUI->dev2GB->isChecked() ?
            niCfgTabUI->mn2LE->text() : "");
}


QString ConfigCtl::uiMAStr2FromDlg()
{
    return (niCfgTabUI->dev2GB->isChecked() ?
            niCfgTabUI->ma2LE->text() : "");
}


QString ConfigCtl::uiXAStr2FromDlg()
{
    return (niCfgTabUI->dev2GB->isChecked() ?
            niCfgTabUI->xa2LE->text() : "");
}


QString ConfigCtl::uiXDStr2FromDlg()
{
    return (niCfgTabUI->dev2GB->isChecked() ?
            niCfgTabUI->xd2LE->text() : "");
}


bool ConfigCtl::isMuxingFromDlg()
{
    return  !niCfgTabUI->mn1LE->text().isEmpty()
            || !niCfgTabUI->ma1LE->text().isEmpty()
            || (niCfgTabUI->dev2GB->isChecked()
                && (!niCfgTabUI->mn2LE->text().isEmpty()
                    || !niCfgTabUI->ma2LE->text().isEmpty())
                );
}


void ConfigCtl::paramsFromDialog(
    DAQ::Params     &q,
    QVector<uint>   &vcMN1,
    QVector<uint>   &vcMA1,
    QVector<uint>   &vcXA1,
    QVector<uint>   &vcXD1,
    QVector<uint>   &vcMN2,
    QVector<uint>   &vcMA2,
    QVector<uint>   &vcXA2,
    QVector<uint>   &vcXD2,
    QString         &uiStr1Err,
    QString         &uiStr2Err )
{
// ----
// IMEC
// ----

// BK: For now
    q.im = acceptedParams.im;

    QString error;
    q.im.deriveChanCounts();
    q.sns.imChans.deriveSaveBits( error, q.im.imCumTypCnt[CimCfg::imSumAll] );
    q.sns.imChans.chanMap = ChanMapIM(
        q.im.imCumTypCnt[CimCfg::imSumAP],
        q.im.imCumTypCnt[CimCfg::imSumNeural] - q.im.imCumTypCnt[CimCfg::imSumAP],
        q.im.imCumTypCnt[CimCfg::imSumAll] - q.im.imCumTypCnt[CimCfg::imSumNeural] );
    q.sns.imChans.chanMap.fillDefault();

// BK: no imec on github, yet
    q.im.enabled = false;

// BK: Need gui
    q.ni.enabled = true;

// ----
// NIDQ
// ----

    if( !Subset::rngStr2Vec( vcMN1, niCfgTabUI->mn1LE->text() ) )
        uiStr1Err = "MN-chans";

    if( !Subset::rngStr2Vec( vcMA1, niCfgTabUI->ma1LE->text() ) ) {
        uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
        uiStr1Err += "MA-chans";
    }

    if( !Subset::rngStr2Vec( vcXA1, niCfgTabUI->xa1LE->text() ) ) {
        uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
        uiStr1Err += "XA-chans";
    }

    if( !Subset::rngStr2Vec( vcXD1, niCfgTabUI->xd1LE->text() ) ) {
        uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
        uiStr1Err += "XD-chans";
    }

    if( !Subset::rngStr2Vec( vcMN2, niCfgTabUI->mn2LE->text() ) )
        uiStr2Err = "MN-chans";

    if( !Subset::rngStr2Vec( vcMA2, niCfgTabUI->ma2LE->text() ) ) {
        uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
        uiStr2Err += "MA-chans";
    }

    if( !Subset::rngStr2Vec( vcXA2, niCfgTabUI->xa2LE->text() ) ) {
        uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
        uiStr2Err += "XA-chans";
    }

    if( !Subset::rngStr2Vec( vcXD2, niCfgTabUI->xd2LE->text() ) ) {
        uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
        uiStr2Err += "XD-chans";
    }

    q.ni.dev1 =
    (niCfgTabUI->device1CB->count() ? devNames[CURDEV1] : "");

    q.ni.dev2 =
    (niCfgTabUI->device2CB->count() ? devNames[CURDEV2] : "");

    if( niCfgTabUI->device1CB->count() ) {

        q.ni.range =
        CniCfg::aiDevRanges.values( q.ni.dev1 )
        [niCfgTabUI->aiRangeCB->currentIndex()];
    }

    q.ni.clockStr1     = niCfgTabUI->clk1CB->currentText();
    q.ni.clockStr2     = niCfgTabUI->clk2CB->currentText();
    q.ni.srate         = niCfgTabUI->srateSB->value();
    q.ni.mnGain        = niCfgTabUI->mnGainSB->value();
    q.ni.maGain        = niCfgTabUI->maGainSB->value();
    q.ni.uiMNStr1      = Subset::vec2RngStr( vcMN1 );
    q.ni.uiMAStr1      = Subset::vec2RngStr( vcMA1 );
    q.ni.uiXAStr1      = Subset::vec2RngStr( vcXA1 );
    q.ni.uiXDStr1      = Subset::vec2RngStr( vcXD1 );
    q.ni.setUIMNStr2( Subset::vec2RngStr( vcMN2 ) );
    q.ni.setUIMAStr2( Subset::vec2RngStr( vcMA2 ) );
    q.ni.setUIXAStr2( Subset::vec2RngStr( vcXA2 ) );
    q.ni.setUIXDStr2( Subset::vec2RngStr( vcXD2 ) );
    q.ni.syncLine      = niCfgTabUI->syncCB->currentText();
    q.ni.muxFactor     = niCfgTabUI->muxFactorSB->value();

    q.ni.termCfg =
    q.ni.stringToTermConfig( niCfgTabUI->aiTerminationCB->currentText() );

    q.ni.isDualDevMode  = niCfgTabUI->dev2GB->isChecked();
    q.ni.syncEnable     = niCfgTabUI->syncEnabChk->isChecked();

    q.ni.deriveChanCounts();

// --------
// DOParams
// --------

// ------------
// TrgTimParams
// ------------

    q.trgTim.tL0    = trigTimPanelUI->L0SB->value();
    q.trgTim.tH     = trigTimPanelUI->HSB->value();
    q.trgTim.tL     = trigTimPanelUI->LSB->value();
    q.trgTim.nH     = trigTimPanelUI->NSB->value();
    q.trgTim.isHInf = trigTimPanelUI->HInfRadio->isChecked();
    q.trgTim.isNInf = trigTimPanelUI->NInfChk->isChecked();

// ------------
// TrgTTLParams
// ------------

    q.trgTTL.marginSecs     = trigTTLPanelUI->marginSB->value();
    q.trgTTL.refractSecs    = trigTTLPanelUI->refracSB->value();
    q.trgTTL.tH             = trigTTLPanelUI->HSB->value();
    q.trgTTL.mode           = trigTTLPanelUI->modeCB->currentIndex();
    q.trgTTL.aiChan         = trigTTLPanelUI->chanSB->value();
    q.trgTTL.inarow         = trigTTLPanelUI->inarowSB->value();
    q.trgTTL.nH             = trigTTLPanelUI->NSB->value();
    q.trgTTL.isNInf         = trigTTLPanelUI->NInfChk->isChecked();

    // Voltage V in range [L,U] and with gain G, is scaled to
    // a signed 16-bit stored value T as follows:
    //
    // T = 65K * (G*V - L)/(U - L) - 32K
    //

    {
        double  T;

        T = q.ni.chanGain( q.trgTTL.aiChan ) * trigTTLPanelUI->TSB->value();
        T = q.ni.range.voltsToUnity( T );

        q.trgTTL.T = qint16(65535.0 * T - 32768.0);
    }

// --------------
// TrgSpikeParams
// --------------

    q.trgSpike.periEvtSecs  = trigSpkPanelUI->periSB->value();
    q.trgSpike.refractSecs  = trigSpkPanelUI->refracSB->value();
    q.trgSpike.aiChan       = trigSpkPanelUI->chanSB->value();
    q.trgSpike.inarow       = trigSpkPanelUI->inarowSB->value();
    q.trgSpike.nS           = trigSpkPanelUI->NSB->value();
    q.trgSpike.isNInf       = trigSpkPanelUI->NInfChk->isChecked();

    // Voltage V in range [L,U] and with gain G, is scaled to
    // a signed 16-bit stored value T as follows:
    //
    // T = 65K * (G*V - L)/(U - L) - 32K
    //

    {
        double  T;

        T = q.ni.chanGain( q.trgSpike.aiChan ) * trigSpkPanelUI->TSB->value();
        T = q.ni.range.voltsToUnity( T );

        q.trgSpike.T = qint16(65535.0 * T - 32768.0);
    }

// ----------
// ModeParams
// ----------

    q.mode.mGate            = (DAQ::GateMode)gateTabUI->gateModeCB->currentIndex();
    q.mode.mTrig            = (DAQ::TrigMode)trigTabUI->trigModeCB->currentIndex();
    q.mode.trgInitiallyOff  = trigTabUI->trgInitDisabChk->isChecked();

// --------
// SeeNSave
// --------

// BK: Need imec complement

    q.sns.niChans.chanMapFile   = snsTabUI->chnMapLE->text().trimmed();
    q.sns.niChans.uiSaveChanStr = snsTabUI->saveChansLE->text();

    q.sns.maxGrfPerTab  = snsTabUI->graphsPerTabCB->currentText().toUInt();
    q.sns.hideGraphs    = snsTabUI->disableGraphsChk->isChecked();

    q.sns.runName       = snsTabUI->runNameLE->text().trimmed();
}


bool ConfigCtl::validNiDevices( QString &err, DAQ::Params &q )
{
// ----
// Dev1
// ----

    if( !CniCfg::aiDevRanges.size()
        || !q.ni.dev1.length() ) {

        err =
        "No NIDAQ analog input devices installed.\n\n"
        "Resolve issues in NI 'Measurement & Automation Explorer'.";
    }

// ----
// Dev2
// ----

    if( !q.ni.isDualDevMode )
        return true;

    if( !q.ni.dev2.length() ) {

        err =
        "No NIDAQ analog input devices installed.\n\n"
        "Resolve issues in NI 'Measurement & Automation Explorer'.";
    }

    if( !q.ni.dev2.compare( q.ni.dev1, Qt::CaseInsensitive ) ) {

        err =
        "Device 1 and 2 cannot be same if dual-device mode selected.";
        return false;
    }

// BK: For now dualDev requires same board model because we
// offer only shared range choices.

    if( CniCfg::getProductName( q.ni.dev2 )
            .compare(
                CniCfg::getProductName( q.ni.dev1 ),
                Qt::CaseInsensitive ) ) {

        err =
        "Device 1 and 2 must be same model for dual-device operation.";
        return false;
    }

    return true;
}


bool ConfigCtl::validNiChannels(
    QString         &err,
    DAQ::Params     &q,
    QVector<uint>   &vcMN1,
    QVector<uint>   &vcMA1,
    QVector<uint>   &vcXA1,
    QVector<uint>   &vcXD1,
    QVector<uint>   &vcMN2,
    QVector<uint>   &vcMA2,
    QVector<uint>   &vcXA2,
    QVector<uint>   &vcXD2,
    QString         &uiStr1Err,
    QString         &uiStr2Err )
{
    uint    maxAI,
            maxDI;
    int     nAI,
            nDI;

// ----
// Dev1
// ----

// previous parsing error?

    if( !uiStr1Err.isEmpty() ) {
        err =
        QString(
        "Error in fields [%1].\n"
        "Valid device 1 NI-DAQ channel strings look like"
        " \"0,1,2,3 or 0:3,5,6.\"")
        .arg( uiStr1Err );
        return false;
    }

// no channels?

    nAI = vcMN1.size() + vcMA1.size() + vcXA1.size();
    nDI = vcXD1.size();

    if( !(nAI + nDI) ) {
        err = "Need at least 1 channel in device 1 NI-DAQ channel set.";
        return false;
    }

// illegal channels?

    maxAI = CniCfg::aiDevChanCount[q.ni.dev1] - 1;
    maxDI = CniCfg::diDevLineCount[q.ni.dev1] - 1;

    if( (vcMN1.count() && vcMN1.last() > maxAI)
        || (vcMA1.count() && vcMA1.last() > maxAI)
        || (vcXA1.count() && vcXA1.last() > maxAI) ) {

        err =
        QString("Device 1 AI channel values must not exceed [%1].")
        .arg( maxAI );
        return false;
    }

    if( vcXD1.count() && vcXD1.last() > maxDI ) {

        err =
        QString("Device 1 DI line values must not exceed [%1].")
        .arg( maxDI );
        return false;
    }

// ai ranges overlap?

    if( vcMN1.count() ) {

        if( (vcMA1.count() && vcMA1.first() <= vcMN1.last())
            || (vcXA1.count() && vcXA1.first() <= vcMN1.last()) ) {

            err = "Device 1 NI-DAQ channel ranges must not overlap.";
            return false;
        }
    }

    if( vcMA1.count() ) {

        if( vcXA1.count() && vcXA1.first() <= vcMA1.last() ) {

            err = "Device 1 NI-DAQ channel ranges must not overlap.";
            return false;
        }
    }

// sync line can not be digital input

    if( q.ni.syncEnable && vcXD1.count() ) {

        QString dev;
        int     line;
        CniCfg::parseDIStr( dev, line, q.ni.syncLine );

        if( dev == q.ni.dev1 && vcXD1.contains( line ) ) {

            err =
            "Sync output line cannot be used as a digital input line.";
            return false;
        }
    }

// too many ai channels?

    if( nAI > 1 && !CniCfg::supportsAISimultaneousSampling( q.ni.dev1 ) ) {

        err =
        QString(
        "Device [%1] does not support simultaneous sampling"
        " of multiple analog input channels.")
        .arg( q.ni.dev1 );
        return false;
    }

    if( q.ni.srate > CniCfg::maxSampleRate( q.ni.dev1, nAI ) ) {

        err =
        QString(
        "Sampling rate [%1] is too high for device 1 channel count (%d).")
        .arg( q.ni.srate )
        .arg( nAI );
        return false;
    }

// ----
// Dev2
// ----

    if( !q.ni.isDualDevMode )
        return true;

// previous parsing error?

    if( !uiStr2Err.isEmpty() ) {
        err =
        QString(
        "Error in fields [%1].\n"
        "Valid device 1 NI-DAQ channel strings look like"
        " \"0,1,2,3 or 0:3,5,6.\"")
        .arg( uiStr2Err );
        return false;
    }

// no channels?

    nAI = vcMN2.size() + vcMA2.size() + vcXA2.size();
    nDI = vcXD2.size();

    if( !(nAI + nDI) ) {
        err = "Need at least 1 channel in device 2 NI-DAQ channel set.";
        return false;
    }

// illegal channels?

    maxAI = CniCfg::aiDevChanCount[q.ni.dev2] - 1;
    maxDI = CniCfg::diDevLineCount[q.ni.dev2] - 1;

    if( (vcMN2.count() && vcMN2.last() > maxAI)
        || (vcMA2.count() && vcMA2.last() > maxAI)
        || (vcXA2.count() && vcXA2.last() > maxAI) ) {

        err =
        QString("Device 2 AI channel values must not exceed [%1].")
        .arg( maxAI );
        return false;
    }

    if( vcXD2.count() && vcXD2.last() > maxDI ) {

        err =
        QString("Device 2 DI line values must not exceed [%1].")
        .arg( maxDI );
        return false;
    }

// ai ranges overlap?

    if( vcMN2.count() ) {

        if( (vcMA2.count() && vcMA2.first() <= vcMN2.last())
            || (vcXA2.count() && vcXA2.first() <= vcMN2.last()) ) {

            err = "Device 2 NI-DAQ channel ranges must not overlap.";
            return false;
        }
    }

    if( vcMA2.count() ) {

        if( vcXA2.count() && vcXA2.first() <= vcMA2.last() ) {

            err = "Device 2 NI-DAQ channel ranges must not overlap.";
            return false;
        }
    }

// sync line can not be digital input

    if( q.ni.syncEnable && vcXD2.count() ) {

        QString dev;
        int     line;
        CniCfg::parseDIStr( dev, line, q.ni.syncLine );

        if( dev == q.ni.dev2 && vcXD2.contains( line ) ) {

            err =
            "Sync output line cannot be used as a digital input line.";
            return false;
        }
    }

// too many ai channels?

    if( nAI > 1 && !CniCfg::supportsAISimultaneousSampling( q.ni.dev2 ) ) {

        err =
        QString(
        "Device [%1] does not support simultaneous sampling"
        " of multiple analog input channels.")
        .arg( q.ni.dev2 );
        return false;
    }

    if( q.ni.srate > CniCfg::maxSampleRate( q.ni.dev2, nAI ) ) {

        err =
        QString(
        "Sampling rate [%1] is too high for device 2 channel count (%d).")
        .arg( q.ni.srate )
        .arg( nAI );
        return false;
    }

    return true;
}


bool ConfigCtl::validTriggering( QString &err, DAQ::Params &q )
{
    if( q.mode.mTrig == DAQ::eTrigTTL || q.mode.mTrig == DAQ::eTrigSpike ) {

//-------------------
// BK: Temporary: Disallow spike, TTL until updated.
//err = "TTL and Spike triggers are disabled until imec integration is completed.";
//return false;
//-------------------

        int trgChan = q.trigChan(),
            nAna    = q.ni.niCumTypCnt[CniCfg::niSumAnalog];

        if( trgChan < 0 || trgChan >= nAna ) {

            err =
            QString(
            "Invalid '%1' trigger channel [%2]; must be in range [0..%3].")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( trgChan )
            .arg( nAna - 1 );
            return false;
        }

        qint16  T;

        if( q.mode.mTrig == DAQ::eTrigTTL )
            T = q.trgTTL.T;
        else
            T = q.trgSpike.T;

        if( T == -32768 || T == 32767 ) {

            err =
            QString(
            "%1 trigger threshold must be in range (%2..%3)/gain = (%4..%5) V.")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( q.ni.range.rmin )
            .arg( q.ni.range.rmax )
            .arg( q.ni.range.rmin/q.evtChanGain() )
            .arg( q.ni.range.rmax/q.evtChanGain() );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validNidqChanMap( QString &err, DAQ::Params &q )
{
    const int   *type = q.ni.niCumTypCnt;

    ChanMapNI &M = q.sns.niChans.chanMap;
    ChanMapNI D(
        0, 0,
        type[CniCfg::niTypeMN] / q.ni.muxFactor,
        (type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN]) / q.ni.muxFactor,
        q.ni.muxFactor,
        type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA],
        type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

    if( q.sns.niChans.chanMapFile.contains( "*" ) )
        q.sns.niChans.chanMapFile.clear();

    if( q.sns.niChans.chanMapFile.isEmpty() ) {

        M = D;
        M.fillDefault();
        return true;
    }

    QString msg;

    if( !M.loadFile( msg, q.sns.niChans.chanMapFile ) ) {

        err = QString("ChanMap: %1.").arg( msg );
        return false;
    }

    if( !M.equalHdr( D ) ) {

        err = QString(
                "ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3 %4 %5 %6 %7)\n"
                "  - Named file: (%8 %9 %10 %11 %12 %13 %14).")
                .arg( D.AP ).arg( D.LF ).arg( D.MN ).arg( D.MA ).arg( D.C ).arg( D.XA ).arg( D.XD )
                .arg( M.AP ).arg( M.LF ).arg( M.MN ).arg( M.MA ).arg( M.C ).arg( M.XA ).arg( M.XD );
        return false;
    }

    return true;
}


bool ConfigCtl::validNidqSaveBits( QString &err, DAQ::Params &q )
{
    return q.sns.niChans.deriveSaveBits(
            err, q.ni.niCumTypCnt[CniCfg::niSumAll] );
}


bool ConfigCtl::valid( QString &err, bool isGUI )
{
    err.clear();

    DAQ::Params     q;
    QVector<uint>   vcMN1, vcMA1, vcXA1, vcXD1,
                    vcMN2, vcMA2, vcXA2, vcXD2;
    QString         uiStr1Err,
                    uiStr2Err;

// ---------------------------
// Get user params from dialog
// ---------------------------

    paramsFromDialog( q,
        vcMN1, vcMA1, vcXA1, vcXD1,
        vcMN2, vcMA2, vcXA2, vcXD2,
        uiStr1Err, uiStr2Err );

// ------------
// Check params
// ------------

    if( !validNiDevices( err, q )
        || !validNiChannels( err, q,
                vcMN1, vcMA1, vcXA1, vcXD1,
                vcMN2, vcMA2, vcXA2, vcXD2,
                uiStr1Err, uiStr2Err ) ) {

        return false;
    }

    if( !validTriggering( err, q ) )
        return false;

    if( !validNidqChanMap( err, q ) )
        return false;

    if( !validNidqSaveBits( err, q ) )
        return false;

    if( !validRunName( err, q.sns.runName, cfgDlg, isGUI ) )
        return false;

// -------------
// Accept params
// -------------

    acceptedParams = q;

// ----
// Save
// ----

    acceptedParams.saveSettings();

    return true;
}


