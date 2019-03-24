
#include "ui_ColorTTLDialog.h"

#include "Util.h"
#include "DAQ.h"
#include "ColorTTLCtl.h"
#include "MGraph.h"
#include "HelpButDialog.h"
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
    if( !p.im.enabled ) {

        err = QString("%1 channel: Imec not enabled.").arg( clr );
        return false;
    }

    if( isAnalog ) {

        // Tests for analog channel and threshold

        int nLegal = p.im.imCumTypCnt[CimCfg::imSumNeural];

        if( chan < 0 || chan >= nLegal ) {

            err =
            QString(
            "Invalid %1 analog channel [%2]; must be in range [0..%3].")
            .arg( clr )
            .arg( chan )
            .arg( nLegal - 1 );
            return false;
        }

        double  Tmin = p.im.int10ToV( -512, chan ),
                Tmax = p.im.int10ToV(  511, chan );

        if( T < Tmin || T > Tmax ) {

            err =
            QString(
            "%1 analog threshold [%2] must be in range (%3..%4) V.")
            .arg( clr )
            .arg( T )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {

        // Tests for digital bit

        if( bit >= 16 ) {

            err = "Imec TTL trigger bits must be in range [0..15].";
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
    if( !p.ni.enabled ) {

        err = QString("%1 channel: Nidq not enabled.").arg( clr );
        return false;
    }

    if( isAnalog ) {

        // Tests for analog channel and threshold

        int nLegal = p.ni.niCumTypCnt[CniCfg::niSumAnalog];

        if( chan < 0 || chan >= nLegal ) {

            err =
            QString(
            "Invalid %1 analog channel [%2]; must be in range [0..%3].")
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
            "%1 analog threshold [%2] must be in selected NI range (%3..%4) V.")
            .arg( clr )
            .arg( T )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {

        // Tests for digital bit

        QVector<uint>   vc1, vc2;

        Subset::rngStr2Vec( vc1, p.ni.uiXDStr1 );
        Subset::rngStr2Vec( vc2, p.ni.uiXDStr2() );

        if( vc1.size() + vc2.size() == 0 ) {

            err = "No NI digital lines have been specified.";
            return false;
        }

        int xdbits1 = 8 * p.ni.xdBytes1;

        if( bit < xdbits1 && !vc1.contains( bit ) ) {

            err =
            QString(
            "Nidq TTL trigger bit [%1] not in primary NI device"
            " XD chan list [%2].")
            .arg( bit )
            .arg( p.ni.uiXDStr1 );
            return false;
        }

        if( bit >= xdbits1 && !vc2.contains( bit - xdbits1 ) ) {

            err =
            QString(
            "Nidq TTL trigger bit [%1] (secondary bit %2)"
            " not in secondary NI device XD chan list [%3].")
            .arg( bit )
            .arg( bit - 8 )
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

    if( stream == "nidq" )
        return validNi( err, clr, p );

    return validIm( err, clr, p );
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

    dlg = new HelpButDialog(
                "TTL Event Coloring Help",
                "CommonResources/ColorTTL_Help.html" );

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

    ConnectUI( cttlUI->C0GB, SIGNAL(clicked(bool)), this, SLOT(C0GBClicked()) );
    ConnectUI( cttlUI->C1GB, SIGNAL(clicked(bool)), this, SLOT(C1GBClicked()) );
    ConnectUI( cttlUI->C2GB, SIGNAL(clicked(bool)), this, SLOT(C2GBClicked()) );
    ConnectUI( cttlUI->C3GB, SIGNAL(clicked(bool)), this, SLOT(C3GBClicked()) );

    ConnectUI( cttlUI->analog0CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana0CBChanged()) );
    ConnectUI( cttlUI->analog1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana1CBChanged()) );
    ConnectUI( cttlUI->analog2CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana2CBChanged()) );
    ConnectUI( cttlUI->analog3CB, SIGNAL(currentIndexChanged(int)), this, SLOT(ana3CBChanged()) );

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
    int         ipa,
    const AIQ   *Qa,
    MGraphX     *Xa,
    int         ipb,
    const AIQ   *Qb,
    MGraphX     *Xb )
{
    setMtx.lock();
        resetState();
        A.init( Xa, Qa, ipa, p );
        B.init( Xb, Qb, ipb, p );
    setMtx.unlock();
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

        grp[i].stream->setCurrentIndex( C.stream == "nidq" );
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
    dlg->close();   // delete help dialog
}


void ColorTTLCtl::scanBlock(
    const vec_i16   &data,
    quint64         headCt,
    int             nC,
    int             ip )
{
    std::vector<int>    vClr;

    setMtx.lock();

    if( eventsScanningThisStream( vClr, ip ) )
        processEvents( data, headCt, nC, vClr, ip );

    setMtx.unlock();
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
        set.clr[i].T = settings.value( "thresh", 1.1 ).toDouble();
        set.clr[i].stream = settings.value( "stream", "nidq" ).toString();
        set.clr[i].chan = settings.value( "chan", 4 ).toInt();
        set.clr[i].bit = settings.value( "bit", 0 ).toInt();
        set.clr[i].isAnalog = settings.value( "isAnalog", true ).toBool();
        set.clr[i].isOn = settings.value( "isOn", false ).toBool();
        settings.endGroup();
    }

    settings.beginGroup( "AllTTLColors" );
    set.minSecs = settings.value( "minSecs", 0.1 ).toDouble();
    set.inarow = settings.value( "inarow", 5 ).toInt();
}


void ColorTTLCtl::saveSettings() const
{
    STDSETTINGS( settings, "colorttl" );

    for( int i = 0; i < 4; ++i ) {

        settings.beginGroup( QString("TTLColor_%1").arg( i ) );
        settings.setValue( "thresh", set.clr[i].T );
        settings.setValue( "stream", set.clr[i].stream );
        settings.setValue( "chan", set.clr[i].chan );
        settings.setValue( "bit", set.clr[i].bit );
        settings.setValue( "isAnalog", set.clr[i].isAnalog );
        settings.setValue( "isOn", set.clr[i].isOn );
        settings.endGroup();
    }

    settings.beginGroup( "AllTTLColors" );
    settings.setValue( "minSecs", set.minSecs );
    settings.setValue( "inarow", set.inarow );
}


void ColorTTLCtl::resetState()
{
    memset( state, 0, 4*sizeof(int) );
}


int ColorTTLCtl::eventsScanningThisStream(
    std::vector<int>    &clr,
    int                 ip ) const
{
    QString stream = (ip >= 0 ? "imec" : "nidq");

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
    int     ip ) const
{
    const TTLClrEach    &C = set.clr[clr];

    if( C.isAnalog ) {

        chan    = C.chan;
        bit     = 0;

        if( ip >= 0 )
            thresh = p.im.vToInt10( C.T, chan );
        else
            thresh = p.ni.vToInt16( C.T, chan );
    }
    else {

        if( ip >= 0 )
            chan = p.im.imCumTypCnt[CimCfg::imSumNeural];
        else
            chan = p.ni.niCumTypCnt[CniCfg::niSumAnalog] + C.bit/16;

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
    int                 ip )
{
    const int ntpts = (int)data.size() / nC;

    Stream  *src,
            *dst = 0;

    if( ip == A.ip ) {
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

        isAnalog = getChan( chan, bit, thresh, clr, ip );

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
                        dst->X->spanMtx.lock();
                        dst->X->evQ[clr].push_back(
                            EvtSpan( start, start + set.minSecs ) );
                        dst->X->spanMtx.unlock();
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
                dst->X->spanMtx.lock();
                dst->X->evQExtendLast( end, set.minSecs, clr );
                dst->X->spanMtx.unlock();
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


