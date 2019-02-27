
#include "Util.h"
#include "DAQ.h"

#include <QSettings>


#define STRMATCH( s, target ) !(s).compare( target, Qt::CaseInsensitive )




namespace DAQ
{

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static const QString gateModeStrs[] = {
        "Immediate", "TCP",
        QString::null
};

static const QString trigModeStrs[] = {
        "Immediate", "Timed", "TTL", "Spike", "TCP",
        QString::null
};

static const QString unk = "Unknown";






/* ---------------------------------------------------------------- */
/* DOParams ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Derive from persistent settings:
//
void DOParams::deriveDOParams()
{
}

/* ---------------------------------------------------------------- */
/* Params --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Stream strings: "nidq", "imec", "imec0", ..., "imec39", ...
//
// Return int starting at position 4, or zero if none.
//
int Params::streamID( const QString &stream )
{
    return stream.rightRef( stream.size() - 4 ).toInt();
}


// Return trigger stream or null.
//
QString Params::trigStream() const
{
    if( mode.mTrig == eTrigTTL )
        return trgTTL.stream;

    if( mode.mTrig == eTrigSpike )
        return trgSpike.stream;

    return QString::null;
}


// Return current trigger analog threshold as integer or zero.
//
int Params::trigThreshAsInt() const
{
    if( mode.mTrig == eTrigTTL ) {

        if( trgTTL.stream == "nidq" )
            return ni.vToInt16( trgTTL.T, trgTTL.chan );
        else {
            return im.vToInt10( trgTTL.T, streamID( trgTTL.stream ),
                    trgTTL.chan );
        }
    }

    if( mode.mTrig == eTrigSpike ) {

        if( trgSpike.stream == "nidq" )
            return ni.vToInt16( trgSpike.T, trgSpike.aiChan );
        else {
            return im.vToInt10( trgSpike.T, streamID( trgSpike.stream ),
                    trgSpike.aiChan );
        }
    }

    return 0;
}


// Return trigger channel or -1.
//
int Params::trigChan() const
{
    if( mode.mTrig == eTrigTTL ) {

        if( trgTTL.isAnalog )
            return trgTTL.chan;
        else if( trgTTL.stream == "nidq" )
            return ni.niCumTypCnt[CniCfg::niSumAnalog] + trgTTL.bit/16;
        else {
// MS: Analog and digital aux may be redefined in phase 3B2
            return im.each[streamID( trgTTL.stream )]
                    .imCumTypCnt[CimCfg::imSumNeural];
        }
    }

    if( mode.mTrig == eTrigSpike )
        return trgSpike.aiChan;

    return -1;
}


void Params::loadSettings( bool remote )
{
    QString fn = QString("daq%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, fn );
    settings.beginGroup( "DAQSettings" );

// ----
// Nidq
// ----

    ni.loadSettings( settings );

// ----------
// SyncParams
// ----------

    sync.sourcePeriod =
    settings.value( "syncSourcePeriod", 1.0 ).toDouble();

    sync.sourceIdx = (SyncSource)
    settings.value( "syncSourceIdx", 0 ).toInt();

    sync.niThresh =
    settings.value( "syncNiThresh", 1.1 ).toDouble();

    sync.imInputSlot =
    settings.value( "syncImInputSlot", 2 ).toInt();

    sync.niChanType =
    settings.value( "syncNiChanType", 1 ).toInt();

    sync.niChan =
    settings.value( "syncNiChan", 224 ).toInt();

    sync.calMins =
    settings.value( "syncCalMins", 10 ).toInt();

    sync.isCalRun = false;

// --------
// DOParams
// --------

// ------------
// TrgTimParams
// ------------

    trgTim.tL0 =
    settings.value( "trgTimTL0", 10.0 ).toDouble();

    trgTim.tH =
    settings.value( "trgTimTH", 10.0 ).toDouble();

    trgTim.tL =
    settings.value( "trgTimTL", 1.0 ).toDouble();

    trgTim.nH =
    settings.value( "trgTimNH", 3 ).toUInt();

    trgTim.isHInf =
    settings.value( "trgTimIsHInf", false ).toBool();

    trgTim.isNInf =
    settings.value( "trgTimIsNInf", false ).toBool();

// ------------
// TrgTTLParams
// ------------

    trgTTL.T =
    settings.value( "trgTTLThresh", 1.1 ).toDouble();

    trgTTL.marginSecs =
    settings.value( "trgTTLMarginS", 1.0 ).toDouble();

    trgTTL.refractSecs =
    settings.value( "trgTTLRefractS", 0.5 ).toDouble();

    trgTTL.tH =
    settings.value( "trgTTLTH", 0.5 ).toDouble();

    trgTTL.stream =
    settings.value( "trgTTLStream", "nidq" ).toString();

    trgTTL.mode =
    settings.value( "trgTTLMode", 0 ).toInt();

    trgTTL.chan =
    settings.value( "trgTTLAIChan", 4 ).toInt();

    trgTTL.bit =
    settings.value( "trgTTLBit", 0 ).toInt();

    trgTTL.inarow =
    settings.value( "trgTTLInarow", 5 ).toUInt();

    trgTTL.nH =
    settings.value( "trgTTLNH", 10 ).toUInt();

    trgTTL.isAnalog =
    settings.value( "trgTTLIsAnalog", true ).toBool();

    trgTTL.isNInf =
    settings.value( "trgTTLIsNInf", true ).toBool();

// --------------
// TrgSpikeParams
// --------------

    trgSpike.T =
    settings.value( "trgSpikeThresh", -100e-6 ).toDouble();

    trgSpike.periEvtSecs =
    settings.value( "trgSpikePeriEvtS", 1.0 ).toDouble();

    trgSpike.refractSecs =
    settings.value( "trgSpikeRefractS", 0.5 ).toDouble();

    trgSpike.stream =
    settings.value( "trgSpikeStream", "nidq" ).toString();

    trgSpike.aiChan =
    settings.value( "trgSpikeAIChan", 4 ).toInt();

    trgSpike.inarow =
    settings.value( "trgSpikeInarow", 5 ).toUInt();

    trgSpike.nS =
    settings.value( "trgSpikeNS", 10 ).toUInt();

    trgSpike.isNInf =
    settings.value( "trgSpikeIsNInf", false ).toBool();

// ----------
// ModeParams
// ----------

    mode.mGate = (GateMode)
    settings.value( "gateMode", 0 ).toInt();

    mode.mTrig = (TrigMode)
    settings.value( "trigMode", 0 ).toInt();

    mode.manOvShowBut =
    settings.value( "manOvShowBut", true ).toBool();

    mode.manOvInitOff =
    settings.value( "manOvInitOff", true ).toBool();

    if( !mode.manOvShowBut )
        mode.manOvInitOff = false;

// --------
// SeeNSave
// --------

    sns.notes =
    settings.value( "snsNotes", "" ).toString();

    sns.runName =
    settings.value( "snsRunName", "myRun" ).toString();

    sns.reqMins =
    settings.value( "snsReqMins", 10 ).toInt();

    sns.pairChk =
    settings.value( "snsPairChk", true ).toBool();

    sns.fldPerPrb =
    settings.value( "snsFldPerProbe", true ).toBool();

    settings.endGroup();

// ----
// Imec
// ----

    im.loadSettings( settings );
}


void Params::saveSettings( bool remote ) const
{
    QString fn = QString("daq%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, fn );
    settings.beginGroup( "DAQSettings" );

// ----
// Nidq
// ----

    ni.saveSettings( settings );

// ----------
// SyncParams
// ----------

    settings.setValue( "syncSourcePeriod", sync.sourcePeriod );
    settings.setValue( "syncSourceIdx", (int)sync.sourceIdx );

    settings.setValue( "syncNiThresh", sync.niThresh );
    settings.setValue( "syncImInputSlot", sync.imInputSlot );
    settings.setValue( "syncNiChanType", sync.niChanType );
    settings.setValue( "syncNiChan", sync.niChan );

    settings.setValue( "syncCalMins", sync.calMins );

// --------
// DOParams
// --------

// ------------
// TrgTimParams
// ------------

    settings.setValue( "trgTimTL0", trgTim.tL0 );
    settings.setValue( "trgTimTH", trgTim.tH );
    settings.setValue( "trgTimTL", trgTim.tL );
    settings.setValue( "trgTimNH", trgTim.nH );
    settings.setValue( "trgTimIsHInf", trgTim.isHInf );
    settings.setValue( "trgTimIsNInf", trgTim.isNInf );

// ------------
// TrgTTLParams
// ------------

    settings.setValue( "trgTTLThresh", trgTTL.T );
    settings.setValue( "trgTTLMarginS", trgTTL.marginSecs );
    settings.setValue( "trgTTLRefractS", trgTTL.refractSecs );
    settings.setValue( "trgTTLTH", trgTTL.tH );
    settings.setValue( "trgTTLStream", trgTTL.stream );
    settings.setValue( "trgTTLMode", trgTTL.mode );
    settings.setValue( "trgTTLAIChan", trgTTL.chan );
    settings.setValue( "trgTTLBit", trgTTL.bit );
    settings.setValue( "trgTTLInarow", trgTTL.inarow );
    settings.setValue( "trgTTLNH", trgTTL.nH );
    settings.setValue( "trgTTLIsAnalog", trgTTL.isAnalog );
    settings.setValue( "trgTTLIsNInf", trgTTL.isNInf );

// --------------
// TrgSpikeParams
// --------------

    settings.setValue( "trgSpikeThresh", trgSpike.T );
    settings.setValue( "trgSpikePeriEvtS", trgSpike.periEvtSecs );
    settings.setValue( "trgSpikeRefractS", trgSpike.refractSecs );
    settings.setValue( "trgSpikeStream", trgSpike.stream );
    settings.setValue( "trgSpikeAIChan", trgSpike.aiChan );
    settings.setValue( "trgSpikeInarow", trgSpike.inarow );
    settings.setValue( "trgSpikeNS", trgSpike.nS );
    settings.setValue( "trgSpikeIsNInf", trgSpike.isNInf );

// ----------
// ModeParams
// ----------

    settings.setValue( "gateMode", (int)mode.mGate );
    settings.setValue( "trigMode", (int)mode.mTrig );
    settings.setValue( "manOvShowBut", mode.manOvShowBut );

    if( mode.manOvShowBut )
        settings.setValue( "manOvInitOff", mode.manOvInitOff );
    else
        settings.setValue( "manOvInitOff", false );

// --------
// SeeNSave
// --------

    settings.setValue( "snsNotes", sns.notes );
    settings.setValue( "snsRunName", sns.runName );
    settings.setValue( "snsReqMins", sns.reqMins );
    settings.setValue( "snsPairChk", sns.pairChk );
    settings.setValue( "snsFldPerProbe", sns.fldPerPrb );

    settings.endGroup();

// ----
// Imec
// ----

    im.saveSettings( settings );
}


// MS: Work needed to give MATLAB access to imec settings subgroups
QString Params::settings2Str()
{
    STDSETTINGS( settings, "daq" );
    settings.beginGroup( "DAQSettings" );

    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    QStringList keys = settings.childKeys();

    foreach( const QString &key, keys )
        ts << key << "=" << settings.value( key ).toString() << "\n";

    return s;
}


// MS: Work needed to give MATLAB access to imec settings subgroups
void Params::str2RemoteSettings( const QString &str )
{
    STDSETTINGS( settings, "daqremote" );
    settings.beginGroup( "DAQSettings" );

    QTextStream ts( str.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString     line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > -1 && eq < line.length() ) {

            QString k = line.left( eq ).trimmed(),
                    v = line.mid( eq + 1 ).trimmed();

            settings.setValue( k, v );
        }
    }
}

/* ---------------------------------------------------------------- */
/* gateModeToString ----------------------------------------------- */
/* ---------------------------------------------------------------- */

const QString& gateModeToString( GateMode gateMode )
{
    if( gateMode >= 0 && gateMode < N_gateModes )
        return gateModeStrs[gateMode];

    return unk;
}

/* ---------------------------------------------------------------- */
/* stringToGateMode ----------------------------------------------- */
/* ---------------------------------------------------------------- */

GateMode stringToGateMode( const QString &str )
{
    QString s = str.trimmed();
    int     m;

    for( m = 0; m < N_gateModes; ++m ) {

        if( STRMATCH( s, gateModeStrs[m] ) )
            break;
    }

    return (GateMode)m;
}

/* ---------------------------------------------------------------- */
/* trigModeToString ----------------------------------------------- */
/* ---------------------------------------------------------------- */

const QString& trigModeToString( TrigMode trigMode )
{
    if( trigMode >= 0 && trigMode < N_trigModes )
        return trigModeStrs[trigMode];

    return unk;
}

/* ---------------------------------------------------------------- */
/* stringToTrigMode ----------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigMode stringToTrigMode( const QString &str )
{
    QString s = str.trimmed();
    int     m;

    for( m = 0; m < N_trigModes; ++m ) {

        if( STRMATCH( s, trigModeStrs[m] ) )
            break;
    }

    return (TrigMode)m;
}

}   // namespace DAQ


