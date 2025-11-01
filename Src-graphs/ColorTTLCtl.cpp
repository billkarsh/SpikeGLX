
#include "ui_ColorTTLDialog.h"

#include "Util.h"
#include "DAQ.h"
#include "ColorTTLCtl.h"
#include "MGraph.h"
#include "SignalBlocker.h"
#include "Subset.h"

#include <QMessageBox>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* TTLClrEach ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool ColorTTLCtl::TTLClrEach::validIm(
    QString             &err,
    const QString       &clr,
    const DAQ::Params   &p )
{
    int np = p.stream_nIM();

    if( !np ) {
        err = QString("%1 channel: Imec not enabled.").arg( clr );
        return false;
    }

    int ip = p.stream2ip( stream );

    if( ip >= np ) {

        err =
        QString(
        "Invalid %1 imec stream [%2]; must be in range [0..%3].")
        .arg( clr )
        .arg( stream )
        .arg( np - 1 );
        return false;
    }

    if( isAnalog ) {

        // Tests for analog channel and threshold

        const CimCfg::PrbEach   &E = p.im.prbj[ip];

        int nLegal = E.imCumTypCnt[CimCfg::imSumNeural],
            maxInt = E.roTbl->maxInt();

        if( chan < 0 || chan >= nLegal ) {

            err =
            QString(
            "Invalid %1 imec analog channel [%2]; must be in range [0..%3].")
            .arg( clr )
            .arg( chan )
            .arg( nLegal - 1 );
            return false;
        }

        double  Tmin = E.intToV( -maxInt, chan ),
                Tmax = E.intToV(  maxInt - 1, chan );

        if( T < Tmin || T > Tmax ) {

            err =
            QString(
            "%1 imec analog threshold [%2] must be in range (%3..%4) V.")
            .arg( clr )
            .arg( T )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {

        // Tests for digital bit

        if( bit != 0 && bit != 6 ) {

            err =
            "Only imec bits {0=imec_event, 6=sync} are available.";
            return false;
        }
    }

    return true;
}


bool ColorTTLCtl::TTLClrEach::validOb(
    QString             &err,
    const QString       &clr,
    const DAQ::Params   &p )
{
    int np = p.stream_nOB();

    if( !np ) {
        err = QString("%1 channel: obx not enabled.").arg( clr );
        return false;
    }

    int ip = p.stream2ip( stream );

    if( ip >= np ) {

        err =
        QString(
        "Invalid %1 obx stream [%2]; must be in range [0..%3].")
        .arg( clr )
        .arg( stream )
        .arg( np - 1 );
        return false;
    }

    const CimCfg::ObxEach   &E = p.im.get_iStrOneBox( ip );

    if( isAnalog ) {

        // Tests for analog channel and threshold

        int nLegal = E.obCumTypCnt[CimCfg::obSumAnalog];

        if( chan < 0 || chan >= nLegal ) {

            err =
            QString(
            "Invalid %1 obx analog channel [%2]; must be in range [0..%3].")
            .arg( clr )
            .arg( chan )
            .arg( nLegal - 1 );
            return false;
        }

        double  Tmin = E.int16ToV( -32768 ),
                Tmax = E.int16ToV(  32767 );

        if( T < Tmin || T > Tmax ) {

            err =
            QString(
            "%1 obx analog threshold [%2] must be in selected range (%3..%4) V.")
            .arg( clr )
            .arg( T )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {

        // Tests for digital bit (XD + SY)

        int nL = 16 * E.isXD + 6;

        if( bit >= nL ) {

            err =
            QString(
            "Obx TTL bit [%1] must be in range [0..%2].")
            .arg( bit )
            .arg( nL - 1 );
            return false;
        }
    }

    return true;
}


bool ColorTTLCtl::TTLClrEach::validNi(
    QString             &err,
    const QString       &clr,
    const DAQ::Params   &p )
{
    if( !p.stream_nNI() ) {
        err = QString("%1 channel: Nidq not enabled.").arg( clr );
        return false;
    }

    if( isAnalog ) {

        // Tests for analog channel and threshold

        int nLegal = p.ni.niCumTypCnt[CniCfg::niSumAnalog];

        if( chan < 0 || chan >= nLegal ) {

            err =
            QString(
            "Invalid %1 NI analog channel [%2]; must be in range [0..%3].")
            .arg( clr )
            .arg( chan )
            .arg( nLegal - 1 );
            return false;
        }

        double  Tmin = p.ni.int16ToV( -32768, chan ),
                Tmax = p.ni.int16ToV(  32767, chan );

        if( T < Tmin || T > Tmax ) {

            err =
            QString(
            "%1 NI analog threshold [%2] must be in selected range (%3..%4) V.")
            .arg( clr )
            .arg( T )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {

        // Tests for digital bit

        QVector<uint>   vc1, vc2;

        if( !Subset::rngStr2Vec( vc1, p.ni.uiXDStr1 ) ) {

            err = "Primary NI device XD list has a format error.";
            return false;
        }

        if( !Subset::rngStr2Vec( vc2, p.ni.uiXDStr2() ) ) {

            err = "Secondary NI device XD list has a format error.";
            return false;
        }

        if( vc1.size() + vc2.size() == 0 ) {

            err = "No NI digital lines have been specified.";
            return false;
        }

        int xdbits1 = 8 * p.ni.xdBytes1;

        if( bit < xdbits1 && !vc1.contains( bit ) ) {

            err =
            QString(
            "NI TTL bit [%1] not in primary NI device"
            " XD chan list [%2].")
            .arg( bit )
            .arg( p.ni.uiXDStr1 );
            return false;
        }

        if( bit >= xdbits1 && !vc2.contains( bit - xdbits1 ) ) {

            err =
            QString(
            "NI TTL bit [%1] (secondary bit %2)"
            " not in secondary NI device XD chan list [%3].")
            .arg( bit )
            .arg( bit - xdbits1 )
            .arg( p.ni.uiXDStr2() );
            return false;
        }
    }

    return true;
}


bool ColorTTLCtl::TTLClrEach::valid(
    QString             &err,
    const QString       &clr,
    const DAQ::Params   &p )
{
    if( !isOn )
        return true;

    if( p.stream_isNI( stream ) )
        return validNi( err, clr, p );

    if( p.stream_isOB( stream ) )
        return validOb( err, clr, p );

    return validIm( err, clr, p );
}

void ColorTTLCtl::TTLClrEach::loadSettings( QSettings &S, const DAQ::Params &p )
{
    T           = S.value( "thresh", 1.1 ).toDouble();
    stream      = S.value( "stream", p.jsip2stream( jsNI, 0 ) ).toString();
    chan        = S.value( "chan", 4 ).toInt();
    bit         = S.value( "bit", 0 ).toInt();
    isAnalog    = S.value( "isAnalog", true ).toBool();
    isOn        = S.value( "isOn", false ).toBool();
}


void ColorTTLCtl::TTLClrEach::saveSettings( QSettings &S ) const
{
    S.setValue( "thresh", T );
    S.setValue( "stream", stream );
    S.setValue( "chan", chan );
    S.setValue( "bit", bit );
    S.setValue( "isAnalog", isAnalog );
    S.setValue( "isOn", isOn );
}

/* ---------------------------------------------------------------- */
/* ChanGroup ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void ColorTTLCtl::ItemGrp::analogChanged( TTLClrEach &C, bool algCBChanged )
{
    if( algCBChanged ) {

        C.isAnalog = analog->currentIndex();

        if( C.isAnalog ) {
            C.bit = chan->value();
            chan->setValue( C.chan );
        }
        else {
            C.chan = chan->value();
            chan->setValue( C.bit );
        }
    }

    bool    enabled = GB->isChecked() && C.isAnalog;

    TLbl->setEnabled( enabled );
    T->setEnabled( enabled );
}

/* ---------------------------------------------------------------- */
/* ColorTTLCtl ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ColorTTLCtl::ColorTTLCtl( QObject *parent, const DAQ::Params &p )
    :   QObject(parent), p(p)
{
    resetState();
    loadSettings();

    dlg = new QDialog();

    cttlUI = new Ui::ColorTTLDialog;
    cttlUI->setupUi( dlg );

    grp[0].GB   = cttlUI->C0GB;
    grp[1].GB   = cttlUI->C1GB;
    grp[2].GB   = cttlUI->C2GB;
    grp[3].GB   = cttlUI->C3GB;

    grp[0].stream   = cttlUI->stream0CB;
    grp[1].stream   = cttlUI->stream1CB;
    grp[2].stream   = cttlUI->stream2CB;
    grp[3].stream   = cttlUI->stream3CB;

    grp[0].analog   = cttlUI->analog0CB;
    grp[1].analog   = cttlUI->analog1CB;
    grp[2].analog   = cttlUI->analog2CB;
    grp[3].analog   = cttlUI->analog3CB;

    grp[0].chan = cttlUI->chan0SB;
    grp[1].chan = cttlUI->chan1SB;
    grp[2].chan = cttlUI->chan2SB;
    grp[3].chan = cttlUI->chan3SB;

    grp[0].TLbl = cttlUI->thresh0Lbl;
    grp[1].TLbl = cttlUI->thresh1Lbl;
    grp[2].TLbl = cttlUI->thresh2Lbl;
    grp[3].TLbl = cttlUI->thresh3Lbl;

    grp[0].T    = cttlUI->T0SB;
    grp[1].T    = cttlUI->T1SB;
    grp[2].T    = cttlUI->T2SB;
    grp[3].T    = cttlUI->T3SB;

    for( int i = 0; i < 4; ++i )
        p.streamCB_fillRuntime( grp[i].stream );

    ConnectUI( cttlUI->C0GB, SIGNAL(clicked(bool)), this, SLOT(C0GBClicked()) );
    ConnectUI( cttlUI->C1GB, SIGNAL(clicked(bool)), this, SLOT(C1GBClicked()) );
    ConnectUI( cttlUI->C2GB, SIGNAL(clicked(bool)), this, SLOT(C2GBClicked()) );
    ConnectUI( cttlUI->C3GB, SIGNAL(clicked(bool)), this, SLOT(C3GBClicked()) );

    ConnectUI( cttlUI->analog0CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana0CBChanged()) );
    ConnectUI( cttlUI->analog1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana1CBChanged()) );
    ConnectUI( cttlUI->analog2CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana2CBChanged()) );
    ConnectUI( cttlUI->analog3CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana3CBChanged()) );

    ConnectUI( cttlUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
    ConnectUI( cttlUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
}


ColorTTLCtl::~ColorTTLCtl()
{
    if( cttlUI ) {
        delete cttlUI;
        cttlUI = 0;
    }

    if( dlg ) {
        delete dlg;
        dlg = 0;
    }
}


void ColorTTLCtl::setClients(
    int         jsa,
    int         ipa,
    const AIQ   *Qa,
    MGraphX     *Xa,
    int         jsb,
    int         ipb,
    const AIQ   *Qb,
    MGraphX     *Xb )
{
    QMutexLocker    ml( &setMtx );
    resetState();
    A.init( Xa, Qa, jsa, ipa, p );
    B.init( Xb, Qb, jsb, ipb, p );
}


bool ColorTTLCtl::valid( QString &err, bool checkStored )
{
    err.clear();

    TTLClrSet   &S      = (checkStored ? set : uiSet);
    QString     clr[4]  = {"Green", "Magenta", "Cyan", "Orange"};

    for( int i = 0; i < 4; ++i ) {

        QString E;

        if( !S.clr[i].valid( E, clr[i], p ) ) {

            if( err.isEmpty() )
                err = E;
            else {
                err.append( "\n" );
                err.append( E );
            }
        }
    }

    return err.isEmpty();
}


void ColorTTLCtl::showDialog()
{
// Get local copy of settings

    uiSet = set;

// Put settings to dialog

    for( int i = 0; i < 4; ++i ) {

        TTLClrEach  &C = uiSet.clr[i];

        p.streamCB_selItem( grp[i].stream, C.stream, false );
        grp[i].T->setValue( C.T );

        {
            SignalBlocker   b0(grp[i].analog),
                            b1(grp[i].GB);

            grp[i].analog->setCurrentIndex( C.isAnalog );
            grp[i].chan->setValue( C.isAnalog ? C.chan : C.bit );
            grp[i].GB->setChecked( C.isOn );
        }

        grp[i].analogChanged( C, false );
    }

    cttlUI->inarowSB->setValue( uiSet.inarow );
    cttlUI->widthSB->setValue( uiSet.minSecs );

// Run dialog

    dlg->exec();
    dlg->close();
}


void ColorTTLCtl::scanBlock(
    const MGraphX   *X,
    const vec_i16   &data,
    quint64         headCt,
    int             nC,
    int             js,
    int             ip )
{
    std::vector<int>    vClr;
    QMutexLocker        ml( &setMtx );

    if( eventsScanningThisStream( X, vClr, js, ip ) )
        processEvents( data, headCt, nC, vClr, js, ip );
}


void ColorTTLCtl::C0GBClicked()
{
    grp[0].analogChanged( uiSet.clr[0], false );
}


void ColorTTLCtl::C1GBClicked()
{
    grp[1].analogChanged( uiSet.clr[1], false );
}


void ColorTTLCtl::C2GBClicked()
{
    grp[2].analogChanged( uiSet.clr[2], false );
}


void ColorTTLCtl::C3GBClicked()
{
    grp[3].analogChanged( uiSet.clr[3], false );
}


void ColorTTLCtl::ana0CBChanged()
{
    grp[0].analogChanged( uiSet.clr[0], true );
}


void ColorTTLCtl::ana1CBChanged()
{
    grp[1].analogChanged( uiSet.clr[1], true );
}


void ColorTTLCtl::ana2CBChanged()
{
    grp[2].analogChanged( uiSet.clr[2], true );
}


void ColorTTLCtl::ana3CBChanged()
{
    grp[3].analogChanged( uiSet.clr[3], true );
}


void ColorTTLCtl::helpBut()
{
    showHelp( "ColorTTL_Help" );
}


void ColorTTLCtl::okBut()
{
// Get settings from dialog

    for( int i = 0; i < 4; ++i ) {

        TTLClrEach  &C = uiSet.clr[i];

        C.isOn      = grp[i].GB->isChecked();
        C.stream    = grp[i].stream->currentText();
        C.isAnalog  = grp[i].analog->currentIndex();
        C.T         = grp[i].T->value();

        if( C.isAnalog )
            C.chan  = grp[i].chan->value();
        else
            C.bit   = grp[i].chan->value();
    }

    uiSet.inarow    = cttlUI->inarowSB->value();
    uiSet.minSecs   = cttlUI->widthSB->value();

// Validate

    QString err;

    if( valid( err, false ) ) {

        // Enact new settings
        setMtx.lock();
            set = uiSet;
            resetState();
            saveSettings();
        setMtx.unlock();

        dlg->accept();
    }
    else if( !err.isEmpty() )
        QMessageBox::critical( dlg, "TTL Parameter Error", err );
}


// Called only from ctor.
//
void ColorTTLCtl::loadSettings()
{
    STDSETTINGS( settings, "colorttl" );

    for( int i = 0; i < 4; ++i ) {

        settings.beginGroup( QString("TTLColor_%1").arg( i ) );
            set.clr[i].loadSettings( settings, p );
        settings.endGroup();
    }

    settings.beginGroup( "TTLColor_All" );
    set.minSecs = settings.value( "minSecs", 0.001 ).toDouble();
    set.inarow = settings.value( "inarow", 5 ).toInt();
}


void ColorTTLCtl::saveSettings() const
{
    STDSETTINGS( settings, "colorttl" );

    for( int i = 0; i < 4; ++i ) {

        settings.beginGroup( QString("TTLColor_%1").arg( i ) );
            set.clr[i].saveSettings( settings );
        settings.endGroup();
    }

    settings.beginGroup( "TTLColor_All" );
    settings.setValue( "minSecs", set.minSecs );
    settings.setValue( "inarow", set.inarow );
}


void ColorTTLCtl::resetState()
{
    memset( state, 0, 4*sizeof(int) );
}


int ColorTTLCtl::eventsScanningThisStream(
        const MGraphX       *X,
        std::vector<int>    &clr,
        int                 js,
        int                 ip ) const
{
// Scan only A if two views of same stream

    if( A.Q == B.Q && X == B.X )
        return 0;

    QString stream = p.jsip2stream( js, ip );

    for( int i = 0; i < 4; ++i ) {
        if( set.clr[i].isOn && set.clr[i].stream == stream )
            clr.push_back( i );
    }

    return clr.size();
}


// Return true if analog.
//
bool ColorTTLCtl::getChan(
    int     &chan,
    int     &bit,
    int     &thresh,
    int     clr,
    int     js,
    int     ip ) const
{
    const TTLClrEach    &C = set.clr[clr];

    if( C.isAnalog ) {

        chan    = C.chan;
        bit     = 0;

        switch( js ) {
            case jsNI: thresh = p.ni.vToInt16( C.T, chan ); break;
            case jsOB: thresh = p.im.get_iStrOneBox( ip ).vToInt16( C.T ); break;
            case jsIM: thresh = p.im.prbj[ip].vToInt( C.T, chan ); break;
        }
    }
    else {

        switch( js ) {
            case jsNI: chan = p.ni.niCumTypCnt[CniCfg::niSumAnalog] + C.bit/16; break;
            case jsOB: chan = p.im.get_iStrOneBox( ip ).obCumTypCnt[CimCfg::obSumAnalog]; break;
            case jsIM: chan = p.im.prbj[ip].imCumTypCnt[CimCfg::imSumNeural]; break;
        }

        bit     = C.bit % 16;
        thresh  = 0;
    }

    return C.isAnalog;
}


bool ColorTTLCtl::findRisingEdge(
    int         &outCt,
    const short *data,
    int         ntpts,
    int         offset,
    int         nchans,
    int         chan,
    qint16      T ) const
{
    const short *d      = &data[chan + offset*nchans],
                *dlim   = &data[chan +  ntpts*nchans];
    int         nok     = 0;

// -------------------
// Must start on a low
// -------------------

    if( *d >= T ) {

        while( (d += nchans) < dlim ) {

            if( *d < T )
                goto seek_edge;
        }

        return false;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( (d += nchans) < dlim ) {

        if( *d >= T ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nok     = 1;

            if( set.inarow == 1 )
                return true;

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( *d >= T ) {

                    if( ++nok >= set.inarow )
                        return true;
                }
                else
                    break;
            }
        }
    }

// ----
// Fail
// ----

    return false;
}


bool ColorTTLCtl::findBitRisingEdge(
    int         &outCt,
    const short *data,
    int         ntpts,
    int         offset,
    int         nchans,
    int         chan,
    int         bit ) const
{
    const short *d      = &data[chan + offset*nchans],
                *dlim   = &data[chan +  ntpts*nchans];
    int         nok     = 0;

// -------------------
// Must start on a low
// -------------------

    if( (*d >> bit) & 1 ) {

        while( (d += nchans) < dlim ) {

            if( !((*d >> bit) & 1))
                goto seek_edge;
        }

        return false;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( (d += nchans) < dlim ) {

        if( (*d >> bit) & 1 ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nok     = 1;

            if( set.inarow == 1 )
                return true;

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( (*d >> bit) & 1 ) {

                    if( ++nok >= set.inarow )
                        return true;
                }
                else
                    break;
            }
        }
    }

// ----
// Fail
// ----

    return false;
}


bool ColorTTLCtl::findFallingEdge(
    int         &outCt,
    const short *data,
    int         ntpts,
    int         offset,
    int         nchans,
    int         chan,
    qint16      T ) const
{
    const short *d      = &data[chan + offset*nchans],
                *dlim   = &data[chan +  ntpts*nchans];
    int         nok     = 0;

// -------------------
// Seek edge candidate
// -------------------

    while( (d += nchans) < dlim ) {

        if( *d < T ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nok     = 1;

            if( set.inarow == 1 )
                return true;

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( *d < T ) {

                    if( ++nok >= set.inarow )
                        return true;
                }
                else
                    break;
            }
        }
    }

// ----
// Fail
// ----

    return false;
}


bool ColorTTLCtl::findBitFallingEdge(
    int         &outCt,
    const short *data,
    int         ntpts,
    int         offset,
    int         nchans,
    int         chan,
    int         bit ) const
{
    const short *d      = &data[chan + offset*nchans],
                *dlim   = &data[chan +  ntpts*nchans];
    int         nok     = 0;

// -------------------
// Seek edge candidate
// -------------------

    while( (d += nchans) < dlim ) {

        if( !((*d >> bit) & 1) ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nok     = 1;

            if( set.inarow == 1 )
                return true;

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( !((*d >> bit) & 1) ) {

                    if( ++nok >= set.inarow )
                        return true;
                }
                else
                    break;
            }
        }
    }

// ----
// Fail
// ----

    return false;
}


#define DST_TREL( ct )  (syncDstTAbs( ct, src, dst, p ) - dst->Q->tZero())


// On each call whole data block is scanned.
// The state[] variables bridge action across blocks.
//
void ColorTTLCtl::processEvents(
    const vec_i16       &data,
    quint64             headCt,
    int                 nC,
    std::vector<int>    &vClr,
    int                 js,
    int                 ip )
{
    const int ntpts = int(data.size()) / nC;

    Stream  *src,
            *dst = 0;

    if( js == A.js && ip == A.ip ) {
        src = &A;
        if( B.Q )
            dst = &B;
    }
    else {
        src = &B;
        if( A.Q )
            dst = &A;
    }

    for( int i = 0, ni = vClr.size(); i < ni; ++i ) {

        int     clr     = vClr[i],
                nextCt  = 0,
                chan, bit, thresh;
        bool    isAnalog,
                found;

        isAnalog = getChan( chan, bit, thresh, clr, js, ip );

        while( nextCt < ntpts ) {

            if( !state[clr] ) {

                // Low, seeking high

                if( isAnalog ) {
                    found = findRisingEdge(
                                nextCt, &data[0], ntpts,
                                nextCt, nC, chan, thresh );
                }
                else {
                    found = findBitRisingEdge(
                                nextCt, &data[0], ntpts,
                                nextCt, nC, chan, bit );
                }

                if( found ) {

                    quint64 ct      = headCt + nextCt;
                    double  start   = ct / src->Q->sRate();

                    state[clr]  = 1;
                    nextCt     += set.inarow;

                    src->X->spanMtx.lock();
                    src->X->evQ[clr].push_back(
                        EvtSpan( start, start + set.minSecs ) );
                    src->X->spanMtx.unlock();

                    if( dst ) {
                        start = DST_TREL( ct );
                        QMutexLocker    ml( &dst->X->spanMtx );
                        dst->X->evQ[clr].push_back(
                            EvtSpan( start, start + set.minSecs ) );
                    }
                }
                else
                    goto next_clr;
            }

            // high, seeking low

            if( isAnalog ) {
                found = findFallingEdge(
                            nextCt, &data[0], ntpts,
                            nextCt, nC, chan, thresh );
            }
            else {
                found = findBitFallingEdge(
                            nextCt, &data[0], ntpts,
                            nextCt, nC, chan, bit );
            }

            // always update painting

            quint64 ct  = headCt + (found ? nextCt : ntpts-1);
            double  end = ct / src->Q->sRate();

            src->X->spanMtx.lock();
            src->X->evQExtendLast( end, set.minSecs, clr );
            src->X->spanMtx.unlock();

            if( dst ) {
                end = DST_TREL( ct );
                QMutexLocker    ml( &dst->X->spanMtx );
                dst->X->evQExtendLast( end, set.minSecs, clr );
            }

            if( found ) {
                nextCt     += set.inarow;
                state[clr]  = 0;
            }
            else
                goto next_clr;
        }
next_clr:;
    }
}


