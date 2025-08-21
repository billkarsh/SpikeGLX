
#include "ui_NITab.h"
#include "ui_NISourceDlg.h"

#include "Util.h"
#include "ConfigCtl.h"
#include "Config_nitab.h"
#include "ChanMapCtl.h"
#include "ShankMapCtl.h"
#include "HelpButDialog.h"
#include "Subset.h"

#include <math.h>

#include <QMessageBox>
#include <QSharedMemory>

#define CURDEV1 niTabUI->device1CB->currentIndex()
#define CURDEV2 niTabUI->device2CB->currentIndex()

static const char *DEF_NISKMP_LE = "*Default (1 shk, 2 col, tip=[0,0])";
static const char *DEF_NICHMP_LE = "*Default (acquired channel order)";


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_nitab::Config_nitab( ConfigCtl *cfg, QWidget *tab )
    :   QObject(0), niTabUI(0), cfg(cfg), singleton(0)
{
    niTabUI = new Ui::NITab;
    niTabUI->setupUi( tab );
    ConnectUI( niTabUI->device1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(device1CBChanged()) );
    ConnectUI( niTabUI->device2CB, SIGNAL(currentIndexChanged(int)), this, SLOT(device2CBChanged()) );
    ConnectUI( niTabUI->mn1LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->ma1LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->mn2LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->ma2LE, SIGNAL(textChanged(QString)), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->dev2GB, SIGNAL(clicked()), this, SLOT(muxingChanged()) );
    ConnectUI( niTabUI->clkSourceCB, SIGNAL(currentIndexChanged(int)), this, SLOT(clkSourceCBChanged()) );
    ConnectUI( niTabUI->newSourceBut, SIGNAL(clicked()), this, SLOT(newSourceBut()) );
    ConnectUI( niTabUI->startEnabChk, SIGNAL(clicked(bool)), this, SLOT(startEnableClicked(bool)) );
    ConnectUI( niTabUI->niShkMapBut, SIGNAL(clicked()), this, SLOT(niShkMapBut()) );
    ConnectUI( niTabUI->niChnMapBut, SIGNAL(clicked()), this, SLOT(niChnMapBut()) );
}


Config_nitab::~Config_nitab()
{
    singletonRelease();

    if( niTabUI ) {
        delete niTabUI;
        niTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_nitab::toGUI( const DAQ::Params &p )
{
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
        int sel     = 0,
            sel2    = 0;

        foreach( const QString &D, CniCfg::niDevNames ) {

            if( CniCfg::aiDevChanCount.contains( D ) ||
                CniCfg::diDevLineCount.contains( D ) ) {

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

// Clock source

    p.ni.fillSRateCB( niTabUI->clkSourceCB, p.ni.clockSource );

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

// shankMap

    QString s = p.ni.sns.shankMapFile;

    if( s.contains( "*" ) )
        s.clear();

    if( s.isEmpty() )
        niTabUI->niShkMapLE->setText( DEF_NISKMP_LE );
    else
        niTabUI->niShkMapLE->setText( s );

// chanMap

    s = p.ni.sns.chanMapFile;

    if( s.contains( "*" ) )
        s.clear();

    if( s.isEmpty() )
        niTabUI->niChnMapLE->setText( DEF_NICHMP_LE );
    else
        niTabUI->niChnMapLE->setText( s );

// save chans

    niTabUI->niSaveChansLE->setText( p.ni.sns.uiSaveChanStr );

// --------------------
// Observe dependencies
// --------------------

    device1CBChanged(); // <-- Call This First!! - Fills in other CBs
    device2CBChanged();
    muxingChanged();
    clkSourceCBChanged();
    startEnableClicked( p.ni.startEnable );
}


void Config_nitab::fromGUI(
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

    q.ni.loadSRateTable();
    q.ni.srate  = niTabUI->niRateSB->value();

    q.ni.dev1 =
    (niTabUI->device1CB->count() ? devNames[CURDEV1] : "");

    q.ni.dev2 =
    (niTabUI->device2CB->count() ? devNames[CURDEV2] : "");

    if( niTabUI->device1CB->count() ) {

        QList<VRange>   rngL = CniCfg::aiDevRanges.values( q.ni.dev1 );

        if( rngL.size() && niTabUI->aiRangeCB->currentIndex() >= 0 )
            q.ni.range = rngL[niTabUI->aiRangeCB->currentIndex()];
    }

    q.ni.clockLine1     = niTabUI->clkLine1CB->currentText();
    q.ni.clockLine2     = niTabUI->clkLine2CB->currentText();
    q.ni.clockSource    = niTabUI->clkSourceCB->currentText();
    q.ni.settle         = cfg->acceptedParams.ni.settle;
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

    q.ni.enabled        = true;
    q.ni.isDualDevMode  = niTabUI->dev2GB->isChecked();
    q.ni.startEnable    = niTabUI->startEnabChk->isChecked();

    q.ni.sns.shankMapFile   = niTabUI->niShkMapLE->text().trimmed();
    q.ni.sns.chanMapFile    = niTabUI->niChnMapLE->text().trimmed();
    q.ni.sns.uiSaveChanStr  = niTabUI->niSaveChansLE->text().trimmed();
}


// Return true if this app instance owns NI hardware.
//
bool Config_nitab::singletonReserve()
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


void Config_nitab::singletonRelease()
{
    if( singleton ) {
        delete singleton;
        singleton = 0;
    }
}


const QString &Config_nitab::curDevName() const
{
    return devNames[CURDEV1];
}


bool Config_nitab::isMuxingFromDlg() const
{
    return  !niTabUI->mn1LE->text().trimmed().isEmpty()
            || !niTabUI->ma1LE->text().trimmed().isEmpty()
            || (niTabUI->dev2GB->isChecked()
                && (!niTabUI->mn2LE->text().trimmed().isEmpty()
                    || !niTabUI->ma2LE->text().trimmed().isEmpty())
                );
}


int Config_nitab::nSources() const
{
    return niTabUI->clkSourceCB->count();
}


// With a dialog, user is constrained to choose items
// we've put into CB controls. Remote case lacks that
// constraint, so we check existence of CB items here.
//
bool Config_nitab::remoteValidate( QString &err, const DAQ::Params &p )
{
    err.clear();

    if( p.ni.dev1 != devNames[CURDEV1] ) {

        err = QString("Device [%1] does not support AI.")
                .arg( p.ni.dev1 );
        return false;
    }

    if( p.ni.dev2 != devNames[CURDEV2] ) {

        err = QString("Device [%1] does not support AI.")
                .arg( p.ni.dev2 );
        return false;
    }

    if( p.ni.clockLine1 != niTabUI->clkLine1CB->currentText() ) {

        err = QString("Clock [%1] not supported on device [%2].")
                .arg( p.ni.clockLine1 )
                .arg( p.ni.dev1 );
        return false;
    }

    if( p.ni.clockLine2 != niTabUI->clkLine2CB->currentText() ) {

        err = QString("Clock [%1] not supported on device [%2].")
                .arg( p.ni.clockLine2 )
                .arg( p.ni.dev2 );
        return false;
    }

    QString rng = QString("[%1, %2]")
                    .arg( p.ni.range.rmin )
                    .arg( p.ni.range.rmax );

    if( rng != niTabUI->aiRangeCB->currentText() ) {

        err = QString("Range %1 not supported on device [%2].")
                .arg( rng )
                .arg( p.ni.dev1 );
        return false;
    }

    if( p.ni.clockSource != niTabUI->clkSourceCB->currentText() ) {

        err = QString("Clock source [%1] not defined.")
                .arg( p.ni.clockSource );
        return false;
    }

    if( p.ni.startLine != niTabUI->startCB->currentText() ) {

        err = QString("Start line [%1] not defined.")
                .arg( p.ni.startLine );
        return false;
    }

    return true;
}


/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_nitab::device1CBChanged()
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
        niTabUI->clkLine1CB->clear();
        niTabUI->clkLine1CB->addItem( "Internal" );

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

            niTabUI->clkLine1CB->addItem( s );

            if( s == cfg->acceptedParams.ni.clockLine1 )
                pfiSel = i + 1;
        }

        niTabUI->clkLine1CB->setCurrentIndex( pfiSel );
    }

// --------------------
// Observe dependencies
// --------------------

    muxingChanged();
    cfg->syncNiDevChanged();
}


// Note:
// 2nd device should not offer internal as a clock choice.
// Rather, it is a slave, intended to augment the primary.
//
void Config_nitab::device2CBChanged()
{
// --------------------
// Set up Dev2 clock CB
// --------------------

    niTabUI->clkLine2CB->clear();

    if( !niTabUI->device2CB->count() ) {

noPFI:
        niTabUI->clkLine2CB->addItem( "PFI2" );
        niTabUI->clkLine2CB->setCurrentIndex( 0 );
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

        niTabUI->clkLine2CB->addItem( s );

        if( s == cfg->acceptedParams.ni.clockLine2 )
            pfiSel = i;
    }

    niTabUI->clkLine2CB->setCurrentIndex( pfiSel );
}


void Config_nitab::muxingChanged()
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

        ci = niTabUI->clkLine1CB->findText( "PFI2", Qt::MatchExactly );
        niTabUI->clkLine1CB->setCurrentIndex( ci > -1 ? ci : 0 );

        ci = niTabUI->clkLine2CB->findText( "PFI2", Qt::MatchExactly );
        niTabUI->clkLine2CB->setCurrentIndex( ci > -1 ? ci : 0 );

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

        bool    wasMux = cfg->acceptedParams.ni.isMuxingMode();

        if( wasMux )
            niTabUI->niSaveChansLE->setText( "all" );
    }

    niTabUI->clkLine1CB->setDisabled( isMux );
    niTabUI->clkLine2CB->setDisabled( isMux || !niTabUI->dev2GB->isChecked() );
    niTabUI->startEnabChk->setDisabled( isMux );
    niTabUI->startCB->setDisabled( isMux );
}


void Config_nitab::clkSourceCBChanged()
{
    niTabUI->niRateSB->setValue(
        cfg->acceptedParams.ni.getSRate(
            niTabUI->clkSourceCB->currentText() ) );
}


void Config_nitab::startEnableClicked( bool checked )
{
    niTabUI->startCB->setEnabled( checked && !isMuxingFromDlg() );
}


void Config_nitab::niShkMapBut()
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

    ShankMapCtl  SM( cfg->dialog(), 0, "nidq", ni.niCumTypCnt[CniCfg::niTypeMN] );

    QString mapFile = SM.edit( niTabUI->niShkMapLE->text().trimmed() );

    if( mapFile.isEmpty() )
        niTabUI->niShkMapLE->setText( DEF_NISKMP_LE );
    else
        niTabUI->niShkMapLE->setText( mapFile );
}


void Config_nitab::niChnMapBut()
{
// ---------------------------------------
// Calculate channel usage from current UI
// ---------------------------------------

    CniCfg  ni;

    if( !niChannelsFromDialog( ni ) )
        return;

    const int   *cum    = ni.niCumTypCnt;
    ChanMapNI   defMap(
        cum[CniCfg::niTypeMN] / ni.muxFactor,
        (cum[CniCfg::niTypeMA] - cum[CniCfg::niTypeMN]) / ni.muxFactor,
        ni.muxFactor,
        cum[CniCfg::niTypeXA] - cum[CniCfg::niTypeMA],
        cum[CniCfg::niTypeXD] - cum[CniCfg::niTypeXA] );

// -------------
// Launch editor
// -------------

    ChanMapCtl  CM( cfg->dialog(), defMap );

    QString mapFile = CM.edit(
                        niTabUI->niChnMapLE->text().trimmed(),
                        -1 );

    if( mapFile.isEmpty() )
        niTabUI->niChnMapLE->setText( DEF_NICHMP_LE );
    else
        niTabUI->niChnMapLE->setText( mapFile );
}


void Config_nitab::newSourceBut()
{
// ------------
// Get settings
// ------------

    nisrc.dev       = devNames[CURDEV1];
    nisrc.devType   = CniCfg::devType( nisrc.dev );
    nisrc.base      = CniCfg::maxTimebase( nisrc.dev );

    if( CniCfg::isDigitalDev( nisrc.dev ) ) {

        nisrc.R0        = CniCfg::maxSampleRate( nisrc.dev, 1 );
        nisrc.maxrate   = nisrc.R0;
        nisrc.settle    = 0;
        nisrc.saferate  = nisrc.R0;
        nisrc.nAI       = 0;
        nisrc.simsam    = false;

        if( niTabUI->mn1LE->text().trimmed().size() ||
            niTabUI->ma1LE->text().trimmed().size() ||
            niTabUI->xa1LE->text().trimmed().size() ) {

            QMessageBox::critical(
                cfg->dialog(),
                "Incompatible Channel Type",
                "All analog channels must be left blank"
                " because the selected device is digital." );
            return;
        }
    }
    else if( (nisrc.simsam =
            CniCfg::supportsAISimultaneousSampling( nisrc.dev )) ) {

        nisrc.R0        = CniCfg::maxSampleRate( nisrc.dev, 1 );
        nisrc.maxrate   = nisrc.R0;
        nisrc.settle    = 0;
        nisrc.saferate  = nisrc.R0;
        nisrc.nAI       = 1;
    }
    else {

        QVector<uint>   vcMN1, vcMA1, vcXA1, vcXD1;

        if( !Subset::rngStr2Vec( vcMN1, niTabUI->mn1LE->text().trimmed() )
            || !Subset::rngStr2Vec( vcMA1, niTabUI->ma1LE->text().trimmed() )
            || !Subset::rngStr2Vec( vcXA1, niTabUI->xa1LE->text().trimmed() )
            || !Subset::rngStr2Vec( vcXD1, niTabUI->xd1LE->text().trimmed() ) ) {

            QMessageBox::critical(
                cfg->dialog(),
                "ChanMap Parameter Error",
                "Bad format in one or more device (1) channel strings." );
            return;
        }

        if( vcMN1.size() || vcMA1.size() ) {

            QMessageBox::critical(
                cfg->dialog(),
                "Incompatible Channel Type",
                "MN and MA (multiplexed) channels must be left blank"
                " because the selected device does not support"
                " simultaneous sampling." );
            return;
        }

        nisrc.nAI = vcXA1.size();

        if( !nisrc.nAI && !vcXD1.size() ) {

            QMessageBox::critical(
                cfg->dialog(),
                "No Channels named",
                "Specify your XA and XD channels before"
                " using the New Source dialog." );
            return;
        }

        nisrc.R0 = CniCfg::maxSampleRate(
                    nisrc.dev,
                    (nisrc.nAI > 1 ? -nisrc.nAI : -1) );

        nisrc.maxrate   = nisrc.R0;
        nisrc.settle    = cfg->acceptedParams.ni.settle;

        if( !nisrc.nAI )
            nisrc.saferate  = nisrc.R0;
        else {
            nisrc.saferate  = 1.0/(1.0/nisrc.R0 + 1.4e-6 * nisrc.settle);
            nisrc.maxrate  /= nisrc.nAI;
            nisrc.saferate /= nisrc.nAI;
        }
    }

    nisrc.exttrig = (niTabUI->clkLine1CB->currentIndex() > 0);

// -------------
// Set up dialog
// -------------

    HelpButDialog   D( "NIClock_Help" );

    sourceUI = new Ui::NISourceDlg;
    sourceUI->setupUi( &D );
    sourceUI->simsamLE->setText( nisrc.simsam ? "Y" : "N" );
    sourceUI->nchanLE->setText(
        (nisrc.simsam || nisrc.nAI < 1) ? "NA" :
        QString("%1 (based on XA, XD)").arg( qMax( nisrc.nAI, 1 ) ) );
    sourceUI->maxrateLE->setText(
        QString("%1").arg( nisrc.maxrate ) );
    sourceUI->settleSB->setValue(
        (nisrc.simsam || nisrc.nAI < 1) ? 0 : cfg->acceptedParams.ni.settle );
    sourceUI->saferateLE->setText(
        QString("%1").arg( nisrc.saferate ) );

    sourceUI->baseLE->setText( QString::number( nisrc.base ) );

    if( nisrc.simsam || nisrc.nAI < 1 )
        sourceUI->settleSB->setEnabled( false );
    else {
        sourceUI->settleSB->setEnabled( true );
        ConnectUI( sourceUI->settleSB, SIGNAL(valueChanged(double)), this, SLOT(sourceSettleChanged()) );
    }

    if( nisrc.exttrig ) {
        sourceUI->maxRadio->setEnabled( false );
        sourceUI->safeRadio->setEnabled( false );
        sourceUI->intRadio->setEnabled( false );
    }
    else {
        sourceUI->extRadio->setEnabled( false );
        sourceUI->whisperRadio->setEnabled( false );
        sourceUI->rateSB->setEnabled( false );

        sourceUI->divSB->setMinimum(
            ceil( nisrc.base/qMin( nisrc.maxrate, SGLX_NI_MAXRATE ) ) );
        sourceUI->divSB->setMaximum( nisrc.base / SGLX_NI_MINRATE );
        sourceUI->intRadio->setText(
            QString("Other internal (set integer divisor [%1, %2])")
                .arg( sourceUI->divSB->minimum() )
                .arg( sourceUI->divSB->maximum() ) );
        Connect( sourceUI->divSB, SIGNAL(valueChanged(int)), this, SLOT(sourceDivChanged(int)), Qt::DirectConnection );
    }

    sourceUI->rateSB->setMinimum( SGLX_NI_MINRATE );
    sourceUI->rateSB->setMaximum( qMin( nisrc.maxrate, SGLX_NI_MAXRATE ) );

    ConnectUI( sourceUI->maxRadio, SIGNAL(clicked()), this, SLOT(sourceMaxChecked()) );
    ConnectUI( sourceUI->safeRadio, SIGNAL(clicked()), this, SLOT(sourceSafeChecked()) );
    ConnectUI( sourceUI->intRadio, SIGNAL(clicked()), this, SLOT(sourceEnabItems()) );
    ConnectUI( sourceUI->extRadio, SIGNAL(clicked()), this, SLOT(sourceEnabItems()) );
    ConnectUI( sourceUI->whisperRadio, SIGNAL(clicked()), this, SLOT(sourceWhisperChecked()) );

    if( isMuxingFromDlg() ) {
        sourceUI->whisperRadio->setChecked( true );
        sourceUI->whisperRadio->setFocus();
        sourceWhisperChecked(); // consequences for that
    }
    else if( nisrc.exttrig ) {
        sourceUI->extRadio->setChecked( true );
        sourceUI->extRadio->setFocus();
        sourceSafeChecked();    // consequences for that
    }
    else {
        sourceUI->safeRadio->setChecked( true );
        sourceUI->safeRadio->setFocus();
        sourceSafeChecked();    // consequences for that
    }

redo:
    if( QDialog::Accepted == D.exec() ) {

        double  rate    = sourceUI->rateSB->value();
        QString key     = sourceUI->nameLE->text().trimmed();
        int     div     = sourceUI->divSB->value();

        if( key.isEmpty() ) {

            QMessageBox::critical(
                &D,
                "Empty Name",
                "Enter a device name, e.g.: Whisper or Internal." );
            goto redo;
        }

        if( key.contains( ":" ) ) {

            QMessageBox::critical(
                &D,
                "Name Error",
                "Device name cannot contain a colon ':'." );
            goto redo;
        }

        key = QString("%1 : %2").arg( key ).arg( rate, 0, 'f', 6 );

        cfg->acceptedParams.ni.newSRate( key, rate, div, nisrc.devType );
        cfg->acceptedParams.ni.fillSRateCB( niTabUI->clkSourceCB, key );
        cfg->acceptedParams.ni.saveSRateTable();
        clkSourceCBChanged();

        if( !nisrc.simsam && nisrc.nAI >= 1 ) {
            cfg->acceptedParams.ni.settle = nisrc.settle;
            cfg->acceptedParams.saveSettings();
        }
    }

    guiBreathe();   // allow dialog messages to complete
    guiBreathe();
    guiBreathe();

    delete sourceUI;
}


void Config_nitab::sourceSettleChanged()
{
    if( nisrc.simsam || nisrc.nAI < 1 )
        return;

    nisrc.settle    = int(100.0*sourceUI->settleSB->value()+0.5)/100.0;
    nisrc.saferate  = 1.0/(1.0/nisrc.R0 + 1.4e-6 * nisrc.settle);
    nisrc.saferate /= nisrc.nAI;

    sourceUI->saferateLE->setText( QString("%1").arg( nisrc.saferate ) );

    if( sourceUI->safeRadio->isChecked() )
        sourceSafeChecked();
}


void Config_nitab::sourceMaxChecked()
{
    if( nisrc.exttrig )
        sourceUI->rateSB->setValue( nisrc.maxrate );
    else {
        sourceSetDiv(
            ceil( nisrc.base/qMin( nisrc.maxrate, SGLX_NI_MAXRATE ) ) );
    }

    sourceEnabItems();
}


void Config_nitab::sourceSafeChecked()
{
    if( nisrc.exttrig )
        sourceUI->rateSB->setValue( nisrc.saferate );
    else {
        sourceSetDiv(
            ceil( nisrc.base/qMin( nisrc.saferate, SGLX_NI_MAXRATE ) ) );
    }

    sourceEnabItems();
}


void Config_nitab::sourceWhisperChecked()
{
    sourceUI->rateSB->setValue( 25000.0 );

    sourceEnabItems();
}


void Config_nitab::sourceEnabItems()
{
    bool    enab = sourceUI->intRadio->isChecked();

    sourceUI->baseLbl->setEnabled( enab );
    sourceUI->baseLE->setEnabled( enab );
    sourceUI->divLbl->setEnabled( enab );
    sourceUI->divSB->setEnabled( enab );

    if( enab )
        sourceSetDiv( sourceUI->divSB->value() );
    else
        sourceMakeName();

    sourceUI->rateSB->setEnabled( sourceUI->extRadio->isChecked() );
}


void Config_nitab::sourceDivChanged( int i )
{
    sourceUI->rateSB->setValue( nisrc.base / i );
    sourceMakeName();
}


void Config_nitab::sourceSetDiv( int i )
{
    sourceUI->divSB->setValue( i );
    sourceDivChanged( i );
}


void Config_nitab::sourceMakeName()
{
    QString name = nisrc.devType;

    if( !nisrc.simsam ) {

        if( !nisrc.nAI )
            name += "_dig";
        else
            name += QString("_%1ch").arg( nisrc.nAI );
    }

    if( nisrc.exttrig ) {

        if( sourceUI->whisperRadio->isChecked() )
            name += "_whisper";
        else
            name += "_ext";
    }
    else if( sourceUI->maxRadio->isChecked() )
        name += "_IntMax";
    else if( sourceUI->safeRadio->isChecked() )
        name += "_IntSafe";
    else
        name += "_Int";

    sourceUI->nameLE->setText( name );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_nitab::setupNiVRangeCB()
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

        QRegularExpression re("\\[.*, (.*)\\]");

        if( CB->currentText().contains( re ) )
            targV = re.match( CB->currentText() ).captured(1).toDouble();
    }
    else
        targV = cfg->acceptedParams.ni.range.rmax;

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


QString Config_nitab::uiMNStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->mn2LE->text().trimmed() : "");
}


QString Config_nitab::uiMAStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->ma2LE->text().trimmed() : "");
}


QString Config_nitab::uiXAStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->xa2LE->text().trimmed() : "");
}


QString Config_nitab::uiXDStr2FromDlg() const
{
    return (niTabUI->dev2GB->isChecked() ?
            niTabUI->xd2LE->text().trimmed() : "");
}


bool Config_nitab::niChannelsFromDialog( CniCfg &ni ) const
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
            cfg->dialog(),
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


