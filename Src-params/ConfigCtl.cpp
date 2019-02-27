
#include "ui_ConfigureDialog.h"
#include "ui_DevicesTab.h"
#include "ui_IMCfgTab.h"
#include "ui_NICfgTab.h"
#include "ui_SyncTab.h"
#include "ui_GateTab.h"
#include "ui_GateImmedPanel.h"
#include "ui_GateTCPPanel.h"
#include "ui_TrigTab.h"
#include "ui_TrigImmedPanel.h"
#include "ui_TrigTimedPanel.h"
#include "ui_TrigTTLPanel.h"
#include "ui_TrigSpikePanel.h"
#include "ui_TrigTCPPanel.h"
#include "ui_MapTab.h"
#include "ui_SeeNSaveTab.h"
#include "ui_IMForceDlg.h"

#include "Pixmaps/Icon-Config.xpm"

#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "HelpButDialog.h"
#include "IMROEditor.h"
#include "ChanMapCtl.h"
#include "ShankMapCtl.h"
#include "ColorTTLCtl.h"
#include "Subset.h"
#include "Version.h"

#include <QSharedMemory>
#include <QMessageBox>
#include <QDirIterator>
#include <QDesktopServices>

#include <math.h>


#define CURDEV1     niTabUI->device1CB->currentIndex()
#define CURDEV2     niTabUI->device2CB->currentIndex()


static const char *DEF_IMRO_LE = "*Default (bank 0, ref ext, gain 500/250)";
static const char *DEF_SKMP_LE = "*Default (1 shk, 2 col, tip=[0,0])";
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
        syncTabUI(0),
        gateTabUI(0),
            gateImmPanelUI(0),
            gateTCPPanelUI(0),
        trigTabUI(0),
            trigImmPanelUI(0), trigTimPanelUI(0),
            trigTTLPanelUI(0), trigSpkPanelUI(0),
            trigTCPPanelUI(0),
        mapTabUI(0),
        snsTabUI(0),
        cfgDlg(0),
        singleton(0),
        imecOK(false), nidqOK(false),
        validated(false)
{
    QVBoxLayout *L;
    QWidget     *panel;

// -----------
// Main dialog
// -----------

    cfgDlg = new HelpButDialog(
                    "Configuration Help",
                    "CommonResources/UserManual.html" );
    cfgDlg->setWindowIcon( QIcon(QPixmap(Icon_Config_xpm)) );
    cfgDlg->setAttribute( Qt::WA_DeleteOnClose, false );

    cfgUI = new Ui::ConfigureDialog;
    cfgUI->setupUi( cfgDlg );
    cfgUI->tabsW->setCurrentIndex( 0 );
    ConnectUI( cfgUI->resetBut, SIGNAL(clicked()), this, SLOT(reset()) );
    ConnectUI( cfgUI->verifyBut, SIGNAL(clicked()), this, SLOT(verify()) );
    ConnectUI( cfgUI->buttonBox, SIGNAL(accepted()), this, SLOT(okButClicked()) );

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
    ConnectUI( devTabUI->detectBut, SIGNAL(clicked()), this, SLOT(detectButClicked()) );
    ConnectUI( devTabUI->skipBut, SIGNAL(clicked()), this, SLOT(skipButClicked()) );

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
    ConnectUI( niTabUI->startEnabChk, SIGNAL(clicked(bool)), this, SLOT(startEnableClicked(bool)) );

// -------
// SyncTab
// -------

    syncTabUI = new Ui::SyncTab;
    syncTabUI->setupUi( cfgUI->syncTab );
    ConnectUI( syncTabUI->sourceCB, SIGNAL(currentIndexChanged(int)), this, SLOT(syncSourceCBChanged()) );
    ConnectUI( syncTabUI->imChanCB, SIGNAL(currentIndexChanged(int)), this, SLOT(syncImChanTypeCBChanged()) );
    ConnectUI( syncTabUI->niChanCB, SIGNAL(currentIndexChanged(int)), this, SLOT(syncNiChanTypeCBChanged()) );
    ConnectUI( syncTabUI->calChk, SIGNAL(clicked(bool)), this, SLOT(syncCalChkClicked()) );

// -------
// GateTab
// -------

    gateTabUI = new Ui::GateTab;
    gateTabUI->setupUi( cfgUI->gateTab );
    ConnectUI( gateTabUI->gateModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(gateModeChanged()) );
    ConnectUI( gateTabUI->manOvShowButChk, SIGNAL(clicked(bool)), this, SLOT(manOvShowButClicked(bool)) );

    L = new QVBoxLayout( gateTabUI->gateFrame );

// Immediate
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eGateImmed ) );
    gateImmPanelUI = new Ui::GateImmedPanel;
    gateImmPanelUI->setupUi( panel );
    L->addWidget( panel );

// TCP
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eGateTCP ) );
    gateTCPPanelUI = new Ui::GateTCPPanel;
    gateTCPPanelUI->setupUi( panel );
    L->addWidget( panel );

// -------
// TrigTab
// -------

    trigTabUI = new Ui::TrigTab;
    trigTabUI->setupUi( cfgUI->trigTab );
    ConnectUI( trigTabUI->trigModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(trigModeChanged()) );

    L = new QVBoxLayout( trigTabUI->trigFrame );

// Immediate
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigImmed ) );
    trigImmPanelUI = new Ui::TrigImmedPanel;
    trigImmPanelUI->setupUi( panel );
    L->addWidget( panel );

// Timed
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTimed ) );
    trigTimPanelUI = new Ui::TrigTimedPanel;
    trigTimPanelUI->setupUi( panel );
    ConnectUI( trigTimPanelUI->HInfRadio, SIGNAL(clicked()), this, SLOT(trigTimHInfClicked()) );
    ConnectUI( trigTimPanelUI->cyclesRadio, SIGNAL(clicked()), this, SLOT(trigTimHInfClicked()) );
    ConnectUI( trigTimPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigTimNInfClicked(bool)) );

    QButtonGroup    *bgTim = new QButtonGroup( panel );
    bgTim->addButton( trigTimPanelUI->HInfRadio );
    bgTim->addButton( trigTimPanelUI->cyclesRadio );
    L->addWidget( panel );

// TTL
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTTL ) );
    trigTTLPanelUI = new Ui::TrigTTLPanel;
    trigTTLPanelUI->setupUi( panel );
    ConnectUI( trigTTLPanelUI->analogRadio, SIGNAL(clicked()), this, SLOT(trigTTLAnalogChanged()) );
    ConnectUI( trigTTLPanelUI->digRadio, SIGNAL(clicked()), this, SLOT(trigTTLAnalogChanged()) );
    ConnectUI( trigTTLPanelUI->modeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(trigTTLModeChanged(int)) );
    ConnectUI( trigTTLPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigTTLNInfClicked(bool)) );
    L->addWidget( panel );

// Spike
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigSpike ) );
    trigSpkPanelUI = new Ui::TrigSpikePanel;
    trigSpkPanelUI->setupUi( panel );
    ConnectUI( trigSpkPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigSpkNInfClicked(bool)) );
    L->addWidget( panel );

// TCP
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTCP ) );
    trigTCPPanelUI = new Ui::TrigTCPPanel;
    trigTCPPanelUI->setupUi( panel );
    L->addWidget( panel );

// ------
// MapTab
// ------

    mapTabUI = new Ui::MapTab;
    mapTabUI->setupUi( cfgUI->mapTab );
    ConnectUI( mapTabUI->imShkMapBut, SIGNAL(clicked()), this, SLOT(imShkMapButClicked()) );
    ConnectUI( mapTabUI->niShkMapBut, SIGNAL(clicked()), this, SLOT(niShkMapButClicked()) );
    ConnectUI( mapTabUI->imChnMapBut, SIGNAL(clicked()), this, SLOT(imChnMapButClicked()) );
    ConnectUI( mapTabUI->niChnMapBut, SIGNAL(clicked()), this, SLOT(niChnMapButClicked()) );

// -----------
// SeeNSaveTab
// -----------

    snsTabUI = new Ui::SeeNSaveTab;
    snsTabUI->setupUi( cfgUI->snsTab );
    ConnectUI( snsTabUI->dataDirBut, SIGNAL(clicked()), this, SLOT(dataDirButClicked()) );
    ConnectUI( snsTabUI->diskBut, SIGNAL(clicked()), this, SLOT(diskButClicked()) );
}


ConfigCtl::~ConfigCtl()
{
    singletonRelease();

    if( snsTabUI ) {
        delete snsTabUI;
        snsTabUI = 0;
    }

    if( mapTabUI ) {
        delete mapTabUI;
        mapTabUI = 0;
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

    if( syncTabUI ) {
        delete syncTabUI;
        syncTabUI = 0;
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
    setupGateTab( acceptedParams ); // adjusts initial dialog sizing
    setupTrigTab( acceptedParams ); // adjusts initial dialog sizing
    syncTabUI->calChk->setChecked( false );
    devTabUI->detectBut->setDefault( true );
    devTabUI->detectBut->setFocus();

// ----------
// Run dialog
// ----------

    int retCode = cfgDlg->exec();

    cfgDlg->close();   // delete help dialog

    return retCode == QDialog::Accepted;
}


bool ConfigCtl::isConfigDlg( QObject *parent )
{
    return parent == cfgDlg;
}


void ConfigCtl::setParams( const DAQ::Params &p, bool write )
{
    acceptedParams = p;

    if( write )
        acceptedParams.saveSettings();
}


void ConfigCtl::externSetsRunName( const QString &name )
{
    if( !validated )
        return;

    QString strippedName = name;
    QRegExp re("(.*)_[gG]\\d+_[tT]\\d+$");

    if( strippedName.contains( re ) )
        strippedName = re.cap(1);

    acceptedParams.sns.runName = strippedName;
    acceptedParams.saveSettings();

    if( cfgDlg->isVisible() )
        snsTabUI->runNameLE->setText( strippedName );
}


// Note: This is called if the user R-clicks imec graphs,
// edits imro table and makes changes. Here we further
// test if any of the bank values have changed because
// that situation requires regeneration of the shankMap.
//
void ConfigCtl::graphSetsImroFile( const QString &file )
{
    DAQ::Params &p = acceptedParams;
    QString     err;

    IMROTbl T_old = p.im.roTbl;

    p.im.imroFile = file;

    if( validImROTbl( err, p ) ) {

        if( !p.im.roTbl.banksSame( T_old ) ) {

            // Force default shankMap from imro
            p.sns.imChans.shankMapFile.clear();
            validImShankMap( err, p );
        }

        p.saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsStdbyStr( const QString &sdtbyStr )
{
    DAQ::Params &p = acceptedParams;
    QString     err;

    p.im.stdbyStr = sdtbyStr;

    if( validImStdbyBits( err, p ) ) {

        if( imVers.opt == 3 ) {

            p.sns.imChans.shankMap = p.sns.imChans.shankMap_orig;
            p.sns.imChans.shankMap.andOutImStdby( p.im.stdbyBits );
        }

        p.saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsImChanMap( const QString &cmFile )
{
    DAQ::Params &p      = acceptedParams;
    QString     msg,
                err;
    const int   *type   = p.im.imCumTypCnt;

    ChanMapIM &M = p.sns.imChans.chanMap;
    ChanMapIM D(
        type[CimCfg::imTypeAP],
        type[CimCfg::imTypeLF] - type[CimCfg::imTypeAP],
        type[CimCfg::imTypeSY] - type[CimCfg::imTypeLF] );

    if( cmFile.isEmpty() ) {

        M = D;
        M.fillDefault();
    }
    else if( !M.loadFile( msg, cmFile ) )
        err = QString("ChanMap: %1.").arg( msg );
    else if( !M.equalHdr( D ) ) {

        err = QString(
                "ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3)\n"
                "  - Named file: (%4 %5 %6).")
                .arg( D.AP ).arg( D.LF ).arg( D.SY )
                .arg( M.AP ).arg( M.LF ).arg( M.SY );
    }

    if( err.isEmpty() ) {

        p.sns.imChans.chanMapFile = cmFile;
        p.saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsNiChanMap( const QString &cmFile )
{
    DAQ::Params &p      = acceptedParams;
    QString     msg,
                err;
    const int   *type   = p.ni.niCumTypCnt;
    int         nMux    = p.ni.muxFactor;

    ChanMapNI &M = p.sns.niChans.chanMap;
    ChanMapNI D(
        type[CniCfg::niTypeMN] / nMux,
        (type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN]) / nMux,
        nMux,
        type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA],
        type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

    if( cmFile.isEmpty() ) {

        M = D;
        M.fillDefault();
    }
    else if( !M.loadFile( msg, cmFile ) )
        err = QString("ChanMap: %1.").arg( msg );
    else if( !M.equalHdr( D ) ) {

        err = QString(
                "ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3 %4 %5)\n"
                "  - Named file: (%6 %7 %8 %9 %10).")
                .arg( D.MN ).arg( D.MA ).arg( D.C ).arg( D.XA ).arg( D.XD )
                .arg( M.MN ).arg( M.MA ).arg( M.C ).arg( M.XA ).arg( M.XD );
    }

    if( err.isEmpty() ) {

        p.sns.niChans.chanMapFile = cmFile;
        p.saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsImSaveStr( const QString &saveStr )
{
    DAQ::Params &p = acceptedParams;
    QString     oldStr, err;

    oldStr = p.sns.imChans.uiSaveChanStr;
    p.sns.imChans.uiSaveChanStr = saveStr;

    if( validImSaveBits( err, p ) )
        p.saveSettings();
    else {
        p.sns.imChans.uiSaveChanStr = oldStr;
        Error() << err;
    }
}


void ConfigCtl::graphSetsNiSaveStr( const QString &saveStr )
{
    DAQ::Params &p = acceptedParams;
    QString     oldStr, err;

    oldStr = p.sns.niChans.uiSaveChanStr;
    p.sns.niChans.uiSaveChanStr = saveStr;

    if( validNiSaveBits( err, p ) )
        p.saveSettings();
    else {
        p.sns.niChans.uiSaveChanStr = oldStr;
        Error() << err;
    }
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

    QRegExp         re( QString("%1_g\\d+|%1\\.").arg( runName ) );
    QDirIterator    it( mainApp()->dataDir() );

    re.setCaseSensitivity( Qt::CaseInsensitive );

    while( it.hasNext() ) {

        it.next();

        if( re.indexIn( it.fileName() ) == 0 )
            return true;
    }

    return false;
}


// The filenaming policy:
// Names (bin, meta) have pattern: runDir/runName_gN_tM.nidq.bin.
// The run name must be unique in dataDir for formal usage.
// We will, however, warn and offer to overwrite existing
// file(s) because it is so useful for test and development.
//
bool ConfigCtl::validRunName(
    QString         &err,
    const QString   &runName,
    QWidget         *parent,
    bool            isGUI ) const
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
        "Run name already exists, overwrite it? '%1'")
        .arg( runName ),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    if( yesNo != QMessageBox::Yes )
        return false;

    return true;
}


// Get standard channel string listing channels in ShankMap order.
//
bool ConfigCtl::chanMapGetsShankOrder(
    QString         &s,
    const QString   type,
    bool            rev,
    QWidget         *parent ) const
{
    DAQ::Params q;
    QString     err;

    if( !shankParamsToQ( err, q ) ) {
        QMessageBox::critical( parent, "ACQ Parameter Error", err );
        return false;
    }

    if( type == "nidq" ) {

        if( rev )
            q.sns.niChans.shankMap.revChanOrderFromMapNi( s );
        else
            q.sns.niChans.shankMap.chanOrderFromMapNi( s );
    }
    else {

        if( rev )
            q.sns.imChans.shankMap.revChanOrderFromMapIm( s );
        else
            q.sns.imChans.shankMap.chanOrderFromMapIm( s );
    }

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
QString ConfigCtl::cmdSrvSetsParamStr( const QString &paramString )
{
    if( !validated )
        return "Run parameters never validated.";

// -------------------------------
// Save settings to "_remote" file
// -------------------------------

// First write current set

    acceptedParams.saveSettings( true );

// Then overwrite entries

    DAQ::Params::str2RemoteSettings( paramString );

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

    if( nidqOK ) {

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
void ConfigCtl::detectButClicked()
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


void ConfigCtl::skipButClicked()
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

    if( doingImec() )
        imWriteCurrent();

// Just do NI again, it's quick

    nidqOK = false;

    if( devTabUI->nidqGB->isChecked() )
        niDetect();

    setSelectiveAccess();
}


void ConfigCtl::forceButClicked()
{
    HelpButDialog   D( "Working with Broken EEPROMs",
                       "CommonResources/Broken_3A_EEPROM.html" );

    forceUI = new Ui::IMForceDlg;
    forceUI->setupUi( &D );
    forceUI->lbLE->setText( imVers.pLB );
    forceUI->snLE->setText( imVers.pSN );
    forceUI->optCB->setCurrentIndex( imVers.opt - 1 );
    forceUI->gainChk->setChecked( imTabUI->gainCorChk->isChecked() );
    ConnectUI( forceUI->exploreBut, SIGNAL(clicked()), this, SLOT(exploreButClicked()) );
    ConnectUI( forceUI->stripBut, SIGNAL(clicked()), this, SLOT(stripButClicked()) );
    ConnectUI( forceUI->optCB, SIGNAL(currentIndexChanged(int)), this, SLOT(optCBChanged(int)) );

    optCBChanged( imVers.opt - 1 );

    QPushButton *B;

    B = forceUI->buttonBox->button( QDialogButtonBox::Ok );
    B->setDefault( false );

    B = forceUI->buttonBox->button( QDialogButtonBox::Cancel );
    B->setDefault( true );
    B->setFocus();

    if( QDialog::Accepted == D.exec() ) {

        QString lbl = forceUI->lbLE->text().trimmed(),
                path;

        if( lbl.isEmpty() ) {
            QMessageBox::information(
            cfgDlg,
            "Label String Empty",
            "Canceling: Blank probe label string." );
            goto exit;
        }

        path = QString("%1/ImecProbeData/%2").arg( appPath() ).arg( lbl );

        if( !QDir( path ).exists() ) {
            QMessageBox::information(
            cfgDlg,
            "Can't Find Data",
            QString("Canceling: Can't find probe folder '%1'.").arg( path ) );
            goto exit;
        }

        imVers.pLB      = forceUI->lbLE->text().trimmed();
        imVers.pSN      = forceUI->snLE->text().trimmed();
        imVers.opt      = forceUI->optCB->currentText().toInt();
        imVers.force    = true;
        imVers.skipADC  = forceUI->skipChk->isChecked();

        imTabUI->snLE->setText( imVers.pSN );
        imTabUI->optLE->setText( QString::number( imVers.opt ) );

        if( imVers.opt == 2 ) {
            imTabUI->gainCorChk->setEnabled( false );
            imTabUI->gainCorChk->setChecked( false );
        }
        else {
            imTabUI->gainCorChk->setEnabled( true );
            imTabUI->gainCorChk->setChecked( forceUI->gainChk->isChecked() );
        }

        imTabUI->stdbyLE->setEnabled( imVers.opt == 3 );

        imWriteCurrent();
    }

exit:
    delete forceUI;
}


void ConfigCtl::exploreButClicked()
{
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(
            appPath() + "/ImecProbeData" ) );
}


void ConfigCtl::stripButClicked()
{
    QString s = forceUI->lbLE->text().trimmed();
    int     n = s.count();

    if( n == 11 )
        forceUI->snLE->setText( s.mid( 1, 9 ) );
    else if( n >= 9 )
        forceUI->snLE->setText( s.right( 9 ) );
    else {
        QString z = "000000000";
        forceUI->snLE->setText( z.left( 9 - n ) + s );
    }
}


void ConfigCtl::optCBChanged( int opt )
{
    if( opt + 1 == 2 ) {
        forceUI->gainChk->setEnabled( false );
        forceUI->gainChk->setChecked( false );
    }
    else
        forceUI->gainChk->setEnabled( true );
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

    setupNiVRangeCB();

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
                std::min( CniCfg::maxSampleRate( devStr ), SGLX_NI_MAXRATE );

    niTabUI->srateSB->setMinimum( minRate );
    niTabUI->srateSB->setMaximum( maxRate );

// --------------------
// Observe dependencies
// --------------------

    muxingChanged();
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

        // Currently the Whisper is the only way to do multiplexing,
        // and the Whisper is currently hardcoded to read its clock
        // input on PFI2. Hence, if multiplexing, we select item PFI2
        // on both devices and disable other choices.
        //
        // Whisper is currently hardcoded to read the run start signal
        // on digital line zero. If multiplexing, we force that choice.

        int ci;

        ci = niTabUI->clk1CB->findText( "PFI2", Qt::MatchExactly );
        niTabUI->clk1CB->setCurrentIndex( ci > -1 ? ci : 0 );

        ci = niTabUI->clk2CB->findText( "PFI2", Qt::MatchExactly );
        niTabUI->clk2CB->setCurrentIndex( ci > -1 ? ci : 0 );

        niTabUI->startEnabChk->setChecked( true );

        if( devNames.count() ) {
            ci = niTabUI->startCB->findText(
                    QString("%1/port0/line0").arg( devNames[CURDEV1] ) );
        }
        else
            ci = -1;

        niTabUI->startCB->setCurrentIndex( ci > -1 ? ci : 0 );
    }
    else {

        bool    wasMux = acceptedParams.ni.isMuxingMode();

        if( wasMux )
            snsTabUI->niSaveChansLE->setText( "all" );
    }

    niTabUI->clk1CB->setDisabled( isMux );
    niTabUI->clk2CB->setDisabled( isMux || !niTabUI->dev2GB->isChecked() );
    niTabUI->startEnabChk->setDisabled( isMux );
    niTabUI->startCB->setDisabled( isMux );
}


void ConfigCtl::clk1CBChanged()
{
    niTabUI->srateSB->setEnabled( niTabUI->clk1CB->currentIndex() == 0 );
}


void ConfigCtl::startEnableClicked( bool checked )
{
    niTabUI->startCB->setEnabled( checked && !isMuxingFromDlg() );
}


void ConfigCtl::syncSourceCBChanged()
{
    DAQ::SyncSource sourceIdx =
        (DAQ::SyncSource)syncTabUI->sourceCB->currentIndex();

    syncTabUI->sourceSB->setEnabled( sourceIdx != DAQ::eSyncSourceNone );

    if( sourceIdx == DAQ::eSyncSourceNI ) {
        syncTabUI->sourceLE->setText(
            "Connect PFI-13 to stream inputs specified below" );
    }
    else if( sourceIdx == DAQ::eSyncSourceExt ) {
        syncTabUI->sourceLE->setText(
            "Connect pulser output to stream inputs specified below" );
    }
    else {
        syncTabUI->sourceLE->setText(
            "We will apply the most recently measured sample rates" );

        syncTabUI->calChk->setChecked( false );
    }

    syncTabUI->calChk->setEnabled( sourceIdx != DAQ::eSyncSourceNone );

    syncImChanTypeCBChanged();
    syncNiChanTypeCBChanged();
    syncCalChkClicked();
}


void ConfigCtl::syncImChanTypeCBChanged()
{
    bool enab   = doingImec()
                    &&
                    ((DAQ::SyncSource)syncTabUI->sourceCB->currentIndex()
                    != DAQ::eSyncSourceNone);
//         enabT  = enab && syncTabUI->imChanCB->currentIndex() == 1;

    syncTabUI->imChanCB->setEnabled( enab );
    syncTabUI->imChanSB->setEnabled( enab );
//    syncTabUI->imThreshSB->setEnabled( enabT );
}


void ConfigCtl::syncNiChanTypeCBChanged()
{
    bool enab   = doingNidq()
                    &&
                    ((DAQ::SyncSource)syncTabUI->sourceCB->currentIndex()
                    != DAQ::eSyncSourceNone),
         enabT  = enab && syncTabUI->niChanCB->currentIndex() == 1;

    syncTabUI->niChanCB->setEnabled( enab );
    syncTabUI->niChanSB->setEnabled( enab );
    syncTabUI->niThreshSB->setEnabled( enabT );
}


void ConfigCtl::syncCalChkClicked()
{
    syncTabUI->minSB->setEnabled(
        syncTabUI->calChk->isEnabled()
        && syncTabUI->calChk->isChecked() );
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


void ConfigCtl::imShkMapButClicked()
{
// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    DAQ::Params q;
    QString     err;

    q.im.imroFile = imTabUI->imroLE->text().trimmed();

    if( !validImROTbl( err, q ) ) {

        if( !err.isEmpty() )
            QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
        return;
    }

// -------------
// Launch editor
// -------------

    ShankMapCtl  SM( cfgDlg, q.im.roTbl, "imec", q.im.roTbl.nChan() );

    QString mapFile = SM.Edit( mapTabUI->imShkMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        mapTabUI->imShkMapLE->setText( DEF_SKMP_LE );
    else
        mapTabUI->imShkMapLE->setText( mapFile );
}


void ConfigCtl::niShkMapButClicked()
{
// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    CniCfg  ni;

    if( !niChannelsFromDialog( ni ) )
        return;

// -------------
// Launch editor
// -------------

    ShankMapCtl  SM( cfgDlg, IMROTbl(), "nidq", ni.niCumTypCnt[CniCfg::niTypeMN] );

    QString mapFile = SM.Edit( mapTabUI->niShkMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        mapTabUI->niShkMapLE->setText( DEF_SKMP_LE );
    else
        mapTabUI->niShkMapLE->setText( mapFile );
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

    QString mapFile = CM.Edit( mapTabUI->imChnMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        mapTabUI->imChnMapLE->setText( DEF_CHMP_LE );
    else
        mapTabUI->imChnMapLE->setText( mapFile );
}


void ConfigCtl::niChnMapButClicked()
{
// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    CniCfg  ni;

    if( !niChannelsFromDialog( ni ) )
        return;

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

    QString mapFile = CM.Edit( mapTabUI->niChnMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        mapTabUI->niChnMapLE->setText( DEF_CHMP_LE );
    else
        mapTabUI->niChnMapLE->setText( mapFile );
}


void ConfigCtl::dataDirButClicked()
{
    MainApp *app = mainApp();

    app->options_PickDataDir();
    snsTabUI->dataDirLbl->setText( app->dataDir() );
}


void ConfigCtl::diskButClicked()
{
    snsTabUI->diskTE->clear();

    DAQ::Params q;
    QString     err;

    if( !validDataDir( err ) ) {
        diskWrite( err );
        return;
    }

    if( !diskParamsToQ( err, q ) ) {
        diskWrite( "Parameter error" );
        QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
        return;
    }

    double  BPS = 0;

    if( doingImec() ) {

        int     ch  = q.apSaveChanCount();
        double  bps = ch * q.im.srate * 2;

        BPS += bps;

        QString s =
            QString("AP: %1 chn @ %2 Hz = %3 MB/s")
            .arg( ch )
            .arg( int(q.im.srate) )
            .arg( bps / (1024*1024), 0, 'f', 3 );

        diskWrite( s );

        ch  = q.lfSaveChanCount();
        bps = ch * q.im.srate/12 * 2;

        BPS += bps;

        s = QString("LF: %1 chn @ %2 Hz = %3 MB/s")
            .arg( ch )
            .arg( int(q.im.srate/12) )
            .arg( bps / (1024*1024), 0, 'f', 3 );

        diskWrite( s );
    }

    if( doingNidq() ) {

        int     ch  = q.sns.niChans.saveBits.count( true );
        double  bps = ch * q.ni.srate * 2;

        BPS += bps;

        QString s =
            QString("NI: %1 chn @ %2 Hz = %3 MB/s")
            .arg( ch )
            .arg( int(q.ni.srate) )
            .arg( bps / (1024*1024), 0, 'f', 3 );

        diskWrite( s );
    }

    quint64 avail = availableDiskSpace();

    QString s =
        QString("Avail: %1 GB / %2 MB/s = %3 min")
        .arg( avail / (1024UL*1024*1024) )
        .arg( BPS / (1024*1024), 0, 'f', 3 )
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


void ConfigCtl::trigTTLAnalogChanged()
{
    bool isAna = trigTTLPanelUI->analogRadio->isChecked();

    trigTTLPanelUI->aStreamCB->setEnabled( isAna );
    trigTTLPanelUI->chanLabel->setEnabled( isAna );
    trigTTLPanelUI->aChanSB->setEnabled( isAna );
    trigTTLPanelUI->TLabel->setEnabled( isAna );
    trigTTLPanelUI->TSB->setEnabled( isAna );
    trigTTLPanelUI->TUnitLabel->setEnabled( isAna );

    trigTTLPanelUI->dStreamCB->setDisabled( isAna );
    trigTTLPanelUI->bitLabel->setDisabled( isAna );
    trigTTLPanelUI->dBitSB->setDisabled( isAna );
    trigTTLPanelUI->gohiLabel->setDisabled( isAna );
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
            txt = "write for N seconds";
            break;
        case DAQ::TrgTTLFollowV:
            txt = "write while voltage is high.";
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

    if( imecOK )
        setupImTab( p );

    if( nidqOK )
        setupNiTab( p );

    if( imecOK || nidqOK ) {
        setupSyncTab( p );
        setupGateTab( p );
        setupTrigTab( p );
        setupMapTab( p );
        setupSnsTab( p );
    }
}


void ConfigCtl::verify()
{
    QString err;

    if( valid( err, true ) )
        ;
    else if( !err.isEmpty() )
        QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
}


void ConfigCtl::okButClicked()
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
    snsTabUI->diskTE->clear();

// Can't tab

    for( int i = 1, n = cfgUI->tabsW->count(); i < n; ++i )
        cfgUI->tabsW->setTabEnabled( i, false );

// Can't verify or ok

    cfgUI->resetBut->setDisabled( true );
    cfgUI->verifyBut->setDisabled( true );
    cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setDisabled( true );
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

    if( imecOK ) {
        setupImTab( acceptedParams );
        cfgUI->tabsW->setTabEnabled( 1, true );
    }

    if( nidqOK ) {
        setupNiTab( acceptedParams );
        cfgUI->tabsW->setTabEnabled( 2, true );
    }

    if( imecOK || nidqOK ) {
        setupSyncTab( acceptedParams );
        setupGateTab( acceptedParams );
        setupTrigTab( acceptedParams );
        setupMapTab( acceptedParams );
        setupSnsTab( acceptedParams );
        cfgUI->tabsW->setTabEnabled( 3, true );
        cfgUI->tabsW->setTabEnabled( 4, true );
        cfgUI->tabsW->setTabEnabled( 5, true );
        cfgUI->tabsW->setTabEnabled( 6, true );
        cfgUI->tabsW->setTabEnabled( 7, true );
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
        "Then click 'Detect' to see what's installed/working." );
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


void ConfigCtl::imWriteCurrent()
{
    QTextEdit   *te = devTabUI->imTE;
    te->clear();
    imWrite( "Previous data:" );
    imWrite( "-----------------------------------" );
    imWrite( QString("Hardware version %1").arg( imVers.hwr ) );
    imWrite( QString("Basestation version %1").arg( imVers.bas ) );
    imWrite( QString("API version %1").arg( imVers.api ) );
    imWrite( QString("Probe serial# %1").arg( imVers.pSN ) );
    imWrite( QString("Probe option  %1").arg( imVers.opt ) );
    imWrite(
        QString("\nOK -- FORCED ID:    %1")
        .arg( imVers.force ? "ON" : "OFF" ) );
}


void ConfigCtl::imDetect()
{
// --------------
// Query hardware
// --------------

    QStringList sl;

    imWrite( "Connecting...allow several seconds." );
    guiBreathe();

    imecOK = CimCfg::getVersions( sl, imVers );

// -------------
// Write reports
// -------------

    QTextEdit   *te = devTabUI->imTE;

    te->clear();

    foreach( const QString &s, sl )
        imWrite( s );

    if( imecOK ) {

        if( imVers.opt < 1 || imVers.opt > 4 ) {
            imWrite(
                QString("\n** Illegal probe option (%1), must be [1..4].")
                .arg( imVers.opt ) );
            imecOK = false;
        }
    }

    if( imecOK ) {
        imWrite(
            QString("\nOK -- FORCED ID:    %1")
            .arg( imVers.force ? "ON" : "OFF" ) );
    }
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
// -------------
// Report header
// -------------

    niWrite( "Multifunction Input Devices:" );
    niWrite( "-----------------------------------" );

// --------------------------------
// Error if hardware already in use
// --------------------------------

    if( !singletonReserve() ) {
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

// --------------------------------
// Report devs having both [AI, DI]
// --------------------------------

    foreach( const QString &D, CniCfg::niDevNames ) {

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

// ---------------------
// Query output hardware
// ---------------------

    CniCfg::probeAOHardware();

// --------------------
// Output report header
// --------------------

    niWrite( "\nAnalog Output Devices:" );
    niWrite( "-----------------------------------" );

// ---------------------------------------------------
// Report devs having [AO]; Note: {} allows goto exit.
// ---------------------------------------------------

    {
        QStringList devs = CniCfg::aoDevChanCount.uniqueKeys();

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
    }

// -------------
// Finish report
// -------------

exit:
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


void ConfigCtl::setupDevTab( const DAQ::Params &p )
{
    devTabUI->imecGB->setChecked( p.im.enabled );
    devTabUI->nidqGB->setChecked( p.ni.enabled );

    devTabUI->skipBut->setEnabled( doingImec() || doingNidq() );

// --------------------
// Observe dependencies
// --------------------
}


void ConfigCtl::setupImTab( const DAQ::Params &p )
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

    if( imVers.opt == 2 ) {
        imTabUI->gainCorChk->setEnabled( false );
        imTabUI->gainCorChk->setChecked( false );
    }
    else {
        imTabUI->gainCorChk->setEnabled( true );
        imTabUI->gainCorChk->setChecked( p.im.doGainCor );
    }

    QString s = p.im.imroFile;

    if( s.contains( "*" ) )
        s.clear();

    if( s.isEmpty() )
        imTabUI->imroLE->setText( DEF_IMRO_LE );
    else
        imTabUI->imroLE->setText( s );

    imTabUI->stdbyLE->setText( p.im.stdbyStr );
    imTabUI->stdbyLE->setEnabled( imVers.opt == 3 );

    imTabUI->noLEDChk->setChecked( p.im.noLEDs );

// --------------------
// Observe dependencies
// --------------------
}


void ConfigCtl::setupNiTab( const DAQ::Params &p )
{
    niTabUI->srateSB->setValue( p.ni.srateSet );
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
    }

// Start

    niTabUI->startEnabChk->setChecked( p.ni.startEnable );
    niTabUI->startCB->clear();

    {
        QStringList L  = CniCfg::getAllDOLines();
        int         sel;

        foreach( QString s, L )
            niTabUI->startCB->addItem( s );

        sel = niTabUI->startCB->findText( p.ni.startLine );

        if( sel < 0 ) {
            sel = niTabUI->startCB->findText(
                    QString("%1/port0/line0").arg( devNames[CURDEV1] ) );
        }

        niTabUI->startCB->setCurrentIndex( sel );
    }

// --------------------
// Observe dependencies
// --------------------

    device1CBChanged(); // <-- Call This First!! - Fills in other CBs
    device2CBChanged();
    muxingChanged();
    clk1CBChanged();
    startEnableClicked( p.ni.startEnable );
}


void ConfigCtl::setupSyncTab( const DAQ::Params &p )
{
// Source

    syncTabUI->sourceCB->setCurrentIndex( (int)p.sync.sourceIdx );
    syncTabUI->sourceSB->setValue( p.sync.sourcePeriod );

// Channels

    syncTabUI->imChanCB->setCurrentIndex( p.sync.imChanType );
    syncTabUI->niChanCB->setCurrentIndex( p.sync.niChanType );
    syncTabUI->imChanSB->setValue( p.sync.imChan );
    syncTabUI->niChanSB->setValue( p.sync.niChan );
//    syncTabUI->imThreshSB->setValue( p.sync.imThresh );
    syncTabUI->niThreshSB->setValue( p.sync.niThresh );

// Calibration

    syncTabUI->minSB->setValue( p.sync.calMins );

// Measured sample rates

    syncTabUI->imRateSB->setValue( p.im.srate );
    syncTabUI->niRateSB->setValue( p.ni.srate );

    syncTabUI->imRateSB->setEnabled( imecOK );
    syncTabUI->niRateSB->setEnabled( nidqOK );

// --------------------
// Observe dependencies
// --------------------

    syncSourceCBChanged();
}


void ConfigCtl::setupGateTab( const DAQ::Params &p )
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


void ConfigCtl::setupTrigTab( const DAQ::Params &p )
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

    QButtonGroup    *bg;

    bg = new QButtonGroup( this );
    bg->addButton( trigTTLPanelUI->analogRadio );
    bg->addButton( trigTTLPanelUI->digRadio );

    trigTTLPanelUI->TSB->setValue( p.trgTTL.T );
    trigTTLPanelUI->marginSB->setValue( p.trgTTL.marginSecs );
    trigTTLPanelUI->refracSB->setValue( p.trgTTL.refractSecs );
    trigTTLPanelUI->HSB->setValue( p.trgTTL.tH );
    trigTTLPanelUI->aStreamCB->setCurrentIndex( p.trgTTL.stream == "nidq" );
    trigTTLPanelUI->dStreamCB->setCurrentIndex( p.trgTTL.stream == "nidq" );
    trigTTLPanelUI->modeCB->setCurrentIndex( p.trgTTL.mode );
    trigTTLPanelUI->aChanSB->setValue( p.trgTTL.chan );
    trigTTLPanelUI->dBitSB->setValue( p.trgTTL.bit );
    trigTTLPanelUI->inarowSB->setValue( p.trgTTL.inarow );
    trigTTLPanelUI->NSB->setValue( p.trgTTL.nH );
    trigTTLPanelUI->analogRadio->setChecked( p.trgTTL.isAnalog );
    trigTTLPanelUI->digRadio->setChecked( !p.trgTTL.isAnalog );
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
    trigTTLAnalogChanged();
    trigTTLModeChanged( p.trgTTL.mode );
    trigTTLNInfClicked( p.trgTTL.isNInf );
    trigSpkNInfClicked( p.trgSpike.isNInf );
}


void ConfigCtl::setupMapTab( const DAQ::Params &p )
{
    QString s;

// Imec shankMap

    s = p.sns.imChans.shankMapFile;

    if( s.contains( "*" ) )
        s.clear();

    if( s.isEmpty() )
        mapTabUI->imShkMapLE->setText( DEF_SKMP_LE );
    else
        mapTabUI->imShkMapLE->setText( s );

    mapTabUI->imShkMapLE->setEnabled( imecOK );
    mapTabUI->imShkMapBut->setEnabled( imecOK );

// Nidq shankMap

    s = p.sns.niChans.shankMapFile;

    if( s.contains( "*" ) )
        s.clear();

    if( s.isEmpty() )
        mapTabUI->niShkMapLE->setText( DEF_SKMP_LE );
    else
        mapTabUI->niShkMapLE->setText( s );

    mapTabUI->niShkMapLE->setEnabled( nidqOK );
    mapTabUI->niShkMapBut->setEnabled( nidqOK );

// Imec chanMap

    s = p.sns.imChans.chanMapFile;

    if( s.contains( "*" ) )
        s.clear();

    if( s.isEmpty() )
        mapTabUI->imChnMapLE->setText( DEF_CHMP_LE );
    else
        mapTabUI->imChnMapLE->setText( s );

    mapTabUI->imChnMapLE->setEnabled( imecOK );
    mapTabUI->imChnMapBut->setEnabled( imecOK );

// Nidq chanMap

    s = p.sns.niChans.chanMapFile;

    if( s.contains( "*" ) )
        s.clear();

    if( s.isEmpty() )
        mapTabUI->niChnMapLE->setText( DEF_CHMP_LE );
    else
        mapTabUI->niChnMapLE->setText( s );

    mapTabUI->niChnMapLE->setEnabled( nidqOK );
    mapTabUI->niChnMapBut->setEnabled( nidqOK );

// --------------------
// Observe dependencies
// --------------------
}


void ConfigCtl::setupSnsTab( const DAQ::Params &p )
{
// Imec

    if( imVers.opt == 4 ) {
        snsTabUI->rngLbl->setText(
        "IM ranges: AP[0:275], LF[276:551], SY[552]" );
    }

    snsTabUI->imSaveChansLE->setText( p.sns.imChans.uiSaveChanStr );
    snsTabUI->imSaveChansLE->setEnabled( imecOK );
    snsTabUI->pairChk->setChecked( p.sns.pairChk );
    snsTabUI->pairChk->setEnabled( imecOK );

// Nidq

    snsTabUI->niSaveChansLE->setText( p.sns.niChans.uiSaveChanStr );
    snsTabUI->niSaveChansLE->setEnabled( nidqOK );

// Common

    snsTabUI->notesTE->setPlainText( p.sns.notes );
    snsTabUI->dataDirLbl->setText( mainApp()->dataDir() );
    snsTabUI->runNameLE->setText( p.sns.runName );

    snsTabUI->diskSB->setValue( p.sns.reqMins );

// --------------------
// Observe dependencies
// --------------------
}


void ConfigCtl::setupNiVRangeCB()
{
    QString         devStr  = devNames[CURDEV1];
    QComboBox       *CB     = niTabUI->aiRangeCB;
    QList<VRange>   rngL    = CniCfg::aiDevRanges.values( devStr );
    double          targV   = 2,
                    selV    = 5000;
    int             nL      = rngL.size(),
                    sel     = 0;

// What to use as target max voltage
// ---------------------------------
// If rangeCB is non-empty, that is, if user has ever seen it before,
// then we will reselect the current choice. Otherwise, we'll try to
// select last saved.

    if( CB->count() ) {

        QRegExp re("\\[.*, (.*)\\]");

        if( CB->currentText().contains( re ) )
            targV = re.cap(1).toDouble();
    }
    else
        targV = acceptedParams.ni.range.rmax;

    CB->clear();

// As we create entries, seek the smallest maxV >= targV,
// making that the selection. On failure select first.

    for( int i = 0; i < nL; ++i ) {

        const VRange    &r  = rngL[i];
        QString         s   = QString("[%1, %2]")
                                .arg( r.rmin )
                                .arg( r.rmax );

        if( r.rmax < selV && r.rmax >= targV ) {
            selV    = r.rmax;
            sel     = i;
        }

        CB->insertItem( i, s );
    }

    CB->setCurrentIndex( sel );
}


QString ConfigCtl::uiMNStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->mn2LE->text().trimmed() : "");
}


QString ConfigCtl::uiMAStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->ma2LE->text().trimmed() : "");
}


QString ConfigCtl::uiXAStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->xa2LE->text().trimmed() : "");
}


QString ConfigCtl::uiXDStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->xd2LE->text().trimmed() : "");
}


bool ConfigCtl::isMuxingFromDlg() const
{
    return  !niTabUI->mn1LE->text().trimmed().isEmpty()
            || !niTabUI->ma1LE->text().trimmed().isEmpty()
            || (niTabUI->dev2GB->isChecked()
                && (!niTabUI->mn2LE->text().trimmed().isEmpty()
                    || !niTabUI->ma2LE->text().trimmed().isEmpty())
                );
}


bool ConfigCtl::niChannelsFromDialog( CniCfg &ni ) const
{
    QVector<uint>   vcMN1, vcMA1, vcXA1, vcXD1,
                    vcMN2, vcMA2, vcXA2, vcXD2;

    if( !Subset::rngStr2Vec( vcMN1, niTabUI->mn1LE->text().trimmed() )
        || !Subset::rngStr2Vec( vcMA1, niTabUI->ma1LE->text().trimmed() )
        || !Subset::rngStr2Vec( vcXA1, niTabUI->xa1LE->text().trimmed() )
        || !Subset::rngStr2Vec( vcXD1, niTabUI->xd1LE->text().trimmed() )
        || !Subset::rngStr2Vec( vcMN2, uiMNStr2FromDlg() )
        || !Subset::rngStr2Vec( vcMA2, uiMAStr2FromDlg() )
        || !Subset::rngStr2Vec( vcXA2, uiXAStr2FromDlg() )
        || !Subset::rngStr2Vec( vcXD2, uiXDStr2FromDlg() ) ) {

        QMessageBox::critical(
            cfgDlg,
            "ChanMap Parameter Error",
            "Bad format in one or more NI-DAQ channel strings." );
        return false;
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

    return true;
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
    QString         &uiStr2Err ) const
{
// ----
// IMEC
// ----

    if( doingImec() ) {

        q.im.srate      = syncTabUI->imRateSB->value();
        q.im.hpFltIdx   = imTabUI->hpCB->currentIndex();
        q.im.softStart  = imTabUI->trigCB->currentIndex();
        q.im.imroFile   = imTabUI->imroLE->text().trimmed();
        q.im.stdbyStr   = imTabUI->stdbyLE->text().trimmed();
        q.im.doGainCor  = imTabUI->gainCorChk->isChecked();
        q.im.noLEDs     = imTabUI->noLEDChk->isChecked();
        q.im.enabled    = true;

        if( q.im.hpFltIdx == 2 )
            q.im.hpFltIdx = 3;
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

        if( !Subset::rngStr2Vec( vcMN1, niTabUI->mn1LE->text().trimmed() ) )
            uiStr1Err = "MN-chans";

        if( !Subset::rngStr2Vec( vcMA1, niTabUI->ma1LE->text().trimmed() ) ) {
            uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
            uiStr1Err += "MA-chans";
        }

        if( !Subset::rngStr2Vec( vcXA1, niTabUI->xa1LE->text().trimmed() ) ) {
            uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
            uiStr1Err += "XA-chans";
        }

        if( !Subset::rngStr2Vec( vcXD1, niTabUI->xd1LE->text().trimmed() ) ) {
            uiStr1Err += (uiStr1Err.isEmpty() ? "" : ", ");
            uiStr1Err += "XD-chans";
        }

        if( !Subset::rngStr2Vec( vcMN2, niTabUI->mn2LE->text().trimmed() ) )
            uiStr2Err = "MN-chans";

        if( !Subset::rngStr2Vec( vcMA2, niTabUI->ma2LE->text().trimmed() ) ) {
            uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
            uiStr2Err += "MA-chans";
        }

        if( !Subset::rngStr2Vec( vcXA2, niTabUI->xa2LE->text().trimmed() ) ) {
            uiStr2Err += (uiStr2Err.isEmpty() ? "" : ", ");
            uiStr2Err += "XA-chans";
        }

        if( !Subset::rngStr2Vec( vcXD2, niTabUI->xd2LE->text().trimmed() ) ) {
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

        q.ni.clockStr1      = niTabUI->clk1CB->currentText();
        q.ni.clockStr2      = niTabUI->clk2CB->currentText();
        q.ni.srateSet       = niTabUI->srateSB->value();
        q.ni.srate          = syncTabUI->niRateSB->value();
        q.ni.mnGain         = niTabUI->mnGainSB->value();
        q.ni.maGain         = niTabUI->maGainSB->value();
        q.ni.uiMNStr1       = Subset::vec2RngStr( vcMN1 );
        q.ni.uiMAStr1       = Subset::vec2RngStr( vcMA1 );
        q.ni.uiXAStr1       = Subset::vec2RngStr( vcXA1 );
        q.ni.uiXDStr1       = Subset::vec2RngStr( vcXD1 );
        q.ni.setUIMNStr2( Subset::vec2RngStr( vcMN2 ) );
        q.ni.setUIMAStr2( Subset::vec2RngStr( vcMA2 ) );
        q.ni.setUIXAStr2( Subset::vec2RngStr( vcXA2 ) );
        q.ni.setUIXDStr2( Subset::vec2RngStr( vcXD2 ) );
        q.ni.startLine      = niTabUI->startCB->currentText();
        q.ni.muxFactor      = niTabUI->muxFactorSB->value();

        q.ni.termCfg =
        q.ni.stringToTermConfig( niTabUI->aiTerminationCB->currentText() );

        q.ni.isDualDevMode  = niTabUI->dev2GB->isChecked();
        q.ni.startEnable    = niTabUI->startEnabChk->isChecked();
        q.ni.enabled        = true;
    }
    else {
        q.ni                = acceptedParams.ni;
        q.ni.enabled        = false;
    }

    q.ni.deriveChanCounts();

// ----------
// SyncParams
// ----------

    q.sync.sourceIdx    = (DAQ::SyncSource)syncTabUI->sourceCB->currentIndex();
    q.sync.sourcePeriod = syncTabUI->sourceSB->value();

    q.sync.imChanType   = syncTabUI->imChanCB->currentIndex();
    q.sync.imChan       = syncTabUI->imChanSB->value();
// MS: IM analog sync not yet implemented
    q.sync.imThresh     = acceptedParams.sync.imThresh;

    q.sync.niChanType   = syncTabUI->niChanCB->currentIndex();
    q.sync.niChan       = syncTabUI->niChanSB->value();
    q.sync.niThresh     = syncTabUI->niThreshSB->value();

    q.sync.isCalRun     = syncTabUI->calChk->isChecked();
    q.sync.calMins      = syncTabUI->minSB->value();

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
    q.trgTTL.mode           = trigTTLPanelUI->modeCB->currentIndex();
    q.trgTTL.chan           = trigTTLPanelUI->aChanSB->value();
    q.trgTTL.bit            = trigTTLPanelUI->dBitSB->value();
    q.trgTTL.inarow         = trigTTLPanelUI->inarowSB->value();
    q.trgTTL.nH             = trigTTLPanelUI->NSB->value();
    q.trgTTL.isAnalog       = trigTTLPanelUI->analogRadio->isChecked();
    q.trgTTL.isNInf         = trigTTLPanelUI->NInfChk->isChecked();

    if( q.trgTTL.isAnalog )
        q.trgTTL.stream = trigTTLPanelUI->aStreamCB->currentText();
    else
        q.trgTTL.stream = trigTTLPanelUI->dStreamCB->currentText();

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

// ----
// Maps
// ----

    q.sns.imChans.shankMapFile  = mapTabUI->imShkMapLE->text().trimmed();
    q.sns.niChans.shankMapFile  = mapTabUI->niShkMapLE->text().trimmed();
    q.sns.imChans.chanMapFile   = mapTabUI->imChnMapLE->text().trimmed();
    q.sns.niChans.chanMapFile   = mapTabUI->niChnMapLE->text().trimmed();

// --------
// SeeNSave
// --------

    q.sns.pairChk               = snsTabUI->pairChk->isChecked();
    q.sns.imChans.uiSaveChanStr = snsTabUI->imSaveChansLE->text().trimmed();
    q.sns.niChans.uiSaveChanStr = snsTabUI->niSaveChansLE->text().trimmed();
    q.sns.notes                 = snsTabUI->notesTE->toPlainText().trimmed();
    q.sns.runName               = snsTabUI->runNameLE->text().trimmed();
    q.sns.reqMins               = snsTabUI->diskSB->value();
}


bool ConfigCtl::validDevTab( QString &err, DAQ::Params &q ) const
{
    if( !q.im.enabled && !q.ni.enabled ) {

        err =
        "Enable/Detect at least one device group on the Devices tab.";
        return false;
    }

    return true;
}


bool ConfigCtl::validImROTbl( QString &err, DAQ::Params &q ) const
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


bool ConfigCtl::validImStdbyBits( QString &err, DAQ::Params &q ) const
{
    if( !doingImec() || imVers.opt != 3 )
        return true;

    return q.im.deriveStdbyBits(
            err, q.im.imCumTypCnt[CimCfg::imSumAP] );
}


bool ConfigCtl::validNiDevices( QString &err, DAQ::Params &q ) const
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
            QString("Device disconnected or off '%1'.")
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
            QString("Device disconnected or off '%1'.")
            .arg( q.ni.dev2 );
            return false;
        }
    }

    if( !q.ni.dev2.compare( q.ni.dev1, Qt::CaseInsensitive ) ) {

        err =
        "Primary and secondary NI devices cannot be same"
        " if dual-device mode selected.";
        return false;
    }

// BK: For now dualDev requires same board model because:
// - We offer only shared range choices.
// - We gauge samples waiting on dev2 by result for dev1,
//      so to keep acquisition in lock step we want
//      identical performance characteristics.

    if( CniCfg::getProductName( q.ni.dev2 )
            .compare(
                CniCfg::getProductName( q.ni.dev1 ),
                Qt::CaseInsensitive ) ) {

        err =
        "Primary and secondary NI devices must be same model"
        " for dual-device operation.";
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
    QString         &uiStr2Err ) const
{
    if( !doingNidq() )
        return true;

    uint    maxAI,
            maxDI;
    int     nAI,
            nDI,
            whisperStartLine = -1;
    bool    isMux = isMuxingFromDlg();

// ----
// Dev1
// ----

// Previous parsing error?

    if( !uiStr1Err.isEmpty() ) {
        err =
        QString(
        "Error in fields [%1].\n"
        "Valid primary NI device channel strings look like"
        " \"0,1,2,3 or 0:3,5,6.\"")
        .arg( uiStr1Err );
        return false;
    }

// No channels?

    nAI = vcMN1.size() + vcMA1.size() + vcXA1.size();
    nDI = vcXD1.size();

    if( !(nAI + nDI) ) {
        err = "Need at least 1 channel in primary NI device channel set.";
        return false;
    }

// Illegal channels?

    maxAI = CniCfg::aiDevChanCount[q.ni.dev1] - 1;

    // For the simultaneous sampling devices {PCI-6133, USB-6366}
    // Only the 8 port0 lines can do clocked ("buffered") input.
    // The count below is all P0 and PFI lines: not informative.
    // maxDI = CniCfg::diDevLineCount[q.ni.dev1] - 1;
    maxDI = 7;

    if( (vcMN1.count() && vcMN1.last() > maxAI)
        || (vcMA1.count() && vcMA1.last() > maxAI)
        || (vcXA1.count() && vcXA1.last() > maxAI) ) {

        err =
        QString("Primary NI device AI channel values must not exceed [%1].")
        .arg( maxAI );
        return false;
    }

    if( vcXD1.count() && vcXD1.last() > maxDI ) {

        err =
        QString("Primary NI device DI line values must not exceed [%1].")
        .arg( maxDI );
        return false;
    }

// Ensure analog channels ordered MN < MA < XA

    if( vcMN1.count() ) {

        if( (vcMA1.count() && vcMA1.first() <= vcMN1.last())
            || (vcXA1.count() && vcXA1.first() <= vcMN1.last()) ) {

            err =
                "Primary NI device analog channels must be"
                " ordered so that MN < MA < XA.";
            return false;
        }
    }

    if( vcMA1.count() ) {

        if( vcXA1.count() && vcXA1.first() <= vcMA1.last() ) {

            err =
                "Primary NI device analog channels must be"
                " ordered so that MN < MA < XA.";
            return false;
        }
    }

// Too many ai channels?

    if( isMux && !CniCfg::supportsAISimultaneousSampling( q.ni.dev1 ) ) {

        err =
        QString(
        "Device [%1] does not support simultaneous sampling which is"
        " needed for MN and MA input channels.")
        .arg( q.ni.dev1 );
        return false;
    }

    double  rMax = CniCfg::maxSampleRate( q.ni.dev1, nAI );

    if( q.ni.srateSet > rMax ) {

        if( nAI ) {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 1 maximum (%2)"
            " for (%3) analog channels.")
            .arg( q.ni.srateSet )
            .arg( rMax )
            .arg( nAI );
        }
        else {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 1 maximum (%2)"
            " for digital channels.")
            .arg( q.ni.srateSet )
            .arg( rMax );
        }

        return false;
    }

// Wrong termination type?

    if( CniCfg::wrongTermConfig(
            err, q.ni.dev1,
            vcMN1 + vcMA1 + vcXA1,
            q.ni.termCfg ) ) {

        return false;
    }

// Start line can not be digital input

    if( q.ni.startEnable && vcXD1.count() ) {

        QString dev;
        CniCfg::parseDIStr( dev, whisperStartLine, q.ni.startLine );

        if( dev == q.ni.dev1 && vcXD1.contains( whisperStartLine ) ) {

            err =
            "Start output line cannot be used as a digital input"
            " line on primary NI device.";
            return false;
        }
    }

// ----
// Dev2
// ----

    if( !q.ni.isDualDevMode )
        return true;

// Previous parsing error?

    if( !uiStr2Err.isEmpty() ) {
        err =
        QString(
        "Error in fields [%1].\n"
        "Valid secondary NI device channel strings look like"
        " \"0,1,2,3 or 0:3,5,6.\"")
        .arg( uiStr2Err );
        return false;
    }

// No channels?

    nAI = vcMN2.size() + vcMA2.size() + vcXA2.size();
    nDI = vcXD2.size();

    if( !(nAI + nDI) ) {
        err = "Need at least 1 channel in secondary NI device channel set.";
        return false;
    }

// Illegal channels?

    maxAI = CniCfg::aiDevChanCount[q.ni.dev2] - 1;
    maxDI = CniCfg::diDevLineCount[q.ni.dev2] - 1;

    if( (vcMN2.count() && vcMN2.last() > maxAI)
        || (vcMA2.count() && vcMA2.last() > maxAI)
        || (vcXA2.count() && vcXA2.last() > maxAI) ) {

        err =
        QString("Secondary NI device AI chan values must not exceed [%1].")
        .arg( maxAI );
        return false;
    }

    if( vcXD2.count() && vcXD2.last() > maxDI ) {

        err =
        QString("Secondary NI device DI line values must not exceed [%1].")
        .arg( maxDI );
        return false;
    }

// Ensure analog channels ordered MN < MA < XA

    if( vcMN2.count() ) {

        if( (vcMA2.count() && vcMA2.first() <= vcMN2.last())
            || (vcXA2.count() && vcXA2.first() <= vcMN2.last()) ) {

            err =
                "Secondary NI device analog channels must be"
                " ordered so that MN < MA < XA.";
            return false;
        }
    }

    if( vcMA2.count() ) {

        if( vcXA2.count() && vcXA2.first() <= vcMA2.last() ) {

            err =
                "Secondary NI device analog channels must be"
                " ordered so that MN < MA < XA.";
            return false;
        }
    }

// Too many ai channels?

    if( isMux && !CniCfg::supportsAISimultaneousSampling( q.ni.dev2 ) ) {

        err =
        QString(
        "Device [%1] does not support simultaneous sampling which is"
        " needed for MN and MA input channels.")
        .arg( q.ni.dev2 );
        return false;
    }

    rMax = CniCfg::maxSampleRate( q.ni.dev2, nAI );

    if( q.ni.srateSet > rMax ) {

        if( nAI ) {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 2 maximum (%2)"
            " for (%3) analog channels.")
            .arg( q.ni.srateSet )
            .arg( rMax )
            .arg( nAI );
        }
        else {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 2 maximum (%2)"
            " for digital channels.")
            .arg( q.ni.srateSet )
            .arg( rMax );
        }

        return false;
    }

// Wrong termination type?

    if( CniCfg::wrongTermConfig(
            err, q.ni.dev2,
            vcMN2 + vcMA2 + vcXA2,
            q.ni.termCfg ) ) {

        return false;
    }

// Start line can not be digital input

    if( whisperStartLine >= 0 && vcXD2.count() ) {

        if( vcXD2.contains( whisperStartLine ) ) {

            err =
            "Common start output line cannot be used as digital"
            " input on either device.";
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validImSaveBits( QString &err, DAQ::Params &q ) const
{
    if( !doingImec() )
        return true;

    int     nc = q.im.imCumTypCnt[CimCfg::imSumAll];
    bool    ok;

    ok = q.sns.imChans.deriveSaveBits( err, nc );

    if( ok ) {

        QBitArray   &B = q.sns.imChans.saveBits;

        // Pair LF to AP

        if( q.sns.pairChk ) {

            int     nAP  = q.im.imCumTypCnt[CimCfg::imTypeAP],
                    nNu  = 2 * nAP;
            bool    isAP = false;

            for( int b = 0; b < nAP; ++b ) {
                if( (isAP = B.testBit( b )) )
                    break;
            }

            if( isAP ) {

                for( int b = nAP; b < nNu; ++b )
                    B.clearBit( b );

                for( int b = 0; b < nAP; ++b ) {

                    if( B.testBit( b ) )
                        B.setBit( nAP + b );
                }
            }
        }

        if( B.count( true ) == nc )
            q.sns.imChans.uiSaveChanStr = "all";
        else
            q.sns.imChans.uiSaveChanStr = Subset::bits2RngStr( B );

        snsTabUI->imSaveChansLE->setText( q.sns.imChans.uiSaveChanStr );
    }

    return ok;
}


bool ConfigCtl::validNiSaveBits( QString &err, DAQ::Params &q ) const
{
    if( !doingNidq() )
        return true;

    return q.sns.niChans.deriveSaveBits(
            err, q.ni.niCumTypCnt[CniCfg::niSumAll] );
}


bool  ConfigCtl::validSyncTab( QString &err, DAQ::Params &q ) const
{
    if( q.sync.sourceIdx == DAQ::eSyncSourceNone )
        return true;

    if( q.sync.sourceIdx == DAQ::eSyncSourceNI ) {

#ifndef HAVE_NIDAQmx
        err =
        "NI sync source unavailable: NI simulated.";
        return false;
#endif

        if( !doingNidq() ) {

            err =
            "NI sync source selected but Nidq not enabled.";
            return false;
        }
    }

    if( doingImec() ) {

        if( q.sync.imChanType == 1 ) {

            // Tests for analog channel and threshold

            err =
            "IM analog sync channels not supported at this time.";
            return false;
        }
        else {

            // Tests for digital bit

            if( q.sync.imChan >= 16 ) {

                err =
                "IM sync bits must be in range [0..15].";
                return false;
            }

            int dword = q.im.imCumTypCnt[CimCfg::imSumNeural];

            if( !q.sns.imChans.saveBits.testBit( dword ) ) {

                err =
                QString(
                "IM sync word (chan %1) not included in saved channels.")
                .arg( dword );
                return false;
            }
        }
    }

    if( doingNidq() ) {

        if( q.sync.niChanType == 1 ) {

            // Tests for analog channel and threshold

            int chMin = q.ni.niCumTypCnt[CniCfg::niSumNeural],
                chMax = q.ni.niCumTypCnt[CniCfg::niSumAnalog];

            if( chMin >= chMax ) {

                err =
                "NI sync channel invalid: No aux channels configured.";
                return false;
            }

            if( q.sync.niChan < chMin || q.sync.niChan >= chMax ) {

                err =
                QString(
                "NI sync chan [%1] must be in range [%2..%3].")
                .arg( q.sync.niChan )
                .arg( chMin )
                .arg( chMax - 1 );
                return false;
            }

            if( !q.sns.niChans.saveBits.testBit( q.sync.niChan ) ) {

                err =
                QString(
                "NI sync chan [%1] not included in saved channels.")
                .arg( q.sync.niChan );
                return false;
            }

            double  Tmin = q.ni.int16ToV( -32768, q.sync.niChan ),
                    Tmax = q.ni.int16ToV(  32767, q.sync.niChan );

            if( q.sync.niThresh < Tmin || q.sync.niThresh > Tmax ) {

                err =
                QString(
                "NI sync threshold must be in range (%1..%2) V.")
                .arg( Tmin ).arg( Tmax );
                return false;
            }
        }
        else {

            // Tests for digital bit

            QVector<uint>   vc1, vc2;

            Subset::rngStr2Vec( vc1, q.ni.uiXDStr1 );
            Subset::rngStr2Vec( vc2, q.ni.uiXDStr2() );

            if( vc1.size() + vc2.size() == 0 ) {

                err =
                "Sync tab: No NI digital lines have been specified.";
                return false;
            }

            if( q.sync.niChan < 8 && !vc1.contains( q.sync.niChan ) ) {

                err =
                QString(
                "NI sync bit [%1] not in primary NI device"
                " XD chan list [%2].")
                .arg( q.sync.niChan )
                .arg( q.ni.uiXDStr1 );
                return false;
            }

            if( q.sync.niChan >= 8
                && !vc2.contains( q.sync.niChan - 8 ) ) {

                err =
                QString(
                "NI sync bit [%1] (secondary bit %2)"
                " not in secondary NI device XD chan list [%3].")
                .arg( q.sync.niChan )
                .arg( q.sync.niChan - 8 )
                .arg( q.ni.uiXDStr2() );
                return false;
            }

            int dword = q.ni.niCumTypCnt[CniCfg::niSumAnalog]
                            + q.sync.niChan/16;

            if( !q.sns.niChans.saveBits.testBit( dword ) ) {

                err =
                QString(
                "NI sync word (chan %1) not included in saved channels.")
                .arg( dword );
                return false;
            }
        }
    }

    return true;
}


bool ConfigCtl::validImTriggering( QString &err, DAQ::Params &q ) const
{
    if( !doingImec() ) {

        err =
        QString(
        "Imec stream selected for '%1' trigger but not enabled.")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) );
        return false;
    }

    if( q.mode.mTrig == DAQ::eTrigSpike
        || (q.mode.mTrig == DAQ::eTrigTTL && q.trgTTL.isAnalog) ) {

        // Tests for analog channel and threshold

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
    }
    else if( q.mode.mTrig == DAQ::eTrigTTL && !q.trgTTL.isAnalog ) {

        // Tests for digital bit

        if( q.trgTTL.bit >= 16 ) {

            err =
            "Imec TTL trigger bits must be in range [0..15].";
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validNiTriggering( QString &err, DAQ::Params &q ) const
{
    if( !doingNidq() ) {

        err =
        QString(
        "Nidq stream selected for '%1' trigger but not enabled.")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) );
        return false;
    }

    if( q.mode.mTrig == DAQ::eTrigSpike
        || (q.mode.mTrig == DAQ::eTrigTTL && q.trgTTL.isAnalog) ) {

        // Tests for analog channel and threshold

        int trgChan = q.trigChan(),
            nLegal  = q.ni.niCumTypCnt[CniCfg::niSumAnalog];

        if( nLegal <= 0 ) {

            err =
            QString(
            "Invalid '%1' trigger channel [%2];"
            " No NI analog channels configured.")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( trgChan );
            return false;
        }

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
    }
    else if( q.mode.mTrig == DAQ::eTrigTTL && !q.trgTTL.isAnalog ) {

        // Tests for digital bit

        QVector<uint>   vc1, vc2;

        Subset::rngStr2Vec( vc1, q.ni.uiXDStr1 );
        Subset::rngStr2Vec( vc2, q.ni.uiXDStr2() );

        if( vc1.size() + vc2.size() == 0 ) {

            err =
            "Trigger Tab: No NI digital lines have been specified.";
            return false;
        }

        if( q.trgTTL.bit < 8 && !vc1.contains( q.trgTTL.bit ) ) {

            err =
            QString(
            "Nidq TTL trigger bit [%1] not in primary NI device"
            " XD list [%2].")
            .arg( q.trgTTL.bit )
            .arg( q.ni.uiXDStr1 );
            return false;
        }

        if( q.trgTTL.bit >= 8
            && !vc2.contains( q.trgTTL.bit - 8 ) ) {

            err =
            QString(
            "Nidq TTL trigger bit [%1] (secondary bit %2)"
            " not in secondary NI device XD chan list [%3].")
            .arg( q.trgTTL.bit )
            .arg( q.trgTTL.bit - 8 )
            .arg( q.ni.uiXDStr2() );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validTrgPeriEvent( QString &err, DAQ::Params &q ) const
{
    if( q.mode.mTrig == DAQ::eTrigSpike
        || q.mode.mTrig == DAQ::eTrigTTL ) {

        // Test for perievent window

        double stream, trgMrg;

        if( q.mode.mTrig == DAQ::eTrigSpike )
            trgMrg = q.trgSpike.periEvtSecs;
        else
            trgMrg = q.trgTTL.marginSecs;

        stream = 0.80 * mainApp()->getRun()->streamSpanMax( q, false );

        if( trgMrg >= stream ) {

            err =
            QString(
            "The trigger added context secs [%1] must be shorter than"
            " [%2] which is 80% of the expected stream length.")
            .arg( trgMrg )
            .arg( stream );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validImShankMap( QString &err, DAQ::Params &q ) const
{
// Pretties ini file, even if not using device
    if( q.sns.imChans.shankMapFile.contains( "*" ) )
        q.sns.imChans.shankMapFile.clear();

    if( !doingImec() )
        return true;

    ShankMap    &M = q.sns.imChans.shankMap;

    if( q.sns.imChans.shankMapFile.isEmpty() ) {

        M.fillDefaultIm( q.im.roTbl );

        // Save in case stdby channels changed
        q.sns.imChans.shankMap_orig = M;

        if( imVers.opt == 3 )
            M.andOutImStdby( q.im.stdbyBits );

        return true;
    }

    QString msg;

    if( !M.loadFile( msg, q.sns.imChans.shankMapFile ) ) {

        err = QString("ShankMap: %1.").arg( msg );
        return false;
    }

    int N;

    N = q.im.roTbl.nElec();

    if( M.nSites() != N ) {

        err = QString(
                "Imec ShankMap header mismatch--\n\n"
                "  - Cur config: %1 electrodes\n"
                "  - Named file: %2 electrodes.")
                .arg( N ).arg( M.nSites() );
        return false;
    }

    N = q.im.roTbl.nChan();

    if( M.e.size() != N ) {

        err = QString(
                "Imec ShankMap entry mismatch--\n\n"
                "  - Cur config: %1 channels\n"
                "  - Named file: %2 channels.")
                .arg( N ).arg( M.e.size() );
        return false;
    }

    M.andOutImRefs( q.im.roTbl );

    // Save in case stdby channels changed
    q.sns.imChans.shankMap_orig = M;

    if( imVers.opt == 3 )
        M.andOutImStdby( q.im.stdbyBits );

    return true;
}


bool ConfigCtl::validNiShankMap( QString &err, DAQ::Params &q ) const
{
// Pretties ini file, even if not using device
    if( q.sns.niChans.shankMapFile.contains( "*" ) )
        q.sns.niChans.shankMapFile.clear();

    if( !doingNidq() )
        return true;

    ShankMap    &M      = q.sns.niChans.shankMap;
    int         nChan   = q.ni.niCumTypCnt[CniCfg::niTypeMN];

    if( q.sns.niChans.shankMapFile.isEmpty() ) {

        // Single shank, two columns

        M.fillDefaultNi( 1, 2, nChan/2, nChan );
        return true;
    }

    QString msg;

    if( !M.loadFile( msg, q.sns.niChans.shankMapFile ) ) {

        err = QString("ShankMap: %1.").arg( msg );
        return false;
    }

    if( M.nSites() < nChan ) {

        err = QString(
                "Nidq ShankMap has too few probe sites--\n\n"
                "  - Cur config: %1 channels\n"
                "  - Named file: %2 electrodes.")
                .arg( nChan ).arg( M.nSites() );
        return false;
    }

    if( M.e.size() != nChan ) {

        err = QString(
                "Nidq ShankMap entry mismatch--\n\n"
                "  - Cur config: %1 channels\n"
                "  - Named file: %2 channels.")
                .arg( nChan ).arg( M.e.size() );
        return false;
    }

    return true;
}


bool ConfigCtl::validImChanMap( QString &err, DAQ::Params &q ) const
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


bool ConfigCtl::validNiChanMap( QString &err, DAQ::Params &q ) const
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


bool ConfigCtl::validDataDir( QString &err ) const
{
    if( !QDir( mainApp()->dataDir() ).exists() ) {

        err =
        QString("Data directory does not exist [%1].")
        .arg( mainApp()->dataDir() );
        return false;
    }

    return true;
}


bool ConfigCtl::validDiskAvail( QString &err, DAQ::Params &q ) const
{
    if( q.sns.reqMins <= 0 )
        return true;

    double  BPS     = 0;
    quint64 avail   = availableDiskSpace();
    int     mins;

    if( doingImec() ) {
        BPS += q.apSaveChanCount() * q.im.srate * 2;
        BPS += q.lfSaveChanCount() * q.im.srate/12 * 2;
    }

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


bool ConfigCtl::shankParamsToQ( QString &err, DAQ::Params &q ) const
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

    if( !validImROTbl( err, q ) )
        return false;

    if( !validImStdbyBits( err, q ) )
        return false;

    if( !validNiDevices( err, q )
        || !validNiChannels( err, q,
                vcMN1, vcMA1, vcXA1, vcXD1,
                vcMN2, vcMA2, vcXA2, vcXD2,
                uiStr1Err, uiStr2Err ) ) {

        return false;
    }

    if( !validImShankMap( err, q ) )
        return false;

    if( !validNiShankMap( err, q ) )
        return false;

    return true;
}


bool ConfigCtl::diskParamsToQ( QString &err, DAQ::Params &q ) const
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

    if( !validImStdbyBits( err, q ) )
        return false;

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

    if( !validSyncTab( err, q ) )
        return false;

    { // limited scope of 'stream'

        QString stream = q.trigStream();

        stream.truncate( 4 );

        if( stream == "imec" && !validImTriggering( err, q ) )
            return false;

        if( stream == "nidq" && !validNiTriggering( err, q ) )
            return false;

        if( !validTrgPeriEvent( err, q ) )
            return false;
    }

    if( !validImShankMap( err, q ) )
        return false;

    if( !validNiShankMap( err, q ) )
        return false;

    if( !validImChanMap( err, q ) )
        return false;

    if( !validNiChanMap( err, q ) )
        return false;

    if( !validDataDir( err ) )
        return false;

    if( !validDiskAvail( err, q ) )
        return false;

    if( !validRunName( err, q.sns.runName, cfgDlg, isGUI ) )
        return false;

// --------------------------
// Warn about ColorTTL issues
// --------------------------

    ColorTTLCtl TTLCC( cfgDlg, q );
    QString     ccErr;

    if( !TTLCC.valid( ccErr ) ) {

        QMessageBox::warning( cfgDlg,
            "Conflicts Detected With ColorTTL Events",
            QString(
            "Issues detected:\n%1\n\n"
            "Fix either the ColorTTL or the run settings...")
            .arg( ccErr ) );

        TTLCC.showDialog();

        int yesNo = QMessageBox::question(
            cfgDlg,
            "Run (or Verify/Save) Now?",
            "[Yes] = settings are ready to go,\n"
            "[No]  = let me edit run settings.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes );

        if( yesNo != QMessageBox::Yes )
            return false;
    }

// -------------
// Accept params
// -------------

    validated = true;
    setParams( q, true );

    return true;
}


