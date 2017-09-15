
#include "Util.h"
#include "DAQ.h"
#include "Subset.h"

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
/* SnsChansBase --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Derive from persistent settings:
//
// saveBits
//
// Return true if uiSaveChanStr format OK.
//
bool SnsChansBase::deriveSaveBits( QString &err, int n16BitChans )
{
    err.clear();

    if( Subset::isAllChansStr( uiSaveChanStr ) ) {

        uiSaveChanStr = "all";
        Subset::defaultBits( saveBits, n16BitChans );
    }
    else if( Subset::rngStr2Bits( saveBits, uiSaveChanStr ) ) {

        uiSaveChanStr = Subset::bits2RngStr( saveBits );

        if( !saveBits.count( true ) ) {
            err = QString(
                    "You must specify at least one %1 channel to save.")
                    .arg( type() );
            return false;
        }

        if( saveBits.size() > n16BitChans ) {
            err = QString(
                    "Save subset [%1] includes channels"
                    " higher than maximum [%2].")
                    .arg( type() )
                    .arg( n16BitChans - 1 );
            return false;
        }

        // in case smaller
        saveBits.resize( n16BitChans );
    }
    else {
        err = QString("Channel save subset [%1] has incorrect format.")
                .arg( type() );
        return false;
    }

    return true;
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
// MS: Revisit for phase 3B2
            return im.each[streamID( trgTTL.stream )]
                    .imCumTypCnt[CimCfg::imSumNeural];
        }
    }

    if( mode.mTrig == eTrigSpike )
        return trgSpike.aiChan;

    return -1;
}


void Params::apSaveBits( QBitArray &apBits, int ip ) const
{
    im.each[ip].justAPBits( apBits, sns.imChans.saveBits );
}


void Params::lfSaveBits( QBitArray &lfBits, int ip ) const
{
    im.each[ip].justLFBits( lfBits, sns.imChans.saveBits );
}


int Params::apSaveChanCount( int ip ) const
{
    QBitArray   apBits;
    apSaveBits( apBits, ip );
    return apBits.count( true );
}


int Params::lfSaveChanCount( int ip ) const
{
    QBitArray   lfBits;
    lfSaveBits( lfBits, ip );
    return lfBits.count( true );
}


void Params::loadSettings( bool remote )
{
    QString fn = QString("daq%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, fn );
    settings.beginGroup( "DAQSettings" );

// -------
// Streams
// -------

    im.loadSettings( settings );
    ni.loadSettings( settings );

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
    settings.value( "trgTTLThresh", 2.0 ).toDouble();

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
    settings.value( "manOvShowBut", false ).toBool();

    mode.manOvInitOff =
    settings.value( "manOvInitOff", false ).toBool();

    if( !mode.manOvShowBut )
        mode.manOvInitOff = false;

// --------
// SeeNSave
// --------

    sns.imChans.shankMapFile =
    settings.value( "snsImShankMapFile", QString() ).toString();

    sns.imChans.chanMapFile =
    settings.value( "snsImChanMapFile", QString() ).toString();

    sns.imChans.uiSaveChanStr =
    settings.value( "snsImSaveChanSubset", "all" ).toString();

    sns.niChans.shankMapFile =
    settings.value( "snsNiShankMapFile", QString() ).toString();

    sns.niChans.chanMapFile =
    settings.value( "snsNiChanMapFile", QString() ).toString();

    sns.niChans.uiSaveChanStr =
    settings.value( "snsNiSaveChanSubset", "all" ).toString();

    sns.notes =
    settings.value( "snsNotes", "" ).toString();

    sns.runName =
    settings.value( "snsRunName", "myRun" ).toString();

    sns.reqMins =
    settings.value( "snsReqMins", 10 ).toInt();
}


void Params::saveSettings( bool remote ) const
{
    QString fn = QString("daq%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, fn );
    settings.beginGroup( "DAQSettings" );

// -------
// Streams
// -------

    im.saveSettings( settings );
    ni.saveSettings( settings );

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

    settings.setValue( "snsImShankMapFile", sns.imChans.shankMapFile );
    settings.setValue( "snsImChanMapFile", sns.imChans.chanMapFile );
    settings.setValue( "snsImSaveChanSubset", sns.imChans.uiSaveChanStr );

    settings.setValue( "snsNiShankMapFile", sns.niChans.shankMapFile );
    settings.setValue( "snsNiChanMapFile", sns.niChans.chanMapFile );
    settings.setValue( "snsNiSaveChanSubset", sns.niChans.uiSaveChanStr );

    settings.setValue( "snsNotes", sns.notes );
    settings.setValue( "snsRunName", sns.runName );
    settings.setValue( "snsReqMins", sns.reqMins );
}


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


