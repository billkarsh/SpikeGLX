
#include "Util.h"
#include "DAQ.h"
#include "SignalBlocker.h"

#include <QComboBox>
#include <QSettings>


#define STRMATCH( s, target ) !(s).compare( target, Qt::CaseInsensitive )




namespace DAQ
{

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static const QString gateModeStrs[] = {
        "Immediate", "TCP",
        QString()
};

static const QString trigModeStrs[] = {
        "Immediate", "Timed", "TTL", "Spike", "TCP",
        QString()
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

// - Stream strings: "nidq", "obx2", "imec", "imec0", "imec39", ...
// - Standard order: any nidq, then any obxj, then any imecj.
// - js: stream type {0=nidq, 1=obx, 2=imec}.
// - ip: integer index part of stream string.
// - iq: single index over active streams in standard order.


// Return stream type {0=nidq, 1=obx, 2=imec}.
//
int Params::stream2js( const QString &stream )
{
    if( stream_isIM( stream ) )
        return jsIM;
    else if( stream_isOB( stream ) )
        return jsOB;
    return jsNI;
}


// Return ip or 0 if none.
//
int Params::stream2ip( const QString &stream )
{
    if( stream_isIM( stream ) )
        return stream.rightRef( stream.size() - 4 ).toInt();
    else if( stream_isOB( stream ) )
        return stream.rightRef( stream.size() - 3 ).toInt();
    return 0;
}


// Return js (and ip) or -1 if error.
//
int Params::stream2jsip( int &ip, const QString &stream ) const
{
    ip = 0;

    if( stream_isNI( stream ) ) {
        if( stream_nNI() )
            return jsNI;
    }
    else if( stream_isOB( stream ) ) {
        ip = stream2ip( stream );
        if( ip < stream_nOB() )
            return jsOB;
    }
    else if( stream_isIM( stream ) ) {
        ip = stream2ip( stream );
        if( ip < stream_nIM() )
            return jsIM;
    }

    return -1;
}


// Return iq or -1 if error.
//
int Params::stream2iq( const QString &stream ) const
{
    if( stream_isNI( stream ) ) {
        if( stream_nNI() )
            return 0;
    }
    else if( stream_isOB( stream ) ) {
        int ip = stream2ip( stream );
        if( ip < stream_nOB() )
            return stream_nNI() + ip;
    }
    else if( stream_isIM( stream ) ) {
        int ip = stream2ip( stream );
        if( ip < stream_nIM() )
            return stream_nNI() + stream_nOB() + ip;
    }

    return -1;
}


QString Params::jsip2stream( int js, int ip )
{
    switch( js ) {
        case jsNI: return "nidq";
        case jsOB: return QString("obx%1").arg( ip );
        case jsIM: return QString("imec%1").arg( ip );
    }

    return "????";
}


// Return stream string or "????" if error.
//
QString Params::iq2stream( int iq ) const
{
    int np = stream_nNI();

    if( iq < np )
        return jsip2stream( jsNI, 0 );

    iq -= np;
    np  = stream_nOB();

    if( iq < np )
        return jsip2stream( jsOB, iq );

    iq -= np;
    np  = stream_nIM();

    if( iq < np )
        return jsip2stream( jsIM, iq );

    return "????";
}


// Return js (and ip) or -1 if error.
//
int Params::iq2jsip( int &ip, int iq ) const
{
    ip = 0;

    int np = stream_nNI();

    if( iq < np )
        return jsNI;

    iq -= np;
    np  = stream_nOB();

    if( iq < np ) {
        ip = iq;
        return jsOB;
    }

    iq -= np;
    np  = stream_nIM();

    if( iq < np ) {
        ip = iq;
        return jsIM;
    }

    return -1;
}


double Params::stream_rate( int iq ) const
{
    int ip, js = iq2jsip( ip, iq );

    return stream_rate( js, ip );
}


double Params::stream_rate( int js, int ip ) const
{
    switch( js ) {
        case jsNI: return ni.srate;
        case jsOB: return im.obxj[ip].srate;
        case jsIM: return im.prbj[ip].srate;
    }

    return 0;
}


int Params::stream_nChans( int js, int ip ) const
{
    switch( js ) {
        case jsNI: return ni.niCumTypCnt[CniCfg::niSumAll];
        case jsOB: return im.obxj[ip].obCumTypCnt[CimCfg::obSumAll];
        case jsIM: return im.prbj[ip].imCumTypCnt[CimCfg::imSumAll];
    }

    return 0;
}


void Params::streamCB_fillRuntime( QComboBox *CB ) const
{
    SignalBlocker   b(CB);

    CB->clear();

    if( stream_nNI() )
        CB->addItem( jsip2stream( jsNI, 0 ) );

    for( int ip = 0, np = stream_nOB(); ip < np; ++ip )
        CB->addItem( jsip2stream( jsOB, ip ) );

    for( int ip = 0, np = stream_nIM(); ip < np; ++ip )
        CB->addItem( jsip2stream( jsIM, ip ) );
}


// Set stream item in standard stream CB.
//
// If the item is found, the index and text are set.
// If the item is not found:
//  if auto true: item zero is selected.
//  if auto false: stream is set as text.
//
// Note that the stream should be retrieved as text by all clients.
//
// Return found.
//
bool Params::streamCB_selItem( QComboBox *CB, QString stream, bool autosel ) const
{
    SignalBlocker   b(CB);

    stream = stream.toLower();

    int     sel     = CB->findText( stream );
    bool    found   = sel >= 0;

    if( found )
        CB->setCurrentIndex( sel );
    else if( autosel )
        CB->setCurrentIndex( 0 );
    else {
        CB->setEditable( true );
        CB->setCurrentText( stream );
    }

    return found;
}


// Return trigger stream or null.
//
QString Params::trigStream() const
{
    if( mode.mTrig == eTrigTTL )
        return trgTTL.stream;

    if( mode.mTrig == eTrigSpike )
        return trgSpike.stream;

    return QString();
}


// Return current trigger analog threshold as integer or zero.
//
int Params::trigThreshAsInt() const
{
    if( mode.mTrig == eTrigTTL ) {

        if( stream_isNI( trgTTL.stream ) )
            return ni.vToInt16( trgTTL.T, trgTTL.chan );
        else if( stream_isOB( trgTTL.stream ) )
            return im.obxj[stream2ip( trgTTL.stream )].vToInt16( trgTTL.T );
        else {
            return im.prbj[stream2ip( trgTTL.stream )]
                    .vToInt( trgTTL.T, trgTTL.chan );
        }
    }
    else if( mode.mTrig == eTrigSpike ) {

        if( stream_isNI( trgSpike.stream ) )
            return ni.vToInt16( trgSpike.T, trgSpike.aiChan );
        else if( stream_isOB( trgSpike.stream ) )
            return im.obxj[stream2ip( trgSpike.stream )].vToInt16( trgSpike.T );
        else {
            return im.prbj[stream2ip( trgSpike.stream )]
                    .vToInt( trgSpike.T, trgSpike.aiChan );
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
        else if( stream_isNI( trgTTL.stream ) )
            return ni.niCumTypCnt[CniCfg::niSumAnalog] + trgTTL.bit/16;
        else if( stream_isOB( trgTTL.stream ) ) {
            return im.obxj[stream2ip( trgTTL.stream )]
                    .obCumTypCnt[CimCfg::obSumAnalog];
        }
        else {
            return im.prbj[stream2ip( trgTTL.stream )]
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

    sync.imPXIInputSlot =
    settings.value( "syncImInputSlot", 2 ).toInt();

    sync.niChanType =
    settings.value( "syncNiChanType", 0 ).toInt();

    sync.niChan =
    settings.value( "syncNiChan", 0 ).toInt();

    sync.calMins =
    settings.value( "syncCalMins", 20 ).toInt();

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
    settings.value( "trgTTLStream", jsip2stream( jsNI, 0 ) ).toString();

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
    settings.value( "trgSpikeStream", jsip2stream( jsNI, 0 ) ).toString();

    trgSpike.aiChan =
    settings.value( "trgSpikeAIChan", 4 ).toInt();

    trgSpike.inarow =
    settings.value( "trgSpikeInarow", 3 ).toUInt();

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

    mode.manOvConfirm =
    settings.value( "manOvConfirm", false ).toBool();

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

    sns.lfPairChk =
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
    settings.setValue( "syncSourceIdx", int(sync.sourceIdx) );

    settings.setValue( "syncNiThresh", sync.niThresh );
    settings.setValue( "syncImInputSlot", sync.imPXIInputSlot );
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

    settings.setValue( "gateMode", int(mode.mGate) );
    settings.setValue( "trigMode", int(mode.mTrig) );
    settings.setValue( "manOvShowBut", mode.manOvShowBut );
    settings.setValue( "manOvConfirm", mode.manOvConfirm );

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
    settings.setValue( "snsPairChk", sns.lfPairChk );
    settings.setValue( "snsFldPerProbe", sns.fldPerPrb );

    settings.endGroup();

// ----
// Imec
// ----

    im.saveSettings( settings );
}


QString Params::remoteGetDAQParams()
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


void Params::remoteSetDAQParams( const QString &str )
{
    STDSETTINGS( settings, "daqremote" );
    settings.beginGroup( "DAQSettings" );

    QTextStream ts( str.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString     line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > 0 && eq < line.length() - 1 ) {

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
    if( gateMode < N_gateModes )
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
    if( trigMode < N_trigModes )
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


