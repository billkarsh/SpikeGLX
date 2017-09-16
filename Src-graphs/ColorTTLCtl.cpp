
#include "ui_ColorTTLDialog.h"

#include "Util.h"
#include "DAQ.h"
#include "ColorTTLCtl.h"
#include "MGraph.h"
#include "GUIControls.h"
#include "HelpButDialog.h"
#include "SignalBlocker.h"
#include "Subset.h"

#include <QMessageBox>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* TTLClr --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool ColorTTLCtl::TTLClr::validIm(
    QString             &err,
    const QString       &clr,
    const DAQ::Params   &p )
{
    if( !p.im.enabled ) {

        err = QString("%1 channel: Imec not enabled.").arg( clr );
        return false;
    }

// MS: Analog and digital aux may be redefined in phase 3B2

    if( isAnalog ) {

        // Tests for analog channel and threshold

        int ip      = p.streamID( stream );
        int nLegal  = p.im.each[ip].imCumTypCnt[CimCfg::imSumNeural];

        if( chan < 0 || chan >= nLegal ) {

            err =
            QString(
            "Invalid %1 analog channel [%2]; must be in range [0..%3].")
            .arg( clr )
            .arg( chan )
            .arg( nLegal - 1 );
            return false;
        }

        double  Tmin = p.im.int10ToV( -512, ip, chan ),
                Tmax = p.im.int10ToV(  511, ip, chan );

        if( T < Tmin || T > Tmax ) {

            err =
            QString(
            "%1 analog threshold must be in range (%2..%3) V.")
            .arg( clr )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {

        // Tests for digital bit

        if( bit >= 16 ) {

            err =
            QString(
            "%1 TTL trigger bits must be in range [0..15].")
            .arg( stream );
            return false;
       }
    }

    return true;
}


bool ColorTTLCtl::TTLClr::validNi(
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
            "%1 analog threshold must be in range (%2..%3) V.")
            .arg( clr )
            .arg( Tmin ).arg( Tmax );
            return false;
        }
    }
    else {

        // Tests for digital bit

        QVector<uint>   vc;
        int             maxBit;

        Subset::rngStr2Vec( vc, p.ni.uiXDStr1 );
        maxBit = vc.size();
        Subset::rngStr2Vec( vc, p.ni.uiXDStr2() );
        maxBit += vc.size();

        if( !maxBit ) {
            err =
            QString(
            "No NI digital lines have been specified.");
            return false;
        }

        if( bit >= maxBit ) {

            err =
            QString(
            "Nidq TTL trigger bits must be in range [0..%1].")
            .arg( maxBit - 1 );
            return false;
       }
    }

    return true;
}


bool ColorTTLCtl::TTLClr::valid(
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

void ColorTTLCtl::ChanGroup::analogChanged( TTLClr &C, bool algCBChanged )
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

ColorTTLCtl::ColorTTLCtl(
    QObject             *parent,
    MGraphX             *Xa,
    MGraphX             *Xb,
    const DAQ::Params   &p )
    :   QObject(parent), p(p), Xa(Xa), Xb(Xb)
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

    for( int i = 0; i < 4; ++i )
        FillStreamCB( grp[i].stream, p.im.nProbes );

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
    saveSettings();

    if( cttlUI ) {
        delete cttlUI;
        cttlUI = 0;
    }

    if( dlg ) {
        delete dlg;
        dlg = 0;
    }
}


void ColorTTLCtl::showDialog()
{
// Get local copy of settings

    uiSet = set;

// Put settings to dialog

    for( int i = 0; i < 4; ++i ) {

        TTLClr  &C = uiSet.clr[i];

        SelStreamCBItem( grp[i].stream, C.stream );
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
    QVector<int>    vClr;

    setMtx.lock();

    if( anyEvents( vClr, ip ) )
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

        TTLClr  &C = uiSet.clr[i];

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

    if( valid( err ) ) {

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


bool ColorTTLCtl::valid( QString &err )
{
    QString clr[4] = {"Green", "Magenta", "Cyan", "Orange"};

    for( int i = 0; i < 4; ++i ) {

        if( !uiSet.clr[i].valid( err, clr[i], p ) )
            return false;
    }

    return true;
}


// Called only from ctor.
//
void ColorTTLCtl::loadSettings()
{
    STDSETTINGS( settings, "colorttl" );

    for( int i = 0; i < 4; ++i ) {

        settings.beginGroup( QString("TTLColor_%1").arg( i ) );
        set.clr[i].T = settings.value( "thresh", 2.0 ).toDouble();
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


int ColorTTLCtl::anyEvents( QVector<int> &clr, int ip ) const
{
    QString stream = (ip >= 0 ? QString("imec%1").arg( ip ) : "nidq");

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
    const TTLClr    &S = set.clr[clr];

    if( S.isAnalog ) {

        chan    = S.chan;
        bit     = 0;

        if( ip >= 0 )
            thresh = p.im.vToInt10( S.T, ip, chan );
        else
            thresh = p.ni.vToInt16( S.T, chan );
    }
    else {

// MS: Analog and digital aux may be redefined in phase 3B2

        if( ip >= 0 )
            chan = p.im.each[ip].imCumTypCnt[CimCfg::imSumNeural];
        else
            chan = p.ni.niCumTypCnt[CniCfg::niSumAnalog] + p.trgTTL.bit/16;

        bit     = S.bit;
        thresh  = 0;
    }

    return S.isAnalog;
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
    int         nhi     = 0;
    bool        found   = false;

// -------------------
// Must start on a low
// -------------------

    if( *d >= T ) {

        while( (d += nchans) < dlim ) {

            if( *d < T )
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( (d += nchans) < dlim ) {

        if( *d >= T ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nhi     = 1;

            if( set.inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( *d >= T ) {

                    if( ++nhi >= set.inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nhi = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    return found;
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
    int         nhi     = 0;
    bool        found   = false;

// -------------------
// Must start on a low
// -------------------

    if( (*d >> bit) & 1 ) {

        while( (d += nchans) < dlim ) {

            if( !((*d >> bit) & 1))
                goto seek_edge;
        }

        goto exit;
    }

// -------------------
// Seek edge candidate
// -------------------

seek_edge:
    while( (d += nchans) < dlim ) {

        if( (*d >> bit) & 1 ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nhi     = 1;

            if( set.inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( (*d >> bit) & 1 ) {

                    if( ++nhi >= set.inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nhi = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    return found;
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
    int         nlo     = 0;
    bool        found   = false;

// -------------------
// Seek edge candidate
// -------------------

    while( (d += nchans) < dlim ) {

        if( *d < T ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nlo     = 1;

            if( set.inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( *d < T ) {

                    if( ++nlo >= set.inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nlo = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    return found;
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
    int         nlo     = 0;
    bool        found   = false;

// -------------------
// Seek edge candidate
// -------------------

    while( (d += nchans) < dlim ) {

        if( !((*d >> bit) & 1) ) {

            // Mark edge start
            outCt   = (d - &data[chan]) / nchans;
            nlo     = 1;

            if( set.inarow == 1 ) {
                found = true;
                goto exit;
            }

            // Check extended run length
            while( (d += nchans) < dlim ) {

                if( !((*d >> bit) & 1) ) {

                    if( ++nlo >= set.inarow ) {
                        found = true;
                        goto exit;
                    }
                }
                else {
                    nlo = 0;
                    break;
                }
            }
        }
    }

// ----
// Exit
// ----

exit:
    return found;
}


// On each call whole data block is scanned.
// The state[] variables bridge action across blocks.
//
void ColorTTLCtl::processEvents(
    const vec_i16   &data,
    quint64         headCt,
    int             nC,
    QVector<int>    &vClr,
    int             ip )
{
    const double    srate = (ip >= 0 ? p.im.all.srate : p.ni.srate);
    const int       ntpts = (int)data.size() / nC;

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

                    double  start = (headCt + nextCt) / srate;

                    nextCt += set.inarow;
                    state[clr] = 1;

                    if( Xa ) {
                        Xa->spanMtx.lock();
                        Xa->evQ[clr].push_back(
                            EvtSpan( start, start + set.minSecs ) );
                        Xa->spanMtx.unlock();
                    }

                    if( Xb ) {
                        Xb->spanMtx.lock();
                        Xb->evQ[clr].push_back(
                            EvtSpan( start, start + set.minSecs ) );
                        Xb->spanMtx.unlock();
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

            double  end = (headCt + (found ? nextCt : ntpts-1)) / srate;

            if( Xa ) {
                Xa->spanMtx.lock();
                Xa->evQExtendLast( end, set.minSecs, clr );
                Xa->spanMtx.unlock();
            }

            if( Xb ) {
                Xb->spanMtx.lock();
                Xb->evQExtendLast( end, set.minSecs, clr );
                Xb->spanMtx.unlock();
            }

            if( found ) {
                nextCt += set.inarow;
                state[clr] = 0;
            }
            else
                goto next_clr;
        }
next_clr:;
    }
}


