
#include "Util.h"
#include "DAQ.h"
#include "ColorTTLCtl.h"
#include "HelpButDialog.h"
#include "MGraph.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* ColorTTLCtl ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ColorTTLCtl::ColorTTLCtl(
    QObject             *parent,
    MGraphX             *Xim,
    MGraphX             *Xni,
    const DAQ::Params   &p )
    :   QObject(parent), p(p), dlg(0), Xim(Xim), Xni(Xni)
{
    for( int i = 0; i < 4; ++i )
        state[i] = 0;

    loadSettings();
}


ColorTTLCtl::~ColorTTLCtl()
{
    saveSettings();

    if( dlg ) {
        delete dlg;
        dlg = 0;
    }
}


void ColorTTLCtl::showDialog()
{
}


void ColorTTLCtl::scanBlock(
    const vec_i16   &data,
    quint64         headCt,
    int             nC,
    bool            isImec )
{
    QVector<int>    vClr;

    if( anyEvents( vClr, isImec ) )
        processEvents( data, headCt, nC, vClr, isImec );
}


// Called only from ctor.
//
void ColorTTLCtl::loadSettings()
{
    STDSETTINGS( settings, "colorttl" );

    for( int i = 0; i < 4; ++i ) {

        settings.beginGroup( QString("TTLColor_%1").arg( i ) );
        set.clr[i].T = settings.value( "thresh", 3.0 ).toDouble();
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


int ColorTTLCtl::anyEvents( QVector<int> &clr, bool isImec ) const
{
    QString stream = (isImec ? "imec" : "nidq");

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
    bool    isImec ) const
{
    const TTLClr    &S = set.clr[clr];

    if( S.isAnalog ) {

        chan    = S.chan;
        bit     = 0;

        if( isImec )
            thresh = p.im.vToInt10( S.T, chan );
        else
            thresh = p.ni.vToInt16( S.T, chan );
    }
    else {

        if( isImec )
            chan = p.im.imCumTypCnt[CimCfg::imSumNeural];
        else
            chan = p.ni.niCumTypCnt[CniCfg::niSumAnalog] + (p.trgTTL.bit/16);

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
    bool            isImec )
{
    const double    srate = (isImec ? p.im.srate : p.ni.srate);
    const int       ntpts = (int)data.size() / nC;

    for( int i = 0, ni = vClr.size(); i < ni; ++i ) {

        int     clr     = vClr[i],
                nextCt  = 0,
                chan, bit, thresh;
        bool    isAnalog,
                found;

        isAnalog = getChan( chan, bit, thresh, clr, isImec );

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

                    if( Xim ) {
                        Xim->spanMtx.lock();
                        Xim->evQ[clr].push_back(
                            EvtSpan( start, start + set.minSecs ) );
                        Xim->spanMtx.unlock();
                    }

                    if( Xni ) {
                        Xni->spanMtx.lock();
                        Xni->evQ[clr].push_back(
                            EvtSpan( start, start + set.minSecs ) );
                        Xni->spanMtx.unlock();
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

            if( Xim ) {
                Xim->spanMtx.lock();
                Xim->evQExtendLast( end, set.minSecs, clr );
                Xim->spanMtx.unlock();
            }

            if( Xni ) {
                Xni->spanMtx.lock();
                Xni->evQExtendLast( end, set.minSecs, clr );
                Xni->spanMtx.unlock();
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


