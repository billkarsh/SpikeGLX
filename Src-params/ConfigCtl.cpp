
#include "ui_ConfigureDialog.h"

#include "Pixmaps/Icon-Config.xpm"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "HelpButDialog.h"
#include "AOCtl.h"
#include "ColorTTLCtl.h"
#include "Subset.h"
#include "SignalBlocker.h"

#include <QMessageBox>
#include <QComboBox>
#include <QDirIterator>


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
        devTab(0),
        imTab(0),
        obxTab(0),
        niTab(0),
        syncTab(0),
        gateTab(0),
        trigTab(0),
        snsTab(0),
        cfgDlg(0),
        usingIM(false), usingOB(false),
        usingNI(false), validated(false)
{
// -----------
// Main dialog
// -----------

    cfgDlg = new HelpButDialog( "UserManual" );
    cfgDlg->setWindowIcon( QIcon(QPixmap(Icon_Config_xpm)) );
    cfgDlg->setAttribute( Qt::WA_DeleteOnClose, false );

    cfgUI = new Ui::ConfigureDialog;
    cfgUI->setupUi( cfgDlg );
    ConnectUI( cfgUI->tabsW, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)) );
    ConnectUI( cfgUI->resetBut, SIGNAL(clicked()), this, SLOT(reset()) );
    ConnectUI( cfgUI->verifyBut, SIGNAL(clicked()), this, SLOT(verify()) );
    ConnectUI( cfgUI->helpBut, SIGNAL(clicked()), this, SLOT(tabHelp()) );
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

// ----
// Tabs
// ----

    devTab  = new Config_devtab( this, cfgUI->devTab );
    imTab   = new Config_imtab( this, cfgUI->imTab );
    obxTab  = new Config_obxtab( this, cfgUI->obxTab );
    niTab   = new Config_nitab( this, cfgUI->niTab );
    syncTab = new Config_synctab( cfgUI->syncTab );
    gateTab = new Config_gatetab( cfgUI->gateTab );
    trigTab = new Config_trigtab( cfgUI->trigTab );
    snsTab  = new Config_snstab( cfgUI->snsTab );

    cfgUI->tabsW->setCurrentIndex( 0 );
}


ConfigCtl::~ConfigCtl()
{
    if( snsTab ) {
        delete snsTab;
        snsTab = 0;
    }

    if( trigTab ) {
        delete trigTab;
        trigTab = 0;
    }

    if( gateTab ) {
        delete gateTab;
        gateTab = 0;
    }

    if( syncTab ) {
        delete syncTab;
        syncTab = 0;
    }

    if( niTab ) {
        delete niTab;
        niTab = 0;
    }

    if( obxTab ) {
        delete obxTab;
        obxTab = 0;
    }

    if( imTab ) {
        delete imTab;
        imTab = 0;
    }

    if( devTab ) {
        delete devTab;
        devTab = 0;
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
    reset();

// ----------
// Run dialog
// ----------

    int retCode = cfgDlg->exec();

    cfgDlg->close();

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


bool ConfigCtl::externSetsRunName(
    QString         &err,
    const QString   &name,
    QWidget         *parent )
{
    if( !validated )
        err = "Run parameters never validated.";
    else if( validRunName( err, acceptedParams, name, parent ) ) {

        acceptedParams.saveSettings();

        if( cfgDlg->isVisible() )
            snsTab->setRunName( name );

        return true;
    }

    return false;
}


// Note: This is called if the user R-clicks imec graphs,
// edits imro table and makes changes. Here we further
// test if any of the bank values have changed because
// that situation requires regeneration of the shankMap.
//
void ConfigCtl::graphSetsImroFile( const QString &file, int ip )
{
    DAQ::Params     &p = acceptedParams;
    CimCfg::PrbEach &E = p.im.prbj[ip];
    QString         err;

    IMROTbl *T_old = IMROTbl::alloc( E.roTbl->type );
    T_old->copyFrom( E.roTbl );

    E.imroFile = file;

    if( validImROTbl( err, E, ip ) ) {

        if( !E.roTbl->isConnectedSame( T_old ) ) {

            // Force default maps from imro
            E.sns.shankMapFile.clear();
            validImShankMap( err, E, ip );
            validImChanMap( err, E, ip );
        }

        imTab->updateProbe( E, ip );
        imTab->saveSettings();
    }
    else
        Error() << err;

    delete T_old;
}


void ConfigCtl::graphSetsStdbyStr( const QString &sdtbyStr, int ip )
{
    CimCfg::PrbEach &E = acceptedParams.im.prbj[ip];
    QString         err;

    E.stdbyStr = sdtbyStr;

    if( validImStdbyBits( err, E, ip ) ) {

        E.sns.shankMap = E.sns.shankMap_orig;
        E.sns.shankMap.andOutImStdby( E.stdbyBits );
        imTab->updateProbe( E, ip );
        imTab->saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsImChanMap( const QString &cmFile, int ip )
{
    CimCfg::PrbEach &E  = acceptedParams.im.prbj[ip];
    ChanMapIM       &M  = E.sns.chanMap;
    QString         msg,
                    err;

    if( cmFile.isEmpty() )
        M.setImroOrder( E.roTbl );
    else if( !M.loadFile( msg, cmFile ) )
        err = QString("IM ChanMap: %1.").arg( msg );
    else {

        const int   *type   = E.imCumTypCnt;
        ChanMapIM   D(
            type[CimCfg::imTypeAP],
            type[CimCfg::imTypeLF] - type[CimCfg::imTypeAP],
            type[CimCfg::imTypeSY] - type[CimCfg::imTypeLF] );

        if( !M.equalHdr( D ) ) {

            err = QString(
                    "IM ChanMap header mismatch--\n\n"
                    "  - Cur config: (%1 %2 %3)\n"
                    "  - Named file: (%4 %5 %6).")
                    .arg( D.AP ).arg( D.LF ).arg( D.SY )
                    .arg( M.AP ).arg( M.LF ).arg( M.SY );
        }
    }

    if( err.isEmpty() ) {

        E.sns.chanMapFile = cmFile;
        imTab->updateProbe( E, ip );
        imTab->saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsObChanMap( const QString &cmFile, int ip )
{
    CimCfg::ObxEach &E      = acceptedParams.im.obxj[ip];
    ChanMapOB       &M      = E.sns.chanMap;
    QString         msg,
                    err;
    const int       *type   = E.obCumTypCnt;

    ChanMapOB   D(
        type[CimCfg::obTypeXA],
        type[CimCfg::obTypeXD] - type[CimCfg::obTypeXA],
        type[CimCfg::obTypeSY] - type[CimCfg::obTypeXD] );

    if( cmFile.isEmpty() ) {

        M = D;
        M.fillDefault();
    }
    else if( !M.loadFile( msg, cmFile ) )
        err = QString("OBX ChanMap: %1.").arg( msg );
    else if( !M.equalHdr( D ) ) {

        err = QString(
                "OBX ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3)\n"
                "  - Named file: (%4 %5 %6).")
                .arg( D.XA ).arg( D.XD ).arg( D.SY )
                .arg( M.XA ).arg( M.XD ).arg( M.SY );
    }

    if( err.isEmpty() ) {

        E.sns.chanMapFile = cmFile;
        obxTab->updateObx( E, ip );
        obxTab->saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsNiChanMap( const QString &cmFile )
{
    DAQ::Params &p      = acceptedParams;
    ChanMapNI   &M      = p.ni.sns.chanMap;
    QString     msg,
                err;
    const int   *type   = p.ni.niCumTypCnt;
    int         nMux    = p.ni.muxFactor;

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
        err = QString("NI ChanMap: %1.").arg( msg );
    else if( !M.equalHdr( D ) ) {

        err = QString(
                "NI ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3 %4 %5)\n"
                "  - Named file: (%6 %7 %8 %9 %10).")
                .arg( D.MN ).arg( D.MA ).arg( D.C ).arg( D.XA ).arg( D.XD )
                .arg( M.MN ).arg( M.MA ).arg( M.C ).arg( M.XA ).arg( M.XD );
    }

    if( err.isEmpty() ) {

        p.ni.sns.chanMapFile = cmFile;
        p.saveSettings();
    }
    else
        Error() << err;
}


void ConfigCtl::graphSetsImSaveStr( const QString &saveStr, int ip )
{
    DAQ::Params     &p  = acceptedParams;
    CimCfg::PrbEach &E  = p.im.prbj[ip];
    QString         oldStr, err;

    oldStr = E.sns.uiSaveChanStr;
    E.sns.uiSaveChanStr = saveStr;

    if( validImSaveBits( err, p, ip ) ) {

        imTab->updateProbe( E, ip );
        imTab->saveSettings();
    }
    else {
        E.sns.uiSaveChanStr = oldStr;
        Error() << err;
    }
}


void ConfigCtl::graphSetsObSaveStr( const QString &saveStr, int ip )
{
    DAQ::Params     &p = acceptedParams;
    CimCfg::ObxEach &E = p.im.obxj[ip];
    QString         oldStr, err;

    oldStr = E.sns.uiSaveChanStr;
    E.sns.uiSaveChanStr = saveStr;

    if( validObSaveBits( err, p, ip ) ) {

        obxTab->updateObx( E, ip );
        obxTab->saveSettings();
    }
    else {
        E.sns.uiSaveChanStr = oldStr;
        Error() << err;
    }
}


void ConfigCtl::graphSetsNiSaveStr( const QString &saveStr )
{
    DAQ::Params &p = acceptedParams;
    QString     oldStr, err;

    oldStr = p.ni.sns.uiSaveChanStr;
    p.ni.sns.uiSaveChanStr = saveStr;

    if( validNiSaveBits( err, p ) )
        p.saveSettings();
    else {
        p.ni.sns.uiSaveChanStr = oldStr;
        Error() << err;
    }
}


void ConfigCtl::graphSetsImSaveBit( int chan, bool setOn, int ip )
{
    DAQ::Params     &p  = acceptedParams;
    CimCfg::PrbEach &E  = p.im.prbj[ip];

    if( chan >= 0 && chan < p.stream_nChans( jsIM, ip ) ) {

        E.sns.saveBits.setBit( chan, setOn );
        E.sns.uiSaveChanStr = Subset::bits2RngStr( E.sns.saveBits );

        Debug()
            << "New imec subset string: "
            << E.sns.uiSaveChanStr;

        imTab->updateProbe( E, ip );
        imTab->saveSettings();
    }
}


void ConfigCtl::graphSetsObSaveBit( int chan, bool setOn, int ip )
{
    DAQ::Params     &p  = acceptedParams;
    CimCfg::ObxEach &E  = p.im.obxj[ip];

    if( chan >= 0 && chan < p.stream_nChans( jsOB, ip ) ) {

        E.sns.saveBits.setBit( chan, setOn );
        E.sns.uiSaveChanStr = Subset::bits2RngStr( E.sns.saveBits );

        Debug()
            << "New obx subset string: "
            << E.sns.uiSaveChanStr;

        obxTab->updateObx( E, ip );
        obxTab->saveSettings();
    }
}


void ConfigCtl::graphSetsNiSaveBit( int chan, bool setOn )
{
    DAQ::Params &p = acceptedParams;

    if( chan >= 0 && chan < p.stream_nChans( jsNI, 0 ) ) {

        p.ni.sns.saveBits.setBit( chan, setOn );
        p.ni.sns.uiSaveChanStr = Subset::bits2RngStr( p.ni.sns.saveBits );

        Debug()
            << "New nidq subset string: "
            << p.ni.sns.uiSaveChanStr;

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


// Get standard channel string listing channels in ShankMap order.
//
bool ConfigCtl::chanMapGetsShankOrder(
    QString         &s,
    const QString   type,
    int             ip,
    bool            rev,
    QWidget         *parent ) const
{
    if( DAQ::Params::stream_isOB( type ) )
        return true;

    DAQ::Params q;
    QString     err;

    if( !shankParamsToQ( err, q, ip ) ) {
        QMessageBox::critical( parent, "ACQ Parameter Error", err );
        return false;
    }

    if( q.stream_isNI( type ) ) {

        if( rev )
            q.ni.sns.shankMap.revChanOrderFromMapNi( s );
        else
            q.ni.sns.shankMap.chanOrderFromMapNi( s );
    }
    else {

        const CimCfg::PrbEach   &E = q.im.prbj[ip];

        if( rev )
            E.sns.shankMap.revChanOrderFromMapIm( s, E.roTbl->nLF() );
        else
            E.sns.shankMap.chanOrderFromMapIm( s, E.roTbl->nLF() );
    }

    return true;
}


void ConfigCtl::setSelectiveAccess( bool availIM, bool availNI )
{
    if( availIM ) {
        prbTab.buildQualIndexTables();
        usingIM = prbTab.nLogProbes() > 0;
        usingOB = prbTab.nLogOnebox() > 0;
    }

    usingNI = availNI;

// Main buttons

    if( availIM || availNI ) {
        cfgUI->resetBut->setEnabled( true );
        cfgUI->verifyBut->setEnabled( true );
        cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
        cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setDefault( true );
        cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setFocus();
    }

// Tabs

    if( usingIM ) {
        imTab->toGUI( acceptedParams );
        cfgUI->tabsW->setTabEnabled( 1, true );
    }

    if( usingOB ) {
        obxTab->toGUI();
        cfgUI->tabsW->setTabEnabled( 2, true );
    }

    if( usingNI ) {
        niTab->toGUI( acceptedParams );
        cfgUI->tabsW->setTabEnabled( 3, true );
    }

    if( availIM || availNI ) {
        syncTab->toGUI( acceptedParams );
        gateTab->toGUI( acceptedParams );
        trigTab->toGUI( acceptedParams );
        snsTab->toGUI( acceptedParams );
        cfgUI->tabsW->setTabEnabled( 4, true );
        cfgUI->tabsW->setTabEnabled( 5, true );
        cfgUI->tabsW->setTabEnabled( 6, true );
        cfgUI->tabsW->setTabEnabled( 7, true );
    }
}


void ConfigCtl::streamCB_fillConfig( QComboBox *CB ) const
{
    SignalBlocker   b(CB);

    CB->clear();

    if( usingNI )
        CB->addItem( DAQ::Params::jsip2stream( jsNI, 0 ) );

    if( usingOB ) {
        for( int ip = 0, np = prbTab.nLogOnebox(); ip < np; ++ip )
            CB->addItem( DAQ::Params::jsip2stream( jsOB, ip ) );
    }

    if( usingIM ) {
        for( int ip = 0, np = prbTab.nLogProbes(); ip < np; ++ip )
            CB->addItem( DAQ::Params::jsip2stream( jsIM, ip ) );
    }
}


bool ConfigCtl::validImROTbl( QString &err, CimCfg::PrbEach &E, int ip ) const
{
// Pretties ini file, even if not using device
    if( E.imroFile.contains( "*" ) )
        E.imroFile.clear();

    if( !usingIM )
        return true;

    int type = prbTab.get_iProbe( ip ).type;

    if( E.roTbl )
        delete E.roTbl;

    E.roTbl = IMROTbl::alloc( type );

    if( E.imroFile.isEmpty() ) {

        E.roTbl->fillDefault();
        return true;
    }

    QString msg;

    if( !E.roTbl->loadFile( msg, E.imroFile ) ) {

        err = QString("ImroFile: %1.").arg( msg );
        return false;
    }

    if( int(E.roTbl->type) != type ) {

        err = QString("Type %1 named in imro file for probe %2.")
                .arg( E.roTbl->type )
                .arg( ip );
        return false;
    }

    return true;
}


bool ConfigCtl::validDataDir( QString &err ) const
{
    MainApp *app = mainApp();

    for( int idir = 0, ndir = app->nDataDirs(); idir < ndir; ++idir ) {

        const QString &s = app->dataDir( idir );

        if( !QDir( s ).exists() ) {
            err = QString("Data directory %1 does not exist [%2].")
                    .arg( idir ).arg( s );
            return false;
        }
    }

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

    for( int ip = 0, np = q.stream_nIM(); ip < np; ++ip ) {

        if( !validImSaveBits( err, q, ip ) )
            return false;
    }

    for( int ip = 0, np = q.stream_nOB(); ip < np; ++ip ) {

        if( !validObChannels( err, q.im.obxj[ip], ip ) )
            return false;

        if( !validObSaveBits( err, q, ip ) )
            return false;
    }

    if( !validNiDevices( err, q )
        || !validNiClock( err, q )
        || !validNiChannels( err, q,
                vcMN1, vcMA1, vcXA1, vcXD1,
                vcMN2, vcMA2, vcXA2, vcXD2,
                uiStr1Err, uiStr2Err )
        || !validNiSaveBits( err, q ) ) {

        return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Space-separated list of current saved chans.
// Used for remote GETSTREAMSAVECHANS command.
//
QString ConfigCtl::cmdSrvGetsSaveChansIm( int ip ) const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );

    if( ip < acceptedParams.stream_nIM() ) {

        const QBitArray &B = acceptedParams.im.prbj[ip].sns.saveBits;
        int             nb = B.size();

        for( int i = 0; i < nb; ++i ) {

            if( B.testBit( i ) )
                ts << i << " ";
        }
    }

    if( s.endsWith( " " ) )
        s.chop( 1 );

    s += "\n";

    return s;
}


// Space-separated list of current saved chans.
// Used for remote GETSTREAMSAVECHANS command.
//
QString ConfigCtl::cmdSrvGetsSaveChansOb( int ip ) const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );

    if( ip < acceptedParams.stream_nOB() ) {

        const QBitArray &B = acceptedParams.im.obxj[ip].sns.saveBits;
        int             nb = B.size();

        for( int i = 0; i < nb; ++i ) {

            if( B.testBit( i ) )
                ts << i << " ";
        }
    }

    if( s.endsWith( " " ) )
        s.chop( 1 );

    s += "\n";

    return s;
}


// Space-separated list of current saved chans.
// Used for remote GETSTREAMSAVECHANS command.
//
QString ConfigCtl::cmdSrvGetsSaveChansNi() const
{
    QString         s;
    QTextStream     ts( &s, QIODevice::WriteOnly );
    const QBitArray &B = acceptedParams.ni.sns.saveBits;
    int             nb = B.size();

    for( int i = 0; i < nb; ++i ) {

        if( B.testBit( i ) )
            ts << i << " ";
    }

    if( s.endsWith( " " ) )
        s.chop( 1 );

    s += "\n";

    return s;
}


// Remote gets params of type:
// {0=GETPARAMS, 1=GETPARAMSIMALL, 2=GETPARAMSIMPRB(ip), 3=GETPARAMSOBX(ip)}.
//
QString ConfigCtl::cmdSrvGetsParamStr( int type, int ip ) const
{
    switch( type ) {
        case 0: return DAQ::Params::remoteGetDAQParams();
        case 1: return CimCfg::PrbAll::remoteGetPrbAll();
        case 2: return acceptedParams.im.prbj[ip].remoteGetPrbEach();
        case 3: return acceptedParams.im.obxj[ip].remoteGetObxEach();
    }
}


// Return empty QString or error string.
// Remote gets params of type:
// {0=SETPARAMS, 1=SETPARAMSIMALL, 2=SETPARAMSIMPRB(ip), 3=SETPARAMSOBX(ip)}.
//
QString ConfigCtl::cmdSrvSetsParamStr( const QString &paramString, int type, int ip )
{
    if( !validated )
        return "Run parameters never validated.";

// -------------------------------
// Save settings to "_remote" file
// -------------------------------

// First write current set

    acceptedParams.saveSettings( true );

// Then overwrite entries

    switch( type ) {
        case 0: DAQ::Params::remoteSetDAQParams( paramString ); break;
        case 1: CimCfg::PrbAll::remoteSetPrbAll( paramString ); break;
    }

// ----------------------
// Early parameter checks
// ----------------------

    DAQ::Params p;
    QString     err;

    p.loadSettings( true );

    if( type == 0 ) {
        if( !p.im.enabled && !p.ni.enabled )
            return "No streams enabled.";
    }

    if( type > 0 ) {
        if( !p.im.enabled )
            return "Imec not enabled.";
    }

    if( type == 1 ) {
        err = imTab->remoteVfyPrbAll( p.im, acceptedParams.im );
        if( !err.isEmpty() )
            return err;
    }

// -----------------------
// Transfer them to dialog
// -----------------------

// Assume hardware unchanged since time user manually clicked Detect

    devTab->toGUI( p, false );
    setNoDialogAccess();

// Now set usingXX as if Detect successful

    if( p.im.enabled ) {
        prbTab.buildQualIndexTables();
        usingIM = prbTab.nLogProbes() > 0;
        usingOB = prbTab.nLogOnebox() > 0;
    }
    else {
        usingIM = false;
        usingOB = false;
    }

    usingNI = p.ni.enabled;

    devTab->updateIMParams();

    if( usingIM ) {
        imTab->toGUI( p );
        if( type == 2 ) {
            err = imTab->remoteSetPrbEach( paramString, ip );
            if( !err.isEmpty() )
                return err;
        }
    }

    if( usingOB ) {
        obxTab->toGUI();
        if( type == 3 ) {
            err = obxTab->remoteSetObxEach( paramString, ip );
            if( !err.isEmpty() )
                return err;
        }
    }

    if( usingNI )
        niTab->toGUI( p );
    else
        niTab->singletonRelease();

    if( p.im.enabled || p.ni.enabled ) {
        syncTab->toGUI( p );
        gateTab->toGUI( p );
        trigTab->toGUI( p );
        snsTab->toGUI( p );
    }

// --------------------------
// Remote-specific validation
// --------------------------

// With a dialog, user is constrained to choose items
// we've put into CB controls. Remote case lacks that
// constraint, so we check existence of CB items here.

    if( type == 0 && usingNI ) {
        if( !niTab->remoteValidate( err, p ) )
            return err;
    }

// -------------------
// Standard validation
// -------------------

    if( type != 2 || !mainApp()->getRun()->isRunning() )
        ip = -1;

    if( !valid( err, 0, ip ) ) {

        err = QString("ACQ Parameter Error [%1]")
                .arg( err.replace( "\n", " " ) );
    }

    return err;
}


void ConfigCtl::initUsing_im_ob()
{
    usingIM = false;
    usingOB = false;
    prbTab.init();
    setNoDialogAccess( false );
}


void ConfigCtl::initUsing_ni()
{
    usingNI = false;
    setNoDialogAccess();
}


void ConfigCtl::initUsing_all()
{
    usingIM = false;
    usingOB = false;
    usingNI = false;
    prbTab.init();
    setNoDialogAccess();
}


void ConfigCtl::updateCalWarning()
{
    devTab->showCalWarning(
        imTab->calPolicyIsNever() || !prbTab.haveQualCalFiles() );
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ConfigCtl::tabChanged( int tab )
{
    QString s;

    switch( cfgUI->tabsW->currentIndex() ) {
        case 0: s = "Devices Help"; break;
        case 1: s = "IM Help"; break;
        case 2: s = "Obx Help"; break;
        case 3: s = "NI Help"; break;
        case 4: s = "Sync Help"; break;
        case 5: s = "Gates Help"; break;
        case 6: s = "Trigs Help"; break;
        case 7: s = "Save Help"; break;
    }

    cfgUI->helpBut->setText( s );
}


void ConfigCtl::tabHelp()
{
    QString s;

    switch( cfgUI->tabsW->currentIndex() ) {
        case 0: s = "DevTab_Help"; break;
        case 1: s = "IMTab_Help"; break;
        case 2: s = "OBXTab_Help"; break;
        case 3: s = "NITab_Help"; break;
        case 4: s = "SyncTab_Help"; break;
        case 5: s = "GateTab_Help"; break;
        case 6: s = "TrigTab_Help"; break;
        case 7: s = "SaveTab_Help"; break;
    }

    showHelp( s );
}


void ConfigCtl::reset()
{
    prbTab.loadProbeSRates();
    prbTab.loadOneboxSRates();
    prbTab.loadProbeTable();
    acceptedParams.loadSettings();

    devTab->toGUI( acceptedParams, true );
    imTab->reset( acceptedParams );
    setNoDialogAccess();
    syncTab->resetCalRunMode();
}


void ConfigCtl::verify()
{
    QString err;

    if( valid( err, cfgDlg ) )
        ;
    else if( !err.isEmpty() )
        QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
}


void ConfigCtl::okBut()
{
    QString err;

    if( valid( err, cfgDlg ) )
        cfgDlg->accept();
    else if( !err.isEmpty() )
        QMessageBox::critical( cfgDlg, "ACQ Parameter Error", err );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ConfigCtl::setNoDialogAccess( bool clearNi )
{
    devTab->setNoDialogAccess( clearNi );

// Disk capacity text

    snsTab->setDialogNoAccess();

// Can't tab

    for( int i = 1, n = cfgUI->tabsW->count(); i < n; ++i )
        cfgUI->tabsW->setTabEnabled( i, false );

// Can't verify or ok

    cfgUI->resetBut->setDisabled( true );
    cfgUI->verifyBut->setDisabled( true );
    cfgUI->buttonBox->button( QDialogButtonBox::Ok )->setDisabled( true );
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
// ----------
// IMEC / OBX
// ----------

    q.im.set_cfg_def_no_streams( acceptedParams.im );

    if( usingIM ) {
        devTab->fromGUI( q );
        imTab->fromGUI( q );
    }

    if( usingOB )
        obxTab->fromGUI( q );

// ----
// NIDQ
// ----

    if( usingNI ) {

        niTab->fromGUI( q,
                vcMN1, vcMA1, vcXA1, vcXD1,
                vcMN2, vcMA2, vcXA2, vcXD2,
                uiStr1Err, uiStr2Err );
    }
    else {
        q.ni                = acceptedParams.ni;
        q.ni.enabled        = false;
    }

    q.ni.deriveChanCounts();

    syncTab->fromGUI( q );
    gateTab->fromGUI( q );
    trigTab->fromGUI( q );
    snsTab->fromGUI( q );
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


// Test that LED settings agree for probes in same HS.
//
bool ConfigCtl::validImLEDs( QString &err, DAQ::Params &q ) const
{
    if( !usingIM )
        return true;

    int np = q.stream_nIM();

    for( int ip = np - 1; ip > 0; --ip ) {

        const CimCfg::ImProbeDat  &P = prbTab.get_iProbe( ip );

        if( P.dock != 2 )
            continue;

        const CimCfg::ImProbeDat  &Q = prbTab.get_iProbe( ip -1 );

        if( Q.dock != 1 || Q.port != P.port || Q.slot != P.slot )
            continue;

        if( q.im.prbj[ip].LEDEnable != q.im.prbj[ip - 1].LEDEnable ) {

            err =
            QString(
            "Probes {%1,%2} are in the same headstage (slot %3, port %4)"
            " but their headstage LED settings do not match.")
            .arg( ip - 1 ).arg( ip )
            .arg( P.slot ).arg( P.port );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validImStdbyBits( QString &err, CimCfg::PrbEach &E, int ip ) const
{
    if( !usingIM )
        return true;

    return E.deriveStdbyBits(
            err, E.imCumTypCnt[CimCfg::imSumAP], ip );
}


bool ConfigCtl::validObChannels( QString &err, CimCfg::ObxEach &E, int ip ) const
{
    if( !usingOB )
        return true;

    QVector<uint>   vc;
    int             nAI;

    if( !Subset::rngStr2Vec( vc, E.uiXAStr ) ) {
        err = QString("Obx %1: XA format error.").arg( ip );
        return false;
    }

// No channels?

    nAI = vc.size();

    if( !(nAI + E.digital) ) {
        err = QString("Obx %1: No channels specified.").arg( ip );
        return false;
    }

// Illegal channels?

    if( nAI && vc.last() > 11 ) {
        err = QString("Obx %1: XA channel indices must not exceed 11.").arg( ip );
        return false;
    }

    return true;
}


bool ConfigCtl::validNiDevices( QString &err, DAQ::Params &q ) const
{
    if( !usingNI )
        return true;

// ----
// Dev1
// ----

    if( !q.ni.dev1.length() ) {

        err =
        "No NIDAQ input devices installed.\n\n"
        "Resolve issues in NI 'Measurement & Automation Explorer'.";
        return false;
    }

// ----
// Dev2
// ----

    if( !q.ni.isDualDevMode )
        return true;

    if( !q.ni.dev2.length() ) {

        err =
        "No secondary NIDAQ input devices installed.\n\n"
        "Resolve issues in NI 'Measurement & Automation Explorer'.";
        return false;
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


bool ConfigCtl::validNiClock( QString &err, DAQ::Params &q ) const
{
    if( !usingNI )
        return true;

    if( niTab->nSources() <= 0 || q.ni.clockSource.isEmpty() ) {

        err =
        "Define at least one NI clock source and its sample rate.\n\n"
        "On NI Setup tab, select a listed clock, or click 'New Source'.";
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
    if( !usingNI )
        return true;

    int     maxAI,
            maxDI,
            nAI,
            nDI,
            doStartLine = -1;
    bool    isMux = niTab->isMuxingFromDlg();

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

    maxAI = CniCfg::aiDevChanCount[q.ni.dev1];
    maxDI = CniCfg::diDevLineCount[q.ni.dev1];

    if( nAI && !maxAI ) {
        err ="Primary NI device is digital; AI channels not supported.";
        return false;
    }

    --maxAI;
    --maxDI;

    if( (vcMN1.count() && int(vcMN1.last()) > maxAI)
        || (vcMA1.count() && int(vcMA1.last()) > maxAI)
        || (vcXA1.count() && int(vcXA1.last()) > maxAI) ) {

        err =
        QString("Primary NI device AI channel values must not exceed [%1].")
        .arg( maxAI );
        return false;
    }

    if( vcXD1.count() && int(vcXD1.last()) > maxDI ) {

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

    double  setRate = q.ni.key2SetRate( q.ni.clockSource ),
            rMax    = CniCfg::maxSampleRate( q.ni.dev1, nAI );

    if( setRate > rMax ) {

        if( nAI ) {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 1 maximum (%2)"
            " for (%3) analog channels.")
            .arg( setRate )
            .arg( rMax )
            .arg( nAI );
        }
        else {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 1 maximum (%2)"
            " for digital channels.")
            .arg( setRate )
            .arg( rMax );
        }

        return false;
    }

// Wrong termination type?

    if( nAI &&
        CniCfg::wrongTermConfig(
            err, q.ni.dev1,
            vcMN1 + vcMA1 + vcXA1,
            q.ni.termCfg ) ) {

        return false;
    }

// Start line cannot be digital input

    if( q.ni.startEnable && vcXD1.count() ) {

        QString dev;
        CniCfg::parseDIStr( dev, doStartLine, q.ni.startLine );

        if( dev == q.ni.dev1 && vcXD1.contains( doStartLine ) ) {

            err =
            "Start output line cannot be used as a digital input"
            " line on primary NI device.";
            return false;
        }
    }

// Sync output cannot be digital input

    if( CniCfg::isDigitalDev( q.ni.dev1 ) ) {

        DAQ::SyncSource sourceIdx = syncTab->curSource();

        if( sourceIdx == DAQ::eSyncSourceNI && vcXD1.contains( 0 ) ) {

            err =
            "Sync output line (0) cannot be used as a digital input"
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

    maxAI = CniCfg::aiDevChanCount[q.ni.dev2];
    maxDI = CniCfg::diDevLineCount[q.ni.dev2];

    if( nAI && !maxAI ) {
        err ="Secondary NI device is digital; AI channels not supported.";
        return false;
    }

    --maxAI;
    --maxDI;

    if( (vcMN2.count() && int(vcMN2.last()) > maxAI)
        || (vcMA2.count() && int(vcMA2.last()) > maxAI)
        || (vcXA2.count() && int(vcXA2.last()) > maxAI) ) {

        err =
        QString("Secondary NI device AI chan values must not exceed [%1].")
        .arg( maxAI );
        return false;
    }

    if( vcXD2.count() && int(vcXD2.last()) > maxDI ) {

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

    if( setRate > rMax ) {

        if( nAI ) {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 2 maximum (%2)"
            " for (%3) analog channels.")
            .arg( setRate )
            .arg( rMax )
            .arg( nAI );
        }
        else {
            err =
            QString(
            "Sampling rate [%1] exceeds dev 2 maximum (%2)"
            " for digital channels.")
            .arg( setRate )
            .arg( rMax );
        }

        return false;
    }

// Wrong termination type?

    if( nAI &&
        CniCfg::wrongTermConfig(
            err, q.ni.dev2,
            vcMN2 + vcMA2 + vcXA2,
            q.ni.termCfg ) ) {

        return false;
    }

// Start line cannot be digital input

    if( doStartLine >= 0 && vcXD2.count() ) {

        if( vcXD2.contains( doStartLine ) ) {

            err =
            "Common start output line cannot be used as digital"
            " input on either device.";
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validImShankMap( QString &err, CimCfg::PrbEach &E, int ip ) const
{
// Pretties ini file, even if not using device
    if( E.sns.shankMapFile.contains( "*" ) )
        E.sns.shankMapFile.clear();

    if( !usingIM )
        return true;

    ShankMap    &M = E.sns.shankMap;

    if( E.sns.shankMapFile.isEmpty() ) {

        M.fillDefaultIm( *E.roTbl );

        // Save in case stdby channels changed
        E.sns.shankMap_orig = M;

        M.andOutImStdby( E.stdbyBits );

        return true;
    }

    QString msg;

    if( !M.loadFile( msg, E.sns.shankMapFile ) ) {

        err = QString("IM ShankMap: %1.").arg( msg );
        return false;
    }

    int N;

    N = E.roTbl->nElec();

    if( M.nSites() != N ) {

        err = QString(
                "IM %1 ShankMap header mismatch--\n\n"
                "  - Cur config: %2 electrodes\n"
                "  - Named file: %3 electrodes.")
                .arg( ip )
                .arg( N ).arg( M.nSites() );
        return false;
    }

    N = E.roTbl->nChan();

    if( int(M.e.size()) != N ) {

        err = QString(
                "IM ShankMap entry mismatch--\n\n"
                "  - Cur config: %1 channels\n"
                "  - Named file: %2 channels.")
                .arg( N ).arg( M.e.size() );
        return false;
    }

    M.andOutImRefs( *E.roTbl );

    // Save in case stdby channels changed
    E.sns.shankMap_orig = M;

    M.andOutImStdby( E.stdbyBits );

    return true;
}


bool ConfigCtl::validNiShankMap( QString &err, DAQ::Params &q ) const
{
// Pretties ini file, even if not using device
    if( q.ni.sns.shankMapFile.contains( "*" ) )
        q.ni.sns.shankMapFile.clear();

    if( !usingNI )
        return true;

    ShankMap    &M      = q.ni.sns.shankMap;
    int         nChan   = q.ni.niCumTypCnt[CniCfg::niTypeMN];

    if( q.ni.sns.shankMapFile.isEmpty() ) {

        // Single shank, two columns
        M.fillDefaultNi( 1, 2, nChan/2, nChan );
        return true;
    }

    QString msg;

    if( !M.loadFile( msg, q.ni.sns.shankMapFile ) ) {

        err = QString("NI ShankMap: %1.").arg( msg );
        return false;
    }

    if( M.nSites() < nChan ) {

        err = QString(
                "NI ShankMap has too few probe sites--\n\n"
                "  - Cur config: %1 channels\n"
                "  - Named file: %2 electrodes.")
                .arg( nChan ).arg( M.nSites() );
        return false;
    }

    if( int(M.e.size()) != nChan ) {

        err = QString(
                "NI ShankMap entry mismatch--\n\n"
                "  - Cur config: %1 channels\n"
                "  - Named file: %2 channels.")
                .arg( nChan ).arg( M.e.size() );
        return false;
    }

    return true;
}


bool ConfigCtl::validImChanMap( QString &err, CimCfg::PrbEach &E, int ip ) const
{
// Pretties ini file, even if not using device
    if( E.sns.chanMapFile.contains( "*" ) )
        E.sns.chanMapFile.clear();

    if( !usingIM )
        return true;

    ChanMapIM   &M = E.sns.chanMap;

    if( E.sns.chanMapFile.isEmpty() ) {

        M.setImroOrder( E.roTbl );
        return true;
    }

    QString msg;

    if( !M.loadFile( msg, E.sns.chanMapFile ) ) {

        err = QString("IM ChanMap: %1.").arg( msg );
        return false;
    }

    const int   *type = E.imCumTypCnt;
    ChanMapIM   D(
        type[CimCfg::imTypeAP],
        type[CimCfg::imTypeLF] - type[CimCfg::imTypeAP],
        type[CimCfg::imTypeSY] - type[CimCfg::imTypeLF] );

    if( !M.equalHdr( D ) ) {

        err = QString(
                "IM %1 ChanMap header mismatch--\n\n"
                "  - Cur config: (%2 %3 %4)\n"
                "  - Named file: (%5 %6 %7).")
                .arg( ip )
                .arg( D.AP ).arg( D.LF ).arg( D.SY )
                .arg( M.AP ).arg( M.LF ).arg( M.SY );
        return false;
    }

    return true;
}


bool ConfigCtl::validObChanMap( QString &err, DAQ::Params &q, int ip ) const
{
    CimCfg::ObxEach &E = q.im.obxj[ip];

// Pretties ini file, even if not using device
    if( E.sns.chanMapFile.contains( "*" ) )
        E.sns.chanMapFile.clear();

    if( !usingOB )
        return true;

    const int   *type   = E.obCumTypCnt;
    ChanMapOB   &M      = E.sns.chanMap;
    ChanMapOB   D(
        type[CimCfg::obTypeXA],
        type[CimCfg::obTypeXD] - type[CimCfg::obTypeXA],
        type[CimCfg::obTypeSY] - type[CimCfg::obTypeXD] );

    if( E.sns.chanMapFile.isEmpty() ) {

        M = D;
        M.fillDefault();
        return true;
    }

    QString msg;

    if( !M.loadFile( msg, E.sns.chanMapFile ) ) {

        err = QString("OBX ChanMap: %1.").arg( msg );
        return false;
    }

    if( !M.equalHdr( D ) ) {

        err = QString(
                "OBX ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3)\n"
                "  - Named file: (%4 %5 %6).")
                .arg( D.XA ).arg( D.XD ).arg( D.SY )
                .arg( M.XA ).arg( M.XD ).arg( M.SY );
        return false;
    }

    return true;
}


bool ConfigCtl::validNiChanMap( QString &err, DAQ::Params &q ) const
{
// Pretties ini file, even if not using device
    if( q.ni.sns.chanMapFile.contains( "*" ) )
        q.ni.sns.chanMapFile.clear();

    if( !usingNI )
        return true;

    const int   *type   = q.ni.niCumTypCnt;
    ChanMapNI   &M      = q.ni.sns.chanMap;
    ChanMapNI   D(
        type[CniCfg::niTypeMN] / q.ni.muxFactor,
        (type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN]) / q.ni.muxFactor,
        q.ni.muxFactor,
        type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA],
        type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

    if( q.ni.sns.chanMapFile.isEmpty() ) {

        M = D;
        M.fillDefault();
        return true;
    }

    QString msg;

    if( !M.loadFile( msg, q.ni.sns.chanMapFile ) ) {

        err = QString("NI ChanMap: %1.").arg( msg );
        return false;
    }

    if( !M.equalHdr( D ) ) {

        err = QString(
                "NI ChanMap header mismatch--\n\n"
                "  - Cur config: (%1 %2 %3 %4 %5)\n"
                "  - Named file: (%6 %7 %8 %9 %10).")
                .arg( D.MN ).arg( D.MA ).arg( D.C ).arg( D.XA ).arg( D.XD )
                .arg( M.MN ).arg( M.MA ).arg( M.C ).arg( M.XA ).arg( M.XD );
        return false;
    }

    return true;
}


bool ConfigCtl::validImSaveBits( QString &err, DAQ::Params &q, int ip ) const
{
    if( !usingIM )
        return true;

    CimCfg::PrbEach &E = q.im.prbj[ip];
    int             nC = q.stream_nChans( jsIM, ip );
    bool            ok;

    ok = E.sns.deriveSaveBits( err, q.jsip2stream( jsIM, ip ), nC );

    if( ok )
        imTab->regularizeSaveChans( E, nC, ip );

    return ok;
}


bool ConfigCtl::validObSaveBits( QString &err, DAQ::Params &q, int ip ) const
{
    if( !usingOB )
        return true;

    CimCfg::ObxEach &E = q.im.obxj[ip];
    int             nC = q.stream_nChans( jsOB, ip );
    bool            ok;

    ok = E.sns.deriveSaveBits( err, q.jsip2stream( jsOB, ip ), nC );

    if( ok )
        obxTab->regularizeSaveChans( E, nC, ip );

    return ok;
}


bool ConfigCtl::validNiSaveBits( QString &err, DAQ::Params &q ) const
{
    if( !usingNI )
        return true;

    return q.ni.sns.deriveSaveBits(
            err, q.jsip2stream( jsNI, 0 ), q.stream_nChans( jsNI, 0 ) );
}


bool ConfigCtl::validSyncTab( QString &err, DAQ::Params &q ) const
{
    if( q.sync.sourceIdx == DAQ::eSyncSourceNI ) {

#ifndef HAVE_NIDAQmx
        err =
        "NI sync source unavailable: NI simulated.";
        return false;
#endif

        if( !usingNI ) {

            err =
            "NI sync source selected but Nidq not enabled.";
            return false;
        }
    }

    if( usingIM ) {

        if( (   // source not PXI
                q.sync.sourceIdx < DAQ::eSyncSourceIM ||
                !prbTab.isSlotPXIType( prbTab.getEnumSlot( q.sync.sourceIdx - DAQ::eSyncSourceIM ) )
            )
            && prbTab.anySlotPXIType() ) {

            if( !prbTab.isSlotUsed( q.sync.imPXIInputSlot ) ) {

                err =
                QString(
                "Sync tab: IM PXI SMA input slot %1 is not enabled.")
                .arg( q.sync.imPXIInputSlot );
                return false;
            }
            else if( !prbTab.isSlotPXIType( q.sync.imPXIInputSlot ) ) {

                err =
                QString(
                "Sync tab: IM PXI SMA input slot %1 is not PXI slot.")
                .arg( q.sync.imPXIInputSlot );
                return false;
            }
        }
    }

    if( q.sync.sourceIdx == DAQ::eSyncSourceNone )
        return true;

    if( usingNI ) {

        if( q.sync.niChanType == 1 ) {

            // Tests for analog channel and threshold

            int chMin = q.ni.niCumTypCnt[CniCfg::niSumNeural],
                chMax = q.ni.niCumTypCnt[CniCfg::niSumAnalog];

            if( chMin >= chMax ) {

                err =
                "NI sync channel invalid: No aux AI channels configured.";
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

            if( !q.ni.sns.saveBits.testBit( q.sync.niChan ) ) {

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

            if( !Subset::rngStr2Vec( vc1, q.ni.uiXDStr1 ) ) {

                err = "Primary NI device XD list has a format error.";
                return false;
            }

            if( !Subset::rngStr2Vec( vc2, q.ni.uiXDStr2() ) ) {

                err = "Secondary NI device XD list has a format error.";
                return false;
            }

            if( vc1.size() + vc2.size() == 0 ) {

                err =
                "Sync tab: No NI digital lines have been specified.";
                return false;
            }

            int xdbits1 = 8 * q.ni.xdBytes1;

            if( q.sync.niChan < xdbits1
                && !vc1.contains( q.sync.niChan ) ) {

                err =
                QString(
                "NI sync bit [%1] not in primary NI device"
                " XD chan list [%2].")
                .arg( q.sync.niChan )
                .arg( q.ni.uiXDStr1 );
                return false;
            }

            if( q.sync.niChan >= xdbits1
                && !vc2.contains( q.sync.niChan - xdbits1 ) ) {

                err =
                QString(
                "NI sync bit [%1] (secondary bit %2)"
                " not in secondary NI device XD chan list [%3].")
                .arg( q.sync.niChan )
                .arg( q.sync.niChan - xdbits1 )
                .arg( q.ni.uiXDStr2() );
                return false;
            }

            int dword = q.ni.niCumTypCnt[CniCfg::niSumAnalog]
                            + q.sync.niChan/16;

            if( !q.ni.sns.saveBits.testBit( dword ) ) {

                err =
                QString(
                "NI sync word (chan %1) not included in saved channels.")
                .arg( dword );
                return false;
            }
        }
    }

    if( q.sync.isCalRun && usingIM && q.im.prbAll.isSvyRun ) {

        err =
        "Can't do calibration run (Sync tab) and"
        " probe survey run (IM tab) at same time.";
        return false;
    }

    return true;
}


bool ConfigCtl::validImTriggering( QString &err, DAQ::Params &q ) const
{
    if( !usingIM ) {

        err =
        QString(
        "Imec stream selected for '%1' trigger but not enabled/detected.")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) );
        return false;
    }

    if( q.mode.mTrig == DAQ::eTrigSpike
        || (q.mode.mTrig == DAQ::eTrigTTL && q.trgTTL.isAnalog) ) {

        // Tests for analog channel and threshold

        int trgChan = q.trigChan(),
            ip      = q.stream2ip( q.trigStream() );

        const CimCfg::PrbEach   &E = q.im.prbj[ip];

        int nLegal  = E.imCumTypCnt[CimCfg::imSumNeural],
            maxInt  = E.roTbl->maxInt();

        if( trgChan < 0 || trgChan >= nLegal ) {

            err =
            QString(
            "Invalid '%1' trigger channel [%2]; must be in range [0..%3].")
            .arg( DAQ::trigModeToString( q.mode.mTrig ) )
            .arg( trgChan )
            .arg( nLegal - 1 );
            return false;
        }

        double  Tmin = E.intToV( -maxInt, trgChan ),
                Tmax = E.intToV(  maxInt - 1, trgChan );

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

        if( q.trgTTL.bit != 0 && q.trgTTL.bit != 6 ) {

            err =
            "Only imec bits {0=imec_event, 6=sync} are available.";
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validObTriggering( QString &err, DAQ::Params &q ) const
{
    if( !usingOB ) {

        err =
        QString(
        "Obx stream selected for '%1' trigger but not enabled/detected.")
        .arg( DAQ::trigModeToString( q.mode.mTrig ) );
        return false;
    }

    int ip = q.stream2ip( q.trigStream() );

    const CimCfg::ObxEach   &E = q.im.obxj[ip];

    if( q.mode.mTrig == DAQ::eTrigSpike
        || (q.mode.mTrig == DAQ::eTrigTTL && q.trgTTL.isAnalog) ) {

        // Tests for analog channel and threshold

        int trgChan = q.trigChan(),
            nLegal  = E.obCumTypCnt[CimCfg::obSumAnalog];

        if( nLegal <= 0 ) {

            err =
            QString(
            "Invalid '%1' trigger channel [%2];"
            " No obx analog channels configured.")
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

        double  Tmin = E.int16ToV( -32768 ),
                Tmax = E.int16ToV(  32767 );

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

        if( !E.digital ) {

            err = "Trigger Tab: No obx digital lines have been specified.";
            return false;
        }

        if( q.trgTTL.bit >= im1BX_NCHN ) {

            err =
            QString(
            "Obx TTL trigger bit [%1] must be in range [0..%2].")
            .arg( q.trgTTL.bit )
            .arg( im1BX_NCHN - 1 );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validNiTriggering( QString &err, DAQ::Params &q ) const
{
    if( !usingNI ) {

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

        if( !Subset::rngStr2Vec( vc1, q.ni.uiXDStr1 ) ) {

            err = "Primary NI device XD list has a format error.";
            return false;
        }

        if( !Subset::rngStr2Vec( vc2, q.ni.uiXDStr2() ) ) {

            err = "Secondary NI device XD list has a format error.";
            return false;
        }

        if( vc1.size() + vc2.size() == 0 ) {

            err =
            "Trigger Tab: No NI digital lines have been specified.";
            return false;
        }

        int xdbits1 = 8 * q.ni.xdBytes1;

        if( q.trgTTL.bit < xdbits1
            && !vc1.contains( q.trgTTL.bit ) ) {

            err =
            QString(
            "Nidq TTL trigger bit [%1] not in primary NI device"
            " XD list [%2].")
            .arg( q.trgTTL.bit )
            .arg( q.ni.uiXDStr1 );
            return false;
        }

        if( q.trgTTL.bit >= xdbits1
            && !vc2.contains( q.trgTTL.bit - xdbits1 ) ) {

            err =
            QString(
            "Nidq TTL trigger bit [%1] (secondary bit %2)"
            " not in secondary NI device XD chan list [%3].")
            .arg( q.trgTTL.bit )
            .arg( q.trgTTL.bit - xdbits1 )
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

        stream = 0.50 * mainApp()->getRun()->streamSpanMax( q, false );

        if( trgMrg >= stream ) {

            err =
            QString(
            "The trigger added context secs [%1] must be shorter than"
            " [%2] which is 50% of the expected stream length.")
            .arg( trgMrg )
            .arg( stream );
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validTrgLowTime( QString &err, DAQ::Params &q ) const
{
    if( q.mode.mTrig == DAQ::eTrigTimed ) {

        // Test for negative L too large

        if( q.trgTim.tL < 0 && -q.trgTim.tL > q.trgTim.tH / 2 ) {

            err = "Time trigger: A negative L must be smaller than H/2.";
            return false;
        }
    }

    return true;
}


bool ConfigCtl::validDiskAvail( QString &err, DAQ::Params &q ) const
{
    if( q.sns.reqMins <= 0 )
        return true;

    MainApp *app = mainApp();

    for( int idir = 0, ndir = app->nDataDirs(); idir < ndir; ++idir ) {

        double  BPS = 0;

        if( usingIM ) {

            for( int ip = 0, np = q.stream_nIM(); ip < np; ++ip ) {

                if( ip % ndir != idir )
                    continue;

                const CimCfg::PrbEach   &E = q.im.prbj[ip];

                BPS += E.apSaveChanCount() * E.srate * 2;

                if( E.lfIsSaving() )
                    BPS += E.lfSaveChanCount() * E.srate/12 * 2;
            }
        }

        if( idir == 0 ) {

            if( usingOB ) {

                for( int ip = 0, np = q.stream_nOB(); ip < np; ++ip ) {

                    const CimCfg::ObxEach   &E = q.im.obxj[ip];
                    BPS += E.sns.saveBits.count( true ) * E.srate * 2;
                }
            }

            if( usingNI )
                BPS += q.ni.sns.saveBits.count( true ) * q.ni.srate * 2;
        }

        if( BPS > 0 ) {

            quint64 avail   = availableDiskSpace( idir );
            int     mins    = avail / BPS / 60;

            if( mins <= q.sns.reqMins ) {

                err =
                QString("Space to record in directory %1 for ~%2 min (required = %3).")
                .arg( idir )
                .arg( mins )
                .arg( q.sns.reqMins );
                return false;
            }
        }
    }

    return true;
}


// Return idir where item found or -1 if none.
//
static int runNameExists( const DAQ::Params &q )
{
    MainApp *app = mainApp();
    int     ndir = app->nDataDirs();

    if( q.mode.initG == -1 ) {

        // This is a fresh name.
        // It must be unused in all dataDir.
        // Pattern:
        // - runname_gN     (run folder),
        // - runname.xxx    (log file).

        QRegExp re( QString("%1_g\\d+|%1\\.").arg( q.sns.runName ) );
        re.setCaseSensitivity( Qt::CaseInsensitive );

        for( int idir = 0; idir < ndir; ++idir ) {

            QDirIterator    it( app->dataDir( idir ) );

            while( it.hasNext() ) {

                it.next();

                if( re.indexIn( it.fileName() ) == 0 )
                    return idir;
            }
        }
    }
    else if( q.sns.fldPerPrb ) {

        // This is a continuation.
        // We can use the specific (g,t) if no such bin.

        QFileInfo   fi;

        for( int idir = 0; idir < ndir; ++idir ) {

            QString base = QString("%1/%2_g%3/%2_g%3_")
                            .arg( app->dataDir( idir ) )
                            .arg( q.sns.runName )
                            .arg( q.mode.initG );

            if( idir == 0 ) {

                fi.setFile( base + QString("t%1.nidq.bin").arg( q.mode.initT ) );
                if( fi.exists() )
                    return idir;

                for( int ip = 0, np = q.stream_nOB(); ip < np; ++ip ) {
                    fi.setFile( base + QString("t%1.obx%2.obx.bin")
                                        .arg( q.mode.initT )
                                        .arg( ip ) );
                    if( fi.exists() )
                        return idir;
                }
            }

            for( int ip = 0, np = q.stream_nIM(); ip < np; ++ip ) {

                QString prbBase =
                    base + QString("imec%1/%2_g%3_t%4.imec%1.")
                            .arg( ip )
                            .arg( q.sns.runName )
                            .arg( q.mode.initG )
                            .arg( q.mode.initT );

                fi.setFile( prbBase + "ap.bin" );
                if( fi.exists() )
                    return idir;

                fi.setFile( prbBase + "lf.bin" );
                if( fi.exists() )
                    return idir;
            }
        }
    }
    else {

        // This is a continuation.
        // We can use the specific (g,t) if no such bin.

        QFileInfo   fi;

        for( int idir = 0; idir < ndir; ++idir ) {

            QString base = QString("%1/%2_g%3/%2_g%3_t%4.")
                            .arg( app->dataDir( idir ) )
                            .arg( q.sns.runName )
                            .arg( q.mode.initG )
                            .arg( q.mode.initT );

            if( idir == 0 ) {

                fi.setFile( base + "nidq.bin" );
                if( fi.exists() )
                    return idir;

                for( int ip = 0, np = q.stream_nOB(); ip < np; ++ip ) {
                    fi.setFile( base + QString("obx%1.obx.bin").arg( ip ) );
                    if( fi.exists() )
                        return idir;
                }
            }

            for( int ip = 0, np = q.stream_nIM(); ip < np; ++ip ) {

                fi.setFile( base + QString("imec%1.ap.bin").arg( ip ) );
                if( fi.exists() )
                    return idir;

                fi.setFile( base + QString("imec%1.lf.bin").arg( ip ) );
                if( fi.exists() )
                    return idir;
            }
        }
    }

    return -1;
}


bool ConfigCtl::validRunName(
    QString         &err,
    DAQ::Params     &q,
    QString         runName,
    QWidget         *parent )
{
    if( q.sync.isCalRun || q.im.prbAll.isSvyRun )
        return true;

    if( runName.isEmpty() ) {
        err = "A non-empty run name is required.";
        return false;
    }

    if( FILEHasIllegals( STR2CHR( runName ) ) ) {
        err = "Run names may not contain any of {/\\[]<>*\":;,?|=}";
        return false;
    }

    QRegExp re("(.*)_[gG](\\d)+_[tT](\\d+)$");

    if( runName.contains( re ) ) {

        q.mode.initG    = re.cap(2).toInt();
        q.mode.initT    = re.cap(3).toInt();
        q.sns.runName   = re.cap(1);
    }
    else {
        q.mode.initG    = -1;
        q.mode.initT    = -1;
        q.sns.runName   = runName;
    }

    int idir = runNameExists( q );

    if( idir < 0 )
        return true;

    if( !parent ) {
        err = "Run name already in use.";
        return false;
    }

    int yesNo = QMessageBox::warning(
        parent,
        "Run Name Already Exists",
        QString(
        "You can't overwrite existing name '%1'    \n\n"
        "Open Explorer window?")
        .arg( runName ),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    if( yesNo == QMessageBox::Yes ) {

        QMetaObject::invokeMethod(
            mainApp(),
            "options_ExploreDataDir",
            Qt::QueuedConnection,
            Q_ARG(int, idir) );
    }
    else
        cfgUI->tabsW->setCurrentIndex( 7 );

    return false;
}


bool ConfigCtl::shankParamsToQ( QString &err, DAQ::Params &q, int ip ) const
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

    CimCfg::PrbEach &E = q.im.prbj[ip];

    if( !validImROTbl( err, E, ip ) )
        return false;

    if( !validImStdbyBits( err, E, ip ) )
        return false;

    if( !validImShankMap( err, E, ip ) )
        return false;

    if( !validNiDevices( err, q )
        || !validNiChannels( err, q,
                vcMN1, vcMA1, vcXA1, vcXD1,
                vcMN2, vcMA2, vcXA2, vcXD2,
                uiStr1Err, uiStr2Err ) ) {

        return false;
    }

    if( !validNiShankMap( err, q ) )
        return false;

    return true;
}


// Validate all settings...
// If iprb <  0: Full replacement of acceptedParams.
// If iprb >= 0: Surgical replacement of acceptedParams.im.prbj[iprb].
//
bool ConfigCtl::valid( QString &err, QWidget *parent, int iprb )
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

    if( !validImLEDs( err, q ) )
        return false;

    for( int ip = 0, np = q.stream_nIM(); ip < np; ++ip ) {

        CimCfg::PrbEach &E = q.im.prbj[ip];

        if( !validImROTbl( err, E, ip ) )
            return false;

        if( !validImStdbyBits( err, E, ip ) )
            return false;

        if( !validImShankMap( err, E, ip ) )
            return false;

        if( !validImChanMap( err, E, ip ) )
            return false;

        if( !validImSaveBits( err, q, ip ) )
            return false;
    }

    for( int ip = 0, np = q.stream_nOB(); ip < np; ++ip ) {

        if( !validObChannels( err, q.im.obxj[ip], ip ) )
            return false;

        if( !validObChanMap( err, q, ip ) )
            return false;

        if( !validObSaveBits( err, q, ip ) )
            return false;
    }

    if( !validNiDevices( err, q )
        || !validNiClock( err, q )
        || !validNiChannels( err, q,
                vcMN1, vcMA1, vcXA1, vcXD1,
                vcMN2, vcMA2, vcXA2, vcXD2,
                uiStr1Err, uiStr2Err )
        || !validNiShankMap( err, q )
        || !validNiChanMap( err, q )
        || !validNiSaveBits( err, q ) ) {

        return false;
    }

    if( !validSyncTab( err, q ) )
        return false;

    if( q.stream_isIM( q.trigStream() ) && !validImTriggering( err, q ) )
        return false;

    if( q.stream_isOB( q.trigStream() ) && !validObTriggering( err, q ) )
        return false;

    if( q.stream_isNI( q.trigStream() ) && !validNiTriggering( err, q ) )
        return false;

    if( !validTrgPeriEvent( err, q ) )
        return false;

    if( !validTrgLowTime( err, q ) )
        return false;

    if( !validDataDir( err ) )
        return false;

    if( !validDiskAvail( err, q ) )
        return false;

    if( !validRunName( err, q, q.sns.runName, parent ) )
        return false;

// --------------------------
// Warn about ColorTTL issues
// --------------------------

    ColorTTLCtl TTLCC( parent, q );
    QString     ccErr;

    if( !TTLCC.valid( ccErr ) ) {

        if( parent ) {

            QMessageBox::warning( parent,
                "Conflicts Detected With ColorTTL Events",
                QString(
                "Issues detected:\n%1\n\n"
                "Fix either the ColorTTL or the run settings...")
                .arg( ccErr ) );

            TTLCC.showDialog();

            int yesNo = QMessageBox::question(
                parent,
                "Run (or Verify/Save) Now?",
                "[Yes] = settings are ready to go,\n"
                "[No]  = let me edit run settings.",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes );

            if( yesNo != QMessageBox::Yes )
                return false;
        }
        else {
            err = QString("ColorTTL conflicts [%1].").arg( ccErr );
            return false;
        }
    }

// -------------
// Accept params
// -------------

    if( iprb < 0 ) {

        validated = true;
        setParams( q, true );
        prbTab.saveProbeTable();

        if( usingOB )
            obxTab->saveSettings();
    }
    else
        running_setProbe( q, iprb );

    if( usingIM )
        imTab->saveSettings();

// Update AO dialog
    mainApp()->getAOCtl()->reset();

    return true;
}


void ConfigCtl::running_setProbe( DAQ::Params &q, int ip )
{
    Run             *run    = mainApp()->getRun();
    CimCfg::PrbEach &E0     = acceptedParams.im.prbj[ip],
                    &E      = q.im.prbj[ip];

    bool    I = !E.roTbl->isConnectedSame( E0.roTbl ),
            S = E.stdbyBits != E0.stdbyBits,
            C = E.sns.chanMapFile != E0.sns.chanMapFile;

    run->grfHardPause( true );
    run->grfWaitPaused();
        E0 = E;
    run->grfHardPause( false );

    imTab->updateProbe( E, ip );
    imTab->saveSettings();

    if( I || S || C ) {

        if( I || S )
            run->imecUpdate( ip );

        run->grfUpdateProbe( ip, I || S, I || C );
    }
}


