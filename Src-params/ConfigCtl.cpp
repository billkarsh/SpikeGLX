
#include "ui_ConfigureDialog.h"
#include "ui_DevicesTab.h"
#include "ui_IMCfgTab.h"
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
#include "ui_IMForce.h"

#include "Pixmaps/Icon-Config.xpm"

#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "ConfigCtl.h"
#include "IMROEditor.h"
#include "ChanMapCtl.h"
#include "Subset.h"
#include "Version.h"

#include <QSharedMemory>
#include <QMessageBox>
#include <QDirIterator>


#define CURDEV1     niTabUI->device1CB->currentIndex()
#define CURDEV2     niTabUI->device2CB->currentIndex()


static const char *DEF_IMRO_LE = "*Default (bank 0, ref ext, gain 50)";
static const char *DEF_CHMP_LE = "*Default (Acquired order)";


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
        devTabUI(0),
        imTabUI(0),
        niTabUI(0),
        gateTabUI(0),
            gateImmPanelUI(0),
            gateTCPPanelUI(0),
        trigTabUI(0),
            trigImmPanelUI(0), trigTimPanelUI(0),
            trigTTLPanelUI(0), trigSpkPanelUI(0),
            trigTCPPanelUI(0),
        snsTabUI(0),
        cfgDlg(0),
        singleton(0),
        imecOK(false), nidqOK(false),
        validated(false)
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
    B->setText( "Run" );
    B->setAutoDefault( true );
    B->setDefault( true );

    B = cfgUI->buttonBox->button( QDialogButtonBox::Cancel );
    B->setAutoDefault( false );
    B->setDefault( false );

// ----------
// DevicesTab
// ----------

    devTabUI = new Ui::DevicesTab;
    devTabUI->setupUi( cfgUI->devTab );
    ConnectUI( devTabUI->skipBut, SIGNAL(clicked()), this, SLOT(skipDetect()) );
    ConnectUI( devTabUI->detectBut, SIGNAL(clicked()), this, SLOT(detect()) );
    devTabUI->detectBut->setDefault( true );

// --------
// IMCfgTab
// --------

    imTabUI = new Ui::IMCfgTab;
    imTabUI->setupUi( cfgUI->imTab );
    ConnectUI( imTabUI->forceBut, SIGNAL(clicked()), this, SLOT(forceButClicked()) );
    ConnectUI( imTabUI->imroBut, SIGNAL(clicked()), this, SLOT(imroButClicked()) );

// --------
// NICfgTab
// --------

    niTabUI = new Ui::NICfgTab;
    niTabUI->setupUi( cfgUI->niTab );
    ConnectUI( niTabUI->device1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(device1CBChanged()) );
    ConnectUI( niTabUI->device2CB, SIGNAL(currentIndexChanged(int)), this, SLOT(device2CBChanged()) );
    ConnectUI( niTabUI->mn1LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->ma1LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->mn2LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->ma2LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->dev2GB, SIGNAL(clicked()), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->clk1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(clk1CBChanged()) );
    ConnectUI( niTabUI->freqBut, SIGNAL(clicked()), this, SLOT(freqButClicked()) );
    ConnectUI( niTabUI->syncEnabChk, SIGNAL(clicked(bool)), this, SLOT(syncEnableClicked(bool)) );

// -------
// GateTab
// -------

    gateTabUI = new Ui::GateTab;
    gateTabUI->setupUi( cfgUI->gateTab );
    ConnectUI( gateTabUI->gateModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(gateModeChanged()) );
    ConnectUI( gateTabUI->manOvShowButChk, SIGNAL(clicked(bool)), this, SLOT(manOvShowButClicked(bool)) );

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
    ConnectUI( snsTabUI->imChnMapBut, SIGNAL(clicked()), this, SLOT(imChnMapButClicked()) );
    ConnectUI( snsTabUI->niChnMapBut, SIGNAL(clicked()), this, SLOT(niChnMapButClicked()) );
    ConnectUI( snsTabUI->runDirBut, SIGNAL(clicked()), this, SLOT(runDirButClicked()) );
    ConnectUI( snsTabUI->diskBut, SIGNAL(clicked()), this, SLOT(diskButClicked()) );
}


ConfigCtl::~ConfigCtl()
{
    singletonRelease();

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

    if( niTabUI ) {
        delete niTabUI;
        niTabUI = 0;
    }

    if( imTabUI ) {
        delete imTabUI;
        imTabUI = 0;
    }

    if( devTabUI ) {
        delete devTabUI;
        devTabUI = 0;
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

bool ConfigCtl::showDialog()
{
    acceptedParams.loadSettings();
    setupDevTab( acceptedParams );
    setNoDialogAccess();

    return QDialog::Accepted == cfgDlg->exec();
}


void ConfigCtl::setRunName( const QString &name )
{
    if( !validated )
        return;

    QString strippedName = name;
    QRegExp re("(.*)_[gG](\\d+)_[tT](\\d+)$");

    if( strippedName.contains( re ) )
        strippedName = re.cap(1);

    acceptedParams.sns.runName = strippedName;
    acceptedParams.saveSettings();

    if( cfgDlg->isVisible() )
        snsTabUI->runNameLE->setText( strippedName );
}


void ConfigCtl::graphSetsImroFile( const QString &file )
{
    QString err;

    acceptedParams.im.imroFile = file;

    if( validImROTbl( err, acceptedParams ) )
        acceptedParams.saveSettings();
    else
        Error() << err;
}


void ConfigCtl::graphSetsImSaveBit( int chan, bool setOn )
{
    DAQ::Params &p = acceptedParams;

    if( chan >= 0 && chan < p.im.imCumTypCnt[CimCfg::imSumAll] ) {

        p.sns.imChans.saveBits.setBit( chan, setOn );

        p.sns.imChans.uiSaveChanStr =
            Subset::bits2RngStr( p.sns.imChans.saveBits );

        Debug()
            << "New imec subset string: "
            << p.sns.imChans.uiSaveChanStr;

        p.saveSettings();
    }
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
// Used for remote GETSAVECHANSIM command.
//
QString ConfigCtl::cmdSrvGetsSaveChansIm() const
{
    QString         s;
    QTextStream     ts( &s, QIODevice::WriteOnly );
    const QBitArray &B = acceptedParams.sns.imChans.saveBits;
    int             nb = B.size();

    for( int i = 0; i < nb; ++i ) {

        if( B.testBit( i ) )
            ts << i << " ";
    }

    ts << "\n";

    return s;
}


// Space-separated list of current saved chans.
// Used for remote GETSAVECHANSNI command.
//
QString ConfigCtl::cmdSrvGetsSaveChansNi() const
{
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
QString ConfigCtl::cmdSrvGetsParamStr() const
{
    return DAQ::Params::settings2Str();
}


// Return QString::null or error string.
// Used for remote SETPARAMS command.
//
QString ConfigCtl::cmdSrvSetsParamStr( const QString &str )
{
    if( !validated )
        return "Run parameters never validated.";

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

    if( p.ni.dev1 != devNames[niTabUI->device1CB->currentIndex()] ) {

        return QString("Device [%1] does not support AI.")
                .arg( p.ni.dev1 );
    }

    if( p.ni.dev2 != devNames[niTabUI->device2CB->currentIndex()] ) {

        return QString("Device [%1] does not support AI.")
                .arg( p.ni.dev2 );
    }

    if( p.ni.clockStr1 != niTabUI->clk1CB->currentText() ) {

        return QString("Clock [%1] not supported on device [%2].")
                .arg( p.ni.clockStr1 )
                .arg( p.ni.dev1 );
    }

    if( p.ni.clockStr2 != niTabUI->clk2CB->currentText() ) {

        return QString("Clock [%1] not supported on device [%2].")
                .arg( p.ni.clockStr2 )
                .arg( p.ni.dev2 );
    }

    QString rng = QString("[%1, %2]")
                    .arg( p.ni.range.rmin )
                    .arg( p.ni.range.rmax );

    if( rng != niTabUI->aiRangeCB->currentText() ) {

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

void ConfigCtl::skipDetect()
{
    if( !somethingChecked() )
        return;

    if( devTabUI->imecGB->isChecked() && !imecOK ) {

        QMessageBox::information(
        cfgDlg,
        "Illegal Selection",
        "IMEC selected but did not pass last time." );
        return;
    }

    if( devTabUI->nidqGB->isChecked() && !nidqOK ) {

        QMessageBox::information(
        cfgDlg,
        "Illegal Selection",
        "NI-DAQ selected but did not pass last time." );
        return;
    }

    if( doingImec() ) {

        QTextEdit   *te = devTabUI->imTE;
        te->clear();
        imWrite( "Previous data:" );
        imWrite( "-----------------------------------" );
        imWrite( QString("Hardware version %1").arg( imVers.hwr ) );
        imWrite( QString("Basestation version %1").arg( imVers.bas ) );
        imWrite( QString("API version %1").arg( imVers.api ) );
        imWrite( QString("Probe serial# %1").arg( imVers.pSN ) );
        imWrite( QString("Probe option  %1").arg( imVers.opt ) );
        imWrite( "\nOK" );
    }

    setSelectiveAccess();
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
void ConfigCtl::detect()
{
    imecOK = false;
    nidqOK = false;

    if( !somethingChecked() )
        return;

    if( devTabUI->imecGB->isChecked() )
        imDetect();

    if( devTabUI->nidqGB->isChecked() )
        niDetect();

    devTabUI->skipBut->setEnabled( doingImec() || doingNidq() );

    setSelectiveAccess();
}


void ConfigCtl::forceButClicked()
{
    QDialog         D;
    Ui::IMForceDlg  *forceUI = new Ui::IMForceDlg;

    forceUI->setupUi( &D );
    forceUI->snLE->setText( imVers.pSN );
    forceUI->optCB->setCurrentIndex( imVers.opt - 1 );

    D.exec();

    imVers.pSN      = forceUI->snLE->text();
    imVers.opt      = forceUI->optCB->currentText().toInt();
    imVers.force    = true;

    delete forceUI;

    imTabUI->snLE->setText( imVers.pSN );
    imTabUI->optLE->setText( QString::number( imVers.opt ) );
}


void ConfigCtl::imroButClicked()
{
// -------------
// Launch editor
// -------------

    IMROEditor  ED( cfgDlg, imVers.pSN.toUInt(), imVers.opt );
    QString     imroFile;

    ED.Edit( imroFile, imTabUI->imroLE->text().trimmed(), -1 );

    if( imroFile.isEmpty() )
        imTabUI->imroLE->setText( DEF_IMRO_LE );
    else
        imTabUI->imroLE->setText( imroFile );
}


void ConfigCtl::device1CBChanged()
{
    if( !niTabUI->device1CB->count() )
        return;

    QString devStr = devNames[CURDEV1];

// --------
// AI range
// --------

    {
        QComboBox       *CB     = niTabUI->aiRangeCB;
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
        niTabUI->clk1CB->clear();
        niTabUI->clk1CB->addItem( "Internal" );

        QStringList pfiStrL = CniCfg::getPFIChans( devStr );
        int         npfi    = pfiStrL.count(),
                    pfiSel  = 0;

        // Note on QString::section() params:
        //
        // ("/dev/PFI").section('/',-1,-1)
        // -> ('/'=sep, -1=last field on right, -1=to end)
        // -> "PFI"

        for( int i = 0; i < npfi; ++i ) {

            QString s = pfiStrL[i].section( '/', -1, -1 );

            niTabUI->clk1CB->addItem( s );

            if( s == acceptedParams.ni.clockStr1 )
                pfiSel = i + 1;
        }

        niTabUI->clk1CB->setCurrentIndex( pfiSel );
    }

// ----------------------
// AI sample rate spinner
// ----------------------

    double  minRate =
                std::max( CniCfg::minSampleRate( devStr ), 100.0 ),
            maxRate =
                std::min( CniCfg::maxSampleRate( devStr ), 100000.0 );

    niTabUI->srateSB->setMinimum( minRate );
    niTabUI->srateSB->setMaximum( maxRate );

// ----
// Sync
// ----

    if( isMuxingFromDlg() ) {

        niTabUI->syncCB->setCurrentIndex(
            niTabUI->syncCB->findText(
                QString("%1/port0/line0").arg( devStr ) ) );
    }
}


void ConfigCtl::device2CBChanged()
{
// --------------------
// Set up Dev2 clock CB
// --------------------

    niTabUI->clk2CB->clear();

    if( !niTabUI->device2CB->count() ) {

noPFI:
        niTabUI->clk2CB->addItem( "PFI2" );
        niTabUI->clk2CB->setCurrentIndex( 0 );
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

        QString s = pfiStrL[i].section( '/', -1, -1 );

        niTabUI->clk2CB->addItem( s );

        if( s == acceptedParams.ni.clockStr2 )
            pfiSel = i;
    }

    niTabUI->clk2CB->setCurrentIndex( pfiSel );
}


void ConfigCtl::muxingChanged()
{
    bool    isMux = isMuxingFromDlg();

    if( isMux ) {

        int ci = niTabUI->clk1CB->findText( "PFI2", Qt::MatchExactly );
        niTabUI->clk1CB->setCurrentIndex( ci > -1 ? ci : 0 );

        niTabUI->syncEnabChk->setChecked( true );

        if( devNames.count() ) {
            ci = niTabUI->syncCB->findText(
                    QString("%1/port0/line0").arg( devNames[CURDEV1] ) );
        }
        else
            ci = -1;

        niTabUI->syncCB->setCurrentIndex( ci > -1 ? ci : 0 );
    }
    else {

        bool    wasMux = acceptedParams.ni.isMuxingMode();

        if( wasMux )
            snsTabUI->niSaveChansLE->setText( "all" );
    }

    niTabUI->clk1CB->setDisabled( isMux );
    niTabUI->syncEnabChk->setDisabled( isMux );
    niTabUI->syncCB->setDisabled( isMux );
}


void ConfigCtl::clk1CBChanged()
{
    niTabUI->freqBut->setEnabled( niTabUI->clk1CB->currentIndex() != 0 );
}


void ConfigCtl::freqButClicked()
{
    if( !devNames.count() )
        return;

    QString txt = niTabUI->freqBut->text();

    niTabUI->freqBut->setText( "Sampling; hold on..." );
    niTabUI->freqBut->repaint();

    double  f = CniCfg::sampleFreq(
                    devNames[CURDEV1],
                    niTabUI->clk1CB->currentText(),
                    niTabUI->syncCB->currentText() );

    niTabUI->freqBut->setText( txt );

    if( !f ) {

        QMessageBox::critical(
            cfgDlg,
            "Frequency Measurement Failed",
            "The measured sample rate is zero...check power supply and cables." );
        return;
    }

    if( isMuxingFromDlg() )
        f /= niTabUI->muxFactorSB->value();

    double  vMin = niTabUI->srateSB->minimum(),
            vMax = niTabUI->srateSB->maximum();

    if( f < vMin || f > vMax ) {

        QMessageBox::warning(
            cfgDlg,
            "Value Outside Range",
            QString("The measured sample rate is [%1].\n\n"
            "The current system is limited to range [%2..%3],\n"
            "so you must use a different clock source or rate.")
            .arg( f ).arg( vMin ).arg( vMax ) );
    }

    niTabUI->srateSB->setValue( f );
}


void ConfigCtl::syncEnableClicked( bool checked )
{
    niTabUI->syncCB->setEnabled( checked && !isMuxingFromDlg() );
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


void ConfigCtl::manOvShowButClicked( bool checked )
{
    gateTabUI->manOvInitOffChk->setEnabled( checked );

    if( !checked )
        gateTabUI->manOvInitOffChk->setChecked( false );
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


void ConfigCtl::imChnMapButClicked()
{
// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    CimCfg  im;

    im.deriveChanCounts( imVers.opt );

    const int   *type = im.imCumTypCnt;

    ChanMapIM defMap(
        type[CimCfg::imTypeAP],
        type[CimCfg::imTypeLF] - type[CimCfg::imTypeAP],
        type[CimCfg::imTypeSY] - type[CimCfg::imTypeLF] );

// -------------
// Launch editor
// -------------

    ChanMapCtl  CM( cfgDlg, defMap );

    QString mapFile = CM.Edit( snsTabUI->imChnMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        snsTabUI->imChnMapLE->setText( DEF_CHMP_LE );
    else
        snsTabUI->imChnMapLE->setText( mapFile );
}


void ConfigCtl::niChnMapButClicked()
{
// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    QVector<uint>   vcMN1, vcMA1, vcXA1, vcXD1,
                    vcMN2, vcMA2, vcXA2, vcXD2;
    CniCfg          ni;

    if( !Subset::rngStr2Vec( vcMN1, niTabUI->mn1LE->text() )
        || !Subset::rngStr2Vec( vcMA1, niTabUI->ma1LE->text() )
        || !Subset::rngStr2Vec( vcXA1, niTabUI->xa1LE->text() )
        || !Subset::rngStr2Vec( vcXD1, niTabUI->xd1LE->text() )
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
    ni.muxFactor        = niTabUI->muxFactorSB->value();
    ni.isDualDevMode    = niTabUI->dev2GB->isChecked();

    ni.deriveChanCounts();

    const int   *type = ni.niCumTypCnt;

    ChanMapNI defMap(
        type[CniCfg::niTypeMN] / ni.muxFactor,
        (type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN]) / ni.muxFactor,
        ni.muxFactor,
        type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA],
        type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

// -------------
// Launch editor
// -------------

    ChanMapCtl  CM( cfgDlg, defMap );

    QString mapFile = CM.Edit( snsTabUI->niChnMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        snsTabUI->niChnMapLE->setText( DEF_CHMP_LE );
    else
        snsTabUI->niChnMapLE->setText( mapFile );
}


void ConfigCtl::runDirButClicked()
{
    MainApp *app = mainApp();

    app->options_PickRunDir();
    snsTabUI->runDirLbl->setText( app->runDir() );
}


void ConfigCtl::diskButClicked()
{
    snsTabUI->diskTE->clear();

    DAQ::Params q;
    QString     err;

    if( !diskParamsToQ( err, q ) ) {
        diskWrite( "Parameter error" );
        QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
        return;
    }

    double  BPS = 0;

    if( doingImec() ) {

        int     ch  = q.sns.imChans.saveBits.count( true );
        double  bps = ch * q.im.srate * 2;

        BPS += bps;

        QString s =
            QString("IM: %1 chn @ %2 Hz = %3 MB/s")
            .arg( ch )
            .arg( (int)q.im.srate )
            .arg( bps / (1024*1024), 0, 'f', 2 );

        diskWrite( s );
    }

    if( doingNidq() ) {

        int     ch  = q.sns.niChans.saveBits.count( true );
        double  bps = ch * q.ni.srate * 2;

        BPS += bps;

        QString s =
            QString("NI: %1 chn @ %2 Hz = %3 MB/s")
            .arg( ch )
            .arg( (int)q.ni.srate )
            .arg( bps / (1024*1024), 0, 'f', 2 );

        diskWrite( s );
    }

    quint64 avail = availableDiskSpace();

    QString s =
        QString("Avail: %1 GB / %2 MB/s = %3 min")
        .arg( avail / (1024UL*1024*1024) )
        .arg( BPS / (1024*1024), 0, 'f', 2 )
        .arg( avail / BPS / 60, 0, 'f', 1 );

    diskWrite( s );
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


void ConfigCtl::reset( DAQ::Params *pRemote )
{
    DAQ::Params &p = (pRemote ? *pRemote : acceptedParams);

    p.loadSettings( pRemote != 0 );

    setupDevTab( p );
    setupImTab( p );
    setupNiTab( p );
    setupGateTab( p );
    setupTrigTab( p );
    setupSnsTab( p );
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

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if this app instance owns NI hardware.
//
bool ConfigCtl::singletonReserve()
{
#ifdef HAVE_NIDAQmx
    if( !singleton ) {

        // Create 8 byte memory space because it's a pretty
        // alignment value We don't use the space, though.

        singleton = new QSharedMemory( "SpikeGLX_App" );

        if( !singleton->create( 8 ) ) {
            singletonRelease();
            return false;
        }
    }
#endif

    return true;
}


void ConfigCtl::singletonRelease()
{
    if( singleton ) {
        delete singleton;
        singleton = 0;
    }
}


void ConfigCtl::setNoDialogAccess()
{
    devTabUI->imTE->clear();
    devTabUI->niTE->clear();

// Can't tab

    for( int i = 1, n = cfgUI->tabsW->count(); i < n; ++i )
        cfgUI->tabsW->setTabEnabled( i, false );

// Can't verify or ok

    cfgUI->resetBut->setDisabled( true );
    cfgUI->verifyBut->setDisabled( true );
    cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setDisabled( true );

    guiBreathe();
}


void ConfigCtl::setSelectiveAccess()
{
// Main buttons

    if( imecOK || nidqOK ) {
        cfgUI->resetBut->setEnabled( true );
        cfgUI->verifyBut->setEnabled( true );
        cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
        cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setDefault( true );
        cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setFocus();
    }

// Tabs

    DAQ::Params &p = acceptedParams;

    if( imecOK ) {
        setupImTab( p );
        cfgUI->tabsW->setTabEnabled( 1, true );
    }

    if( nidqOK ) {
        setupNiTab( p );
        cfgUI->tabsW->setTabEnabled( 2, true );
    }

    if( imecOK || nidqOK ) {
        setupGateTab( p );
        setupTrigTab( p );
        setupSnsTab( p );
        cfgUI->tabsW->setTabEnabled( 3, true );
        cfgUI->tabsW->setTabEnabled( 4, true );
        cfgUI->tabsW->setTabEnabled( 5, true );
    }
}


bool ConfigCtl::somethingChecked()
{
    setNoDialogAccess();

    if( !devTabUI->nidqGB->isChecked() )
        singletonRelease();

    if( !devTabUI->imecGB->isChecked()
        && !devTabUI->nidqGB->isChecked() ) {

        QMessageBox::information(
        cfgDlg,
        "No Hardware Selected",
        "'Enable' the hardware devices you want use...\n\n"
        "Then click 'Detect' to see what's installed." );
        return false;
    }

    return true;
}


void ConfigCtl::imWrite( const QString &s )
{
    QTextEdit   *te = devTabUI->imTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


void ConfigCtl::imDetect()
{
    QTextEdit   *te = devTabUI->imTE;
    QStringList sl;
    bool        ok;

    imWrite( "Connecting...allow several seconds." );
    guiBreathe();

    ok = CimCfg::getVersions( sl, imVers );

    te->clear();
    foreach( const QString &s, sl )
        imWrite( s );

    if( ok ) {

        if( imVers.opt < 1 || imVers.opt > 4 ) {
            imWrite(
                QString("\n** Illegal probe option (%1), must be [1..4].")
                .arg( imVers.opt ) );
        }
        else
            imecOK = true;
    }

    if( imecOK )
        imWrite( "\nOK" );
    else
        imWrite( "\nFAIL - Cannot be used" );

    te->moveCursor( QTextCursor::Start );
}


void ConfigCtl::niWrite( const QString &s )
{
    QTextEdit   *te = devTabUI->niTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


QColor ConfigCtl::niSetColor( const QColor &c )
{
    QTextEdit   *te     = devTabUI->niTE;
    QColor      cPrev   = te->textColor();

    te->setTextColor( c );

    return cPrev;
}


void ConfigCtl::niDetect()
{
    niWrite( "Multifunction Input Devices:" );
    niWrite( "-----------------------------------" );

    if( !singletonReserve() ) {
        niWrite(
            "Another instance of " APPNAME " already owns"
            " the NI hardware." );
        return;
    }

    if( !CniCfg::isHardware() ) {
        niWrite( "None" );
        return;
    }

    CniCfg::probeAIHardware();
    CniCfg::probeAllDILines();

// First list devs having both [AI, DI]

    for( int idev = 0; idev <= 16; ++idev ) {

        QString D = QString( "Dev%1" ).arg( idev );

        if( CniCfg::aiDevChanCount.contains( D ) ) {

            if( CniCfg::diDevLineCount.contains( D ) ) {

                QList<VRange>   rngL = CniCfg::aiDevRanges.values( D );

                if( rngL.size() ) {
                    niWrite(
                        QString("%1 (%2)")
                        .arg( D )
                        .arg( CniCfg::getProductName( D ) ) );
                    nidqOK = true;
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
        }
    }

    if( !nidqOK )
        niWrite( "None" );

// Now [AO]

    niWrite( "\nAnalog Output Devices:" );
    niWrite( "-----------------------------------" );

    CniCfg::probeAOHardware();

    QStringList devs    = CniCfg::aoDevChanCount.uniqueKeys();

    foreach( const QString &D, devs ) {

        QList<VRange>   rngL = CniCfg::aoDevRanges.values( D );

        if( rngL.size() ) {
            niWrite(
                QString("%1 (%2)")
                .arg( D )
                .arg( CniCfg::getProductName( D ) ) );
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

    if( !devs.count() )
        niWrite( "None" );

    niWrite( "-- end --" );

    if( nidqOK )
        niWrite( "\nOK" );
    else
        niWrite( "\nFAIL - Cannot be used" );

    devTabUI->niTE->moveCursor( QTextCursor::Start );
}


bool ConfigCtl::doingImec() const
{
    return imecOK && devTabUI->imecGB->isChecked();
}


bool ConfigCtl::doingNidq() const
{
    return nidqOK && devTabUI->nidqGB->isChecked();
}


void ConfigCtl::diskWrite( const QString &s )
{
    QTextEdit   *te = snsTabUI->diskTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


void ConfigCtl::setupDevTab( DAQ::Params &p )
{
    devTabUI->imecGB->setChecked( p.im.enabled );
    devTabUI->nidqGB->setChecked( p.ni.enabled );

    devTabUI->skipBut->setEnabled( doingImec() || doingNidq() );

// --------------------
// Observe dependencies
// --------------------
}


void ConfigCtl::setupImTab( DAQ::Params &p )
{
    imTabUI->snLE->setText( imVers.pSN );
    imTabUI->optLE->setText( QString::number( imVers.opt ) );

    imTabUI->hpCB->setCurrentIndex( p.im.hpFltIdx == 3 ? 2 : p.im.hpFltIdx );
//    imTabUI->trigCB->setCurrentIndex( p.im.softStart );

// BK: =============================================
// BK: Until triggering modes supported by imec...
    imTabUI->trigCB->setCurrentIndex( 1 );
    imTabUI->trigCB->setDisabled( true );
// BK: =============================================

    imTabUI->gainCorChk->setChecked( p.im.doGainCor );

    if( p.im.imroFile.contains( "*" ) )
        p.im.imroFile.clear();

    if( p.im.imroFile.isEmpty() )
        imTabUI->imroLE->setText( DEF_IMRO_LE );
    else
        imTabUI->imroLE->setText( p.im.imroFile );

// --------------------
// Observe dependencies
// --------------------
}


void ConfigCtl::setupNiTab( DAQ::Params &p )
{
    niTabUI->srateSB->setValue( p.ni.srate );
    niTabUI->mnGainSB->setValue( p.ni.mnGain );
    niTabUI->maGainSB->setValue( p.ni.maGain );

// Devices

    QComboBox   *CB1, *CB2;

    CB1 = niTabUI->device1CB;
    CB2 = niTabUI->device2CB;

    devNames.clear();
    CB1->clear();
    CB2->clear();

    {
        QStringList devs    = CniCfg::aiDevChanCount.uniqueKeys();
        int         sel     = 0,
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

    niTabUI->mn1LE->setText( p.ni.uiMNStr1 );
    niTabUI->ma1LE->setText( p.ni.uiMAStr1 );
    niTabUI->xa1LE->setText( p.ni.uiXAStr1 );
    niTabUI->xd1LE->setText( p.ni.uiXDStr1 );
    niTabUI->mn2LE->setText( p.ni.uiMNStr2Bare() );
    niTabUI->ma2LE->setText( p.ni.uiMAStr2Bare() );
    niTabUI->xa2LE->setText( p.ni.uiXAStr2Bare() );
    niTabUI->xd2LE->setText( p.ni.uiXDStr2Bare() );

// Termination choices loaded in form data

    {
        int ci = niTabUI->aiTerminationCB->findText(
                    CniCfg::termConfigToString( p.ni.termCfg ),
                    Qt::MatchExactly );

        niTabUI->aiTerminationCB->setCurrentIndex( ci > -1 ? ci : 0 );
    }

    niTabUI->muxFactorSB->setValue( p.ni.muxFactor );

    if( CB2->count() > 1 ) {
        niTabUI->dev2GB->setChecked( p.ni.isDualDevMode );
        niTabUI->dev2GB->setEnabled( true );
    }
    else {
        niTabUI->dev2GB->setChecked( false );
        niTabUI->dev2GB->setEnabled( false );
        p.ni.isDualDevMode = false;
    }

// Sync

    niTabUI->syncEnabChk->setChecked( p.ni.syncEnable );
    niTabUI->syncCB->clear();

    {
        QStringList L  = CniCfg::getAllDOLines();
        int         sel;

        foreach( QString s, L )
            niTabUI->syncCB->addItem( s );

        sel = niTabUI->syncCB->findText( p.ni.syncLine );

        if( sel < 0 ) {
            sel = niTabUI->syncCB->findText(
                    QString("%1/port0/line0").arg( devNames[CURDEV1] ) );
        }

        niTabUI->syncCB->setCurrentIndex( sel );
    }

// --------------------
// Observe dependencies
// --------------------

    device1CBChanged(); // <-- Call This First!! - Fills in other CBs
    device2CBChanged();
    muxingChanged();
    clk1CBChanged();
    syncEnableClicked( p.ni.syncEnable );
}


void ConfigCtl::setupGateTab( DAQ::Params &p )
{
    gateTabUI->gateModeCB->setCurrentIndex( (int)p.mode.mGate );
    gateTabUI->manOvShowButChk->setChecked( p.mode.manOvShowBut );
    gateTabUI->manOvInitOffChk->setChecked( p.mode.manOvInitOff );

// --------------------
// Observe dependencies
// --------------------

    gateModeChanged();
    manOvShowButClicked( p.mode.manOvShowBut );
}


void ConfigCtl::setupTrigTab( DAQ::Params &p )
{
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

    trigTTLPanelUI->TSB->setValue( p.trgTTL.T );
    trigTTLPanelUI->marginSB->setValue( p.trgTTL.marginSecs );
    trigTTLPanelUI->refracSB->setValue( p.trgTTL.refractSecs );
    trigTTLPanelUI->HSB->setValue( p.trgTTL.tH );
    trigTTLPanelUI->streamCB->setCurrentIndex( p.trgTTL.stream == "nidq" );
    trigTTLPanelUI->modeCB->setCurrentIndex( p.trgTTL.mode );
    trigTTLPanelUI->chanSB->setValue( p.trgTTL.aiChan );
    trigTTLPanelUI->inarowSB->setValue( p.trgTTL.inarow );
    trigTTLPanelUI->NSB->setValue( p.trgTTL.nH );
    trigTTLPanelUI->NInfChk->setChecked( p.trgTTL.isNInf );

// --------------
// TrgSpikeParams
// --------------

    trigSpkPanelUI->TSB->setValue( 1e6 * p.trgSpike.T );
    trigSpkPanelUI->periSB->setValue( p.trgSpike.periEvtSecs );
    trigSpkPanelUI->refracSB->setValue( p.trgSpike.refractSecs );
    trigSpkPanelUI->streamCB->setCurrentIndex( p.trgSpike.stream == "nidq" );
    trigSpkPanelUI->chanSB->setValue( p.trgSpike.aiChan );
    trigSpkPanelUI->inarowSB->setValue( p.trgSpike.inarow );
    trigSpkPanelUI->NSB->setValue( p.trgSpike.nS );
    trigSpkPanelUI->NInfChk->setChecked( p.trgSpike.isNInf );

// -------
// TrigTab
// -------

    trigTabUI->trigModeCB->setCurrentIndex( (int)p.mode.mTrig );

// --------------------
// Observe dependencies
// --------------------

    trigModeChanged();
    trigTimHInfClicked();
    trigTimNInfClicked( p.trgTim.isNInf );
    trigTTLModeChanged( p.trgTTL.mode );
    trigTTLNInfClicked( p.trgTTL.isNInf );
    trigSpkNInfClicked( p.trgSpike.isNInf );
}


void ConfigCtl::setupSnsTab( DAQ::Params &p )
{
// Imec

    if( p.sns.imChans.chanMapFile.contains( "*" ) )
        p.sns.imChans.chanMapFile.clear();

    if( p.sns.imChans.chanMapFile.isEmpty() )
        snsTabUI->imChnMapLE->setText( DEF_CHMP_LE );
    else
        snsTabUI->imChnMapLE->setText( p.sns.imChans.chanMapFile );

    snsTabUI->imChnMapBut->setEnabled( imecOK );

// Nidq

    if( p.sns.niChans.chanMapFile.contains( "*" ) )
        p.sns.niChans.chanMapFile.clear();

    if( p.sns.niChans.chanMapFile.isEmpty() )
        snsTabUI->niChnMapLE->setText( DEF_CHMP_LE );
    else
        snsTabUI->niChnMapLE->setText( p.sns.niChans.chanMapFile );

    snsTabUI->niChnMapBut->setEnabled( nidqOK );

    snsTabUI->imSaveChansLE->setText( p.sns.imChans.uiSaveChanStr );
    snsTabUI->niSaveChansLE->setText( p.sns.niChans.uiSaveChanStr );

    snsTabUI->runDirLbl->setText( mainApp()->runDir() );
    snsTabUI->runNameLE->setText( p.sns.runName );

    snsTabUI->diskSB->setValue( p.sns.reqMins );

// --------------------
// Observe dependencies
// --------------------
}


QString ConfigCtl::uiMNStr2FromDlg()
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->mn2LE->text() : "");
}


QString ConfigCtl::uiMAStr2FromDlg()
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->ma2LE->text() : "");
}


QString ConfigCtl::uiXAStr2FromDlg()
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->xa2LE->text() : "");
}


QString ConfigCtl::uiXDStr2FromDlg()
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->xd2LE->text() : "");
}


bool ConfigCtl::isMuxingFromDlg()
{
    return  !niTabUI->mn1LE->text().isEmpty()
            || !niTabUI->ma1LE->text().isEmpty()
            || (niTabUI->dev2GB->isChecked()
                && (!niTabUI->mn2LE->text().isEmpty()
                    || !niTabUI->ma2LE->text().isEmpty())
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

    if( doingImec() ) {

        q.im.hpFltIdx   = imTabUI->hpCB->currentIndex();
        q.im.softStart  = imTabUI->trigCB->currentIndex();
        q.im.doGainCor  = imTabUI->gainCorChk->isChecked();

        if( q.im.hpFltIdx == 2 )
            q.im.hpFltIdx = 3;

        q.im.imroFile   = imTabUI->imroLE->text().trimmed();
        q.im.enabled    = true;
    }
    else {
        q.im            = acceptedParams.im;
        q.im.enabled    = false;
    }

    q.im.deriveChanCounts( imVers.opt );

// ----
// NIDQ
// ----

    if( doingNidq() ) {

        if( !Subset::rngStr2Vec( vcMN1, niTabUI->mn1LE->text() ) )
            uiStr1Err = "MN-chans";

        if( !Subset::rngStr2Vec( vcMA1, niTabUI->ma1LE->text() ) ) {
            uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
            uiStr1Err += "MA-chans";
        }

        if( !Subset::rngStr2Vec( vcXA1, niTabUI->xa1LE->text() ) ) {
            uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
            uiStr1Err += "XA-chans";
        }

        if( !Subset::rngStr2Vec( vcXD1, niTabUI->xd1LE->text() ) ) {
            uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
            uiStr1Err += "XD-chans";
        }

        if( !Subset::rngStr2Vec( vcMN2, niTabUI->mn2LE->text() ) )
            uiStr2Err = "MN-chans";

        if( !Subset::rngStr2Vec( vcMA2, niTabUI->ma2LE->text() ) ) {
            uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
            uiStr2Err += "MA-chans";
        }

        if( !Subset::rngStr2Vec( vcXA2, niTabUI->xa2LE->text() ) ) {
            uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
            uiStr2Err += "XA-chans";
        }

        if( !Subset::rngStr2Vec( vcXD2, niTabUI->xd2LE->text() ) ) {
            uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
            uiStr2Err += "XD-chans";
        }

        q.ni.dev1 =
        (niTabUI->device1CB->count() ? devNames[CURDEV1] : "");

        q.ni.dev2 =
        (niTabUI->device2CB->count() ? devNames[CURDEV2] : "");

        if( niTabUI->device1CB->count() ) {

            QList<VRange>   rngL = CniCfg::aiDevRanges.values( q.ni.dev1 );

            if( rngL.size() && niTabUI->aiRangeCB->currentIndex() >= 0 )
                q.ni.range = rngL[niTabUI->aiRangeCB->currentIndex()];
        }

        q.ni.clockStr1     = niTabUI->clk1CB->currentText();
        q.ni.clockStr2     = niTabUI->clk2CB->currentText();
        q.ni.srate         = niTabUI->srateSB->value();
        q.ni.mnGain        = niTabUI->mnGainSB->value();
        q.ni.maGain        = niTabUI->maGainSB->value();
        q.ni.uiMNStr1      = Subset::vec2RngStr( vcMN1 );
        q.ni.uiMAStr1      = Subset::vec2RngStr( vcMA1 );
        q.ni.uiXAStr1      = Subset::vec2RngStr( vcXA1 );
        q.ni.uiXDStr1      = Subset::vec2RngStr( vcXD1 );
        q.ni.setUIMNStr2( Subset::vec2RngStr( vcMN2 ) );
        q.ni.setUIMAStr2( Subset::vec2RngStr( vcMA2 ) );
        q.ni.setUIXAStr2( Subset::vec2RngStr( vcXA2 ) );
        q.ni.setUIXDStr2( Subset::vec2RngStr( vcXD2 ) );
        q.ni.syncLine      = niTabUI->syncCB->currentText();
        q.ni.muxFactor     = niTabUI->muxFactorSB->value();

        q.ni.termCfg =
        q.ni.stringToTermConfig( niTabUI->aiTerminationCB->currentText() );

        q.ni.isDualDevMode  = niTabUI->dev2GB->isChecked();
        q.ni.syncEnable     = niTabUI->syncEnabChk->isChecked();
        q.ni.enabled        = true;
    }
    else {
        q.ni                = acceptedParams.ni;
        q.ni.enabled        = false;
    }

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

    q.trgTTL.T              = trigTTLPanelUI->TSB->value();
    q.trgTTL.marginSecs     = trigTTLPanelUI->marginSB->value();
    q.trgTTL.refractSecs    = trigTTLPanelUI->refracSB->value();
    q.trgTTL.tH             = trigTTLPanelUI->HSB->value();
    q.trgTTL.stream         = trigTTLPanelUI->streamCB->currentText();
    q.trgTTL.mode           = trigTTLPanelUI->modeCB->currentIndex();
    q.trgTTL.aiChan         = trigTTLPanelUI->chanSB->value();
    q.trgTTL.inarow         = trigTTLPanelUI->inarowSB->value();
    q.trgTTL.nH             = trigTTLPanelUI->NSB->value();
    q.trgTTL.isNInf         = trigTTLPanelUI->NInfChk->isChecked();

// --------------
// TrgSpikeParams
// --------------

    q.trgSpike.T            = trigSpkPanelUI->TSB->value() / 1e6;
    q.trgSpike.periEvtSecs  = trigSpkPanelUI->periSB->value();
    q.trgSpike.refractSecs  = trigSpkPanelUI->refracSB->value();
    q.trgSpike.stream       = trigSpkPanelUI->streamCB->currentText();
    q.trgSpike.aiChan       = trigSpkPanelUI->chanSB->value();
    q.trgSpike.inarow       = trigSpkPanelUI->inarowSB->value();
    q.trgSpike.nS           = trigSpkPanelUI->NSB->value();
    q.trgSpike.isNInf       = trigSpkPanelUI->NInfChk->isChecked();

// ----------
// ModeParams
// ----------

    q.mode.mGate            = (DAQ::GateMode)gateTabUI->gateModeCB->currentIndex();
    q.mode.mTrig            = (DAQ::TrigMode)trigTabUI->trigModeCB->currentIndex();
    q.mode.manOvShowBut     = gateTabUI->manOvShowButChk->isChecked();
    q.mode.manOvInitOff     = gateTabUI->manOvInitOffChk->isChecked();

// --------
// SeeNSave
// --------

    q.sns.imChans.chanMapFile   = snsTabUI->imChnMapLE->text().trimmed();
    q.sns.niChans.chanMapFile   = snsTabUI->niChnMapLE->text().trimmed();
    q.sns.imChans.uiSaveChanStr = snsTabUI->imSaveChansLE->text();
    q.sns.niChans.uiSaveChanStr = snsTabUI->niSaveChansLE->text();
    q.sns.runName               = snsTabUI->runNameLE->text().trimmed();
    q.sns.reqMins               = snsTabUI->diskSB->value();
}


bool ConfigCtl::validDevTab( QString &err, DAQ::Params &q )
{
    if( !q.im.enabled && !q.ni.enabled ) {

        err =
        "Enable/Detect at least one device group on the Devices tab.";
        return false;
    }

    return true;
}


bool ConfigCtl::validImROTbl( QString &err, DAQ::Params &q )
{
// Pretties ini file, even if not using device
    if( q.im.imroFile.contains( "*" ) )
        q.im.imroFile.clear();

    if( !doingImec() )
        return true;

    if( q.im.imroFile.isEmpty() ) {

        q.im.roTbl.fillDefault( imVers.pSN.toUInt(), imVers.opt );
        return true;
    }

    QString msg;

    if( !q.im.roTbl.loadFile( msg, q.im.imroFile ) ) {

        err = QString("ImroFile: %1.").arg( msg );
        return false;
    }

    if( (int)q.im.roTbl.opt != imVers.opt ) {

        err = QString( "Option %1 named in imro file." )
                .arg( q.im.roTbl.opt );
        return false;
    }

    return true;
}


bool ConfigCtl::validNiDevices( QString &err, DAQ::Params &q )
{
    if( !doingNidq() )
        return true;

// ----
// Dev1
// ----

    if( !CniCfg::aiDevRanges.size()
        || !q.ni.dev1.length() ) {

        err =
        "No NIDAQ analog input devices installed.\n\n"
        "Resolve issues in NI 'Measurement & Automation Explorer'.";
        return false;
    }

    {
        QList<VRange>   rngL = CniCfg::aiDevRanges.values( q.ni.dev1 );

        if( !rngL.size() ) {
            err =
            QString("Device '%1' is disconnected or off.")
            .arg( q.ni.dev1 );
            return false;
        }
    }

// ----
// Dev2
// ----

    if( !q.ni.isDualDevMode )
        return true;

    if( !q.ni.dev2.length() ) {

        err =
        "No secondary NIDAQ analog input devices installed.\n\n"
        "Resolve issues in NI 'Measurement & Automation Explorer'.";
        return false;
    }

    {
        QList<VRange>   rngL = CniCfg::aiDevRanges.values( q.ni.dev2 );

        if( !rngL.size() ) {
            err =
            QString("Device '%1' is disconnected or off.")
            .arg( q.ni.dev2 );
            return false;
        }
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
    if( !doingNidq() )
        return true;

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


bool ConfigCtl::validImTriggering( QString &err, DAQ::Params &q )
{
    if( !doingImec() ) {

        err =
        QString(
        "Imec stream selected for '%1' trigger but not enabled.")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) );
        return false;
    }

    int trgChan = q.trigChan(),
        nLegal  = q.im.imCumTypCnt[CimCfg::imSumNeural];

    if( trgChan < 0 || trgChan >= nLegal ) {

        err =
        QString(
        "Invalid '%1' trigger channel [%2]; must be in range [0..%3].")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) )
        .arg( trgChan )
        .arg( nLegal - 1 );
        return false;
    }

    double  Tmin = q.im.int10ToV( -512, trgChan ),
            Tmax = q.im.int10ToV(  511, trgChan );

    if( q.mode.mTrig == DAQ::eTrigTTL ) {

        if( q.trgTTL.T < Tmin || q.trgTTL.T > Tmax ) {

            err =
            QString(
            "%1 trigger threshold must be in range (%2..%3) V.")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {
        if( q.trgSpike.T < Tmin || q.trgSpike.T > Tmax ) {

            err =
            QString(
            "%1 trigger threshold must be in range (%2..%3) uV.")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( 1e6*Tmin ).arg( 1e6*Tmax );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validNiTriggering( QString &err, DAQ::Params &q )
{
    if( !doingNidq() ) {

        err =
        QString(
        "Nidq stream selected for '%1' trigger but not enabled.")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) );
        return false;
    }

    int trgChan = q.trigChan(),
        nLegal  = q.ni.niCumTypCnt[CniCfg::niSumAnalog];

    if( trgChan < 0 || trgChan >= nLegal ) {

        err =
        QString(
        "Invalid '%1' trigger channel [%2]; must be in range [0..%3].")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) )
        .arg( trgChan )
        .arg( nLegal - 1 );
        return false;
    }

    double  Tmin = q.ni.int16ToV( -32768, trgChan ),
            Tmax = q.ni.int16ToV(  32767, trgChan );

    if( q.mode.mTrig == DAQ::eTrigTTL ) {

        if( q.trgTTL.T < Tmin || q.trgTTL.T > Tmax ) {

            err =
            QString(
            "%1 trigger threshold must be in range (%2..%3) V.")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {
        if( q.trgSpike.T < Tmin || q.trgSpike.T > Tmax ) {

            err =
            QString(
            "%1 trigger threshold must be in range (%2..%3) uV.")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( 1e6*Tmin ).arg( 1e6*Tmax );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validImChanMap( QString &err, DAQ::Params &q )
{
// Pretties ini file, even if not using device
    if( q.sns.imChans.chanMapFile.contains( "*" ) )
        q.sns.imChans.chanMapFile.clear();

    if( !doingImec() )
        return true;

    const int   *type = q.im.imCumTypCnt;

    ChanMapIM &M = q.sns.imChans.chanMap;
    ChanMapIM D(
        type[CimCfg::imTypeAP],
        type[CimCfg::imTypeLF] - type[CimCfg::imTypeAP],
        type[CimCfg::imTypeSY] - type[CimCfg::imTypeLF] );

    if( q.sns.imChans.chanMapFile.isEmpty() ) {

        M = D;
        M.fillDefault();
        return true;
    }

    QString msg;

    if( !M.loadFile( msg, q.sns.imChans.chanMapFile ) ) {

        err = QString("ChanMap: %1.").arg( msg );
        return false;
    }

    if( !M.equalHdr( D ) ) {

        err = QString(
                "ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3)\n"
                "  - Named file: (%4 %5 %6).")
                .arg( D.AP ).arg( D.LF ).arg( D.SY )
                .arg( M.AP ).arg( M.LF ).arg( M.SY );
        return false;
    }

    return true;
}


bool ConfigCtl::validNiChanMap( QString &err, DAQ::Params &q )
{
// Pretties ini file, even if not using device
    if( q.sns.niChans.chanMapFile.contains( "*" ) )
        q.sns.niChans.chanMapFile.clear();

    if( !doingNidq() )
        return true;

    const int   *type = q.ni.niCumTypCnt;

    ChanMapNI &M = q.sns.niChans.chanMap;
    ChanMapNI D(
        type[CniCfg::niTypeMN] / q.ni.muxFactor,
        (type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN]) / q.ni.muxFactor,
        q.ni.muxFactor,
        type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA],
        type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

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
                "  - Cur config: (%1 %2 %3 %4 %5)\n"
                "  - Named file: (%6 %7 %8 %9 %10).")
                .arg( D.MN ).arg( D.MA ).arg( D.C ).arg( D.XA ).arg( D.XD )
                .arg( M.MN ).arg( M.MA ).arg( M.C ).arg( M.XA ).arg( M.XD );
        return false;
    }

    return true;
}


bool ConfigCtl::validImSaveBits( QString &err, DAQ::Params &q )
{
    if( !doingImec() )
        return true;

    return q.sns.imChans.deriveSaveBits(
            err, q.im.imCumTypCnt[CimCfg::imSumAll] );
}


bool ConfigCtl::validNiSaveBits( QString &err, DAQ::Params &q )
{
    if( !doingNidq() )
        return true;

    return q.sns.niChans.deriveSaveBits(
            err, q.ni.niCumTypCnt[CniCfg::niSumAll] );
}


bool ConfigCtl::validDiskAvail( QString &err, DAQ::Params &q )
{
    if( q.sns.reqMins <= 0 )
        return true;

    double  BPS     = 0;
    quint64 avail   = availableDiskSpace();
    int     mins;

    if( doingImec() )
        BPS += q.sns.imChans.saveBits.count( true ) * q.im.srate * 2;

    if( doingNidq() )
        BPS += q.sns.niChans.saveBits.count( true ) * q.ni.srate * 2;

    mins = avail / BPS / 60;

    if( mins <= q.sns.reqMins ) {

        err =
        QString("Space to record for ~%1 min (required = %2).")
        .arg( mins )
        .arg( q.sns.reqMins );
        return false;
    }

    return true;
}


bool ConfigCtl::diskParamsToQ( QString &err, DAQ::Params &q )
{
    err.clear();

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

    if( !validImSaveBits( err, q ) )
        return false;

    if( !validNiSaveBits( err, q ) )
        return false;

    return true;
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

    if( !validDevTab( err, q ) )
        return false;

    if( !validImROTbl( err, q ) )
        return false;

    if( !validNiDevices( err, q )
        || !validNiChannels( err, q,
                vcMN1, vcMA1, vcXA1, vcXD1,
                vcMN2, vcMA2, vcXA2, vcXD2,
                uiStr1Err, uiStr2Err ) ) {

        return false;
    }

    QString stream;

    if( q.mode.mTrig == DAQ::eTrigTTL )
        stream = q.trgTTL.stream;
    else if( q.mode.mTrig == DAQ::eTrigSpike )
        stream = q.trgSpike.stream;

    if( stream == "imec" && !validImTriggering( err, q ) )
        return false;

    if( stream == "nidq" && !validNiTriggering( err, q ) )
        return false;

    if( !validImChanMap( err, q ) )
        return false;

    if( !validNiChanMap( err, q ) )
        return false;

    if( !validImSaveBits( err, q ) )
        return false;

    if( !validNiSaveBits( err, q ) )
        return false;

    if( !validDiskAvail( err, q ) )
        return false;

    if( !validRunName( err, q.sns.runName, cfgDlg, isGUI ) )
        return false;

// -------------
// Accept params
// -------------

    acceptedParams  = q;
    validated       = true;

// ----
// Save
// ----

    acceptedParams.saveSettings();

    return true;
}


