
#include "Stim.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DAQ.h"
#include "Subset.h"
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;
#ifdef HAVE_NIDAQmx
#include "NI/NIDAQmx.h"
#else
typedef void*   TaskHandle;
#endif

#include <QDir>
#include <QSettings>
#include <QStack>
#include <QThread>
#include <QVector>

#include <math.h>

/* ---------------------------------------------------------------- */
/* Globals -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static QMap<QString,TaskHandle> gNITasks;

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx

#define DAQmxErrChkNoJump(functionCall)                     \
    (DAQmxFailed(dmxErrNum = (functionCall)) &&             \
    (dmxFnName = STR(functionCall)))

static QVector<char>    dmxErrMsg;
static const char       *dmxFnName;
static int32            dmxErrNum;

static void ni_clearErrors()
{
    dmxErrMsg.clear();
    dmxFnName   = "";
    dmxErrNum   = 0;
}

// Capture latest dmxErrNum as a descriptive C-string.
// Call as soon as possible after offending operation.
//
static void ni_setExtErrMsg()
{
    const int msgbytes = 2048;
    dmxErrMsg.resize( msgbytes );
    dmxErrMsg[0] = 0;
    DAQmxGetExtendedErrorInfo( &dmxErrMsg[0], msgbytes );
}

static QString ni_getError()
{
    if( DAQmxFailed( dmxErrNum ) ) {
        ni_setExtErrMsg();
        QString e = QString("DAQmx Error:\nFun=<%1>\n").arg( dmxFnName );
        e += QString("ErrNum=<%1>\n").arg( dmxErrNum );
        e += QString("ErrMsg='%1'.").arg( &dmxErrMsg[0] );
        return e;
    }

    return QString();
}

static void ni_destroyTask( TaskHandle &taskHandle )
{
    if( taskHandle ) {
        DAQmxStopTask( taskHandle );
        DAQmxClearTask( taskHandle );
        taskHandle = 0;
    }
}

static QString ni_outChan2dev( const QString &outChan )
{
    QStringList sl = outChan.split('/');
    return sl[0].trimmed();
}

static TaskHandle ni_outChan2task( const QString &outChan )
{
    QMap<QString,TaskHandle>::iterator
        it  = gNITasks.find( outChan.toLower() ),
        end = gNITasks.end();

    ni_clearErrors();

    return (it != end ? it.value() : 0);
}

static void ni_setTask( const QString &outChan, TaskHandle T )
{
    gNITasks[outChan.toLower()] = T;
}

static void ni_removeTask( const QString &outChan )
{
    TaskHandle  T = ni_outChan2task( outChan );

    if( T ) {
        ni_destroyTask( T );
        gNITasks.remove( outChan );
    }
}

static void ni_set1AnalogValue( const QString &outChan, double v )
{
    TaskHandle  T;

    if( DAQmxFailed( DAQmxCreateTask( "", &T ) ) )
        goto done;
    if( DAQmxFailed( DAQmxCreateAOVoltageChan(
                        T,
                        STR2CHR( outChan ),
                        "",
                        -5.0,
                        5.0,
                        DAQmx_Val_Volts,
                        NULL ) ) )
        goto done;
     DAQmxWriteAnalogScalarF64(
                        T,
                        true,   // autostart
                        0,      // timeout seconds
                        v,
                        NULL );

done:
    ni_destroyTask( T );
}

#endif

/* ---------------------------------------------------------------- */
/* WaveMeta ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString WaveMeta::readMetaFile( const QString &wave )
{
    QString path = QString("%1/_Waves/%2.meta").arg( appPath() ).arg( wave );

    if( !QFile( path ).exists() ) {
        QString err =
        QString("Wave.read: Can't find file '%1'.").arg( path );
        Log() << err;
        return err;
    }

    QSettings S( path, QSettings::IniFormat );
    S.beginGroup( "WaveMeta" );

    freq    = S.value( "sample_frequency_Hz_dbl", 0 ).toDouble();
    wav_vpp = S.value( "wave_Vpp_dbl", 0 ).toDouble();
    dev_vpp = S.value( "device_Vpp_dbl", 0 ).toDouble();
    nsamp   = S.value( "num_samples_int32", 0 ).toInt();
    isbin   = S.value( "data_are_binary_bool", false ).toBool();

    if( wav_vpp > dev_vpp ) {
        return
        QString("Wave.read: Wave voltage exceeds device voltage in file '%1'.")
        .arg( path );
    }

    if( nsamp < 0 ) {
        return
        QString("Wave.read: Negative sample count %1 in file '%2'.")
        .arg( nsamp ).arg( path );
    }

    return QString();
}


QString WaveMeta::writeMetaFile( const QString &wave ) const
{
    QString path = QString("%1/_Waves").arg( appPath() );

    if( !QDir().mkdir( path ) ) {
        QString err =
        QString("Wave.write: Can't create dir '%1'.").arg( path );
        Log() << err;
        return err;
    }

    path = QString("%1/%2.meta").arg( path ).arg( wave );

    QSettings S( path, QSettings::IniFormat );
    S.beginGroup( "WaveMeta" );

    S.setValue( "sample_frequency_Hz_dbl", freq );
    S.setValue( "wave_Vpp_dbl", wav_vpp );
    S.setValue( "device_Vpp_dbl", dev_vpp );
    S.setValue( "num_samples_int32", nsamp );
    S.setValue( "data_are_binary_bool", isbin );

    return QString();
}


QString WaveMeta::readTextFile( QString &text, const QString &wave )
{
// Open

    QString path = QString("%1/_Waves/%2.txt").arg( appPath() ).arg( wave );
    QFile   f( path );

    if( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        return
        QString("Waveplayer can't open(read) wave text file '%1'.")
        .arg( path );
    }

// Read

    text = QTextStream( &f ).readAll();

    if( !text.size() ) {
        return
        QString("Waveplayer can't read wave text file '%1'.")
        .arg( path );
    }

    return QString();
}


QString WaveMeta::writeTextFile( const QString &wave, QString &text )
{
// Open

    QString path = QString("%1/_Waves/%2.txt").arg( appPath() ).arg( wave );
    QFile   f( path );

    if( !f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        return
        QString("Waveplayer can't open(write) wave text file '%1'.")
        .arg( path );
    }

// Write

    QTextStream ts( &f );

    ts << text;

    return QString();
}


struct LOOP {
    int reps,
        offset,
        nsamp;
    LOOP()
        : reps(1), offset(0), nsamp(0)          {}
    LOOP( int reps, int offset )
        : reps(reps), offset(offset), nsamp(0)  {}
};

enum {
    inNone  = 0,
    inLoop  = 1,
    inShape = 2
};


// Vocab:
// - do N {}
// - level( V, dur_ms )
// - ramp( V1, V2, dur_ms )
// - sin( amp_V, offset_V, freq_Hz, dur_ms )
//
// testMax >  0: measure and set nsamp iff <= testMax.
// testMax <= 0: read nsamp; fill buf.
//
QString WaveMeta::parseText( vec_i16 &buf, const QString &text, int testMax )
{
    QRegExp         reDo("do(\\d+)$"),
                    reShape("(.*)\\((.*)");
    QStack<LOOP>    Lstk;
    QString         token;
    double          scl     = 0x7FFF * wav_vpp / dev_vpp,
                    t2s     = 0.001  * freq;
    int             state   = inNone,
                    offset  = 0,    // running written nsamp
                    nlbrace = 0,    // { count
                    nrbrace = 0,    // } count
                    tlin    = 0,    // token start line
                    tpos    = 0,    // token start pos
                    clin    = 1,    // cursor line
                    cpos    = 0;    // cursor pos

    if( testMax > 0 )
        nsamp = 0;
    else
        buf.resize( nsamp );

// Enclose all in 'do 1 {}' to have a top place to pass sums upward.
// There's no final '}' that will pop this off the stack so it doesn't
// change the action.

    Lstk.push( LOOP() );

// Character loop

    for( int i = 0, n = text.size(); i < n; ++i ) {

        QChar   C = text[i].toLower();

        if( C == QChar::LineFeed ) {
            ++clin;
            cpos = 0;
            continue;
        }

        cpos += C.isPrint();

        if( C.isSpace() )
            ;
        else if( C == '}' ) {

            if( state != inNone || Lstk.size() <= 1 ) {
                return
                QString("Wave text: Misplaced closing '}' at line %1 pos %2.")
                .arg( clin ).arg( cpos );
            }

            // ----------
            // Execute do
            // ----------

            LOOP    L = Lstk.pop();

            Lstk.top().nsamp += L.reps * L.nsamp;

            for( int irep = 2; irep <= L.reps; ++irep ) {
                if( testMax <= 0 ) {
                    memcpy(
                        &buf[offset],
                        &buf[L.offset],
                        L.nsamp*sizeof(qint16) );
                }
                offset += L.nsamp;
            }

            ++nrbrace;
        }
        else if( C == '{' ) {

            if( state == inNone ) {
                return
                QString("Wave text: Misplaced opening '{' at line %1 pos %2.")
                .arg( clin ).arg( cpos );
            }

            if( state == inShape ) {
                return
                QString("Wave text: Bad shape token '%1' at line %2 pos %3.")
                .arg( token ).arg( tlin ).arg( tpos );
            }

            // --------------
            // Parse do token
            // --------------

            int idx = token.indexOf( reDo );
            if( idx < 0 ) {
                return
                QString("Wave text: Bad 'do' command '%1' at line %2 pos %3.")
                .arg( token ).arg( tlin ).arg( tpos );
            }

            Lstk.push( LOOP( reDo.cap(1).toInt(), offset ) );
            ++nlbrace;
            state = inNone;
        }
        else if( C == ')' ) {

            if( state == inNone ) {
                return
                QString("Wave text: Misplaced closing ')' at line %1 pos %2.")
                .arg( clin ).arg( cpos );
            }

            if( state == inLoop ) {
                return
                QString("Wave text: Bad 'do' command %1 at line %2 pos %3.")
                .arg( token ).arg( tlin ).arg( tpos );
            }

            // -------------------------
            // Parse/execute shape token
            // -------------------------

            int idx = token.indexOf( reShape );
            if( idx < 0 ) {
                return
                QString("Wave text: Bad shape type '%1' at line %2 pos %3.")
                .arg( token ).arg( tlin ).arg( tpos );
            }

            QString     shape   = reShape.cap(1);
            QStringList args    = reShape.cap(2).split(',');
            int         nsamp;

            token += ')';   // for error messages

            if( shape == "level" ) {

                if( args.size() != 2 ) {
                    return
                    QString("Wave text: level(v,t) takes 2 params: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                double  v = args[0].toDouble();
                if( v > 1 || v < -1 ) {
                    return
                    QString("Wave text: level(v,t) voltage out of range [-1,1]: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                nsamp = args[1].toDouble() * t2s;
                if( nsamp <= 0 ) {
                    return
                    QString("Wave text: level(v,t) time <= 0: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                if( testMax <= 0 ) {
                    qint16  *dst = &buf[offset];
                    v *= scl;
                    for( int i = 0; i < nsamp; ++i, ++dst )
                        *dst = v;
                }
            }
            else if( shape == "ramp" ) {

                if( args.size() != 3 ) {
                    return
                    QString("Wave text: ramp(v1,v2,t) takes 3 params: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                double  v1 = args[0].toDouble(),
                        v2 = args[1].toDouble();
                if( v1 > 1 || v1 < -1 ) {
                    return
                    QString("Wave text: ramp(v1,v2,t) v1 out of range [-1,1]: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }
                if( v2 > 1 || v2 < -1 ) {
                    return
                    QString("Wave text: ramp(v1,v2,t) v2 out of range [-1,1]: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                nsamp = args[2].toDouble() * t2s;
                if( nsamp <= 0 ) {
                    return
                    QString("Wave text: ramp(v1,v2,t) time <= 0: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                if( testMax <= 0 ) {
                    qint16  *dst = &buf[offset];
                    v1 *= scl;
                    v2 *= scl;
                    v2  = (v2 - v1) / nsamp;
                    for( int i = 0; i < nsamp; ++i, ++dst )
                        *dst = v1 + i*v2;
                }
            }
            else if( shape == "sin" ) {

                if( args.size() != 4 ) {
                    return
                    QString("Wave text: sin(A,B,f,t) takes 4 params: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                double  A = args[0].toDouble(),
                        B = args[1].toDouble();
                if( A + B > 1 || A + B < -1 ) {
                    return
                    QString("Wave text: sin(A,B,f,t) (A+B) out of range [-1,1]: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                nsamp = args[3].toDouble() * t2s;
                if( nsamp <= 0 ) {
                    return
                    QString("Wave text: sin(A,B,f,t) time <= 0: '%1' at line %2 pos %3.")
                    .arg( token ).arg( tlin ).arg( tpos );
                }

                if( testMax <= 0 ) {
                    double  w    = 2*M_PI * args[2].toDouble() / (1000*t2s);
                    qint16  *dst = &buf[offset];
                    A *= scl;
                    B *= scl;
                    for( int i = 0; i < nsamp; ++i, ++dst )
                        *dst = B + A*sin( w*i );
                }
            }
            else {
                return
                QString("Wave text: Unknown shape type '%1' at line %2 pos %3.")
                .arg( token ).arg( tlin ).arg( tpos );
            }

            Lstk.top().nsamp    += nsamp;
            offset              += nsamp;

            state = inNone;
        }
        else if( state == inNone ) {

            // ---------
            // New token
            // ---------

            token = C;
            tlin  = clin;
            tpos  = cpos;

            if( C == 'd' )
                state = inLoop;
            else
                state = inShape;
        }
        else {

            // --------------
            // Continue token
            // --------------

            token += C;
        }
    }

    if( state != inNone ) {
        return
        QString("Wave text: Unfinished token '%1' at line %2 pos %3.")
        .arg( token ).arg( tlin ).arg( tpos );
    }

    if( nrbrace != nlbrace ) {
        return
        QString("Wave text: Braces not paired: There are %1 '{' and there are %2 '}'.")
        .arg( nlbrace ).arg( nrbrace );
    }

    if( testMax > 0 ) {

        if( offset & 1 ) {
            return
            QString("Wave text: Odd sample count %1.").arg( offset );
        }
        if( offset > testMax ) {
            return
            QString("Wave text: Sample count exceeds %1.").arg( testMax );
        }

        nsamp = offset;
    }

    return QString();
}

/* ---------------------------------------------------------------- */
/* OBX ------------------------------------------------------------ */
/* ---------------------------------------------------------------- */

struct OBX {
    const ConfigCtl     *C;
    const DAQ::Params   &p;
    int                 istr,
                        isel,
                        slot;
    OBX( int istr );
    const QString &AIStr()  {return p.im.get_iSelOneBox( isel ).uiXAStr;}
    const QString &AOStr()  {return p.im.get_iSelOneBox( isel ).uiAOStr;}
};


OBX::OBX( int istr )
    :   C(mainApp()->cfgCtl()), p(C->acceptedParams), istr(istr)
{
    isel  = p.im.obx_istr2isel( istr );
    slot  = C->prbTab.get_iOneBox( isel ).slot;
}

/* ---------------------------------------------------------------- */
/* CStim ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Debug ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static void generate_sqrtest()
{
#define NDATA   10000

    WaveMeta    W;
    W.freq      = NDATA;
    W.wav_vpp   = 2;
    W.dev_vpp   = 5;
    W.nsamp     = NDATA;
    W.isbin     = true;
    W.writeMetaFile( "sqrtest" );

    std::vector<float>  buf;
    buf.resize( NDATA );
    float   *pf = &buf[0];

    for( int i = 0; i < NDATA/2; ++i, ++pf )
        *pf = 1;
    for( int i = NDATA/2; i < NDATA; ++i, ++pf )
        *pf = -1;

    QString path = QString("%1/_Waves/sqrtest.bin").arg( appPath() );
    QFile   f( path );
    f.open( QIODevice::WriteOnly );
    f.write( (char*)&buf[0], NDATA*sizeof(float) );
}


void CStim::stimteststart()
{
    Log()<<"writ "<< obx_wave_download_file( 0, "parsetest" );
    Log()<<"arm  "<< obx_wave_arm( 0, -2, true );
    Log()<<"trig "<< obx_wave_start_stop( 0, true );
}

/* ---------------------------------------------------------------- */
/* OBX waveplayer client ------------------------------------------ */
/* ---------------------------------------------------------------- */

QString CStim::obx_wave_download_file( int istr, const QString &wave )
{
    WaveMeta    W;
    QString     err = W.readMetaFile( wave );

    if( !err.isEmpty() )
        return err;

    if( W.freq > OBX_WAV_FRQ_MAX ) {
        return
        QString("Wave.read: Sample frequency exceeds %1 in file '%2'.")
        .arg( OBX_WAV_FRQ_MAX ).arg( wave );
    }
    if( W.nsamp & 1 ) {
        return
        QString("Wave.read: Odd sample count %1 in file '%2'.")
        .arg( W.nsamp ).arg( wave );
    }
    if( W.nsamp > OBX_WAV_BUF_MAX ) {
        return
        QString("Wave.read: Sample count exceeds %1 in file '%2'.")
        .arg( OBX_WAV_BUF_MAX ).arg( wave );
    }

    if( W.isbin )
        return obx_wave_download_binFile( istr, W, wave );
    else
        return obx_wave_download_txtFile( istr, W, wave );
}


QString CStim::obx_wave_download_binFile(
    int             istr,
    const WaveMeta  &W,
    const QString   &wave )
{
// Open binary

    QString path = QString("%1/_Waves/%2.bin").arg( appPath() ).arg( wave );
    QFile   f( path );

    if( !f.open( QIODevice::ReadOnly ) ) {
        return
        QString("Waveplayer can't open wave binary file '%1'.")
        .arg( path );
    }

// Read binary

    std::vector<float>  fbuf( W.nsamp );
    float   *pf = &fbuf[0];

    if( !f.read( (char*)pf, W.nsamp*sizeof(float) ) ) {
        return
        QString("Waveplayer can't read wave binary file '%1'.")
        .arg( path );
    }

// Scale binary

    vec_i16 buf( W.nsamp );
    qint16  *pi     = &buf[0];
    double  scale   = 0x7FFF * W.wav_vpp / W.dev_vpp;

    for( int i = 0; i < W.nsamp; ++i, ++pf, ++pi )
        *pi = scale * *pf;

// Download binary

    return obx_wave_download_buf( W.freq, istr, W.nsamp, &buf[0] );
}


QString CStim::obx_wave_download_txtFile(
    int             istr,
    WaveMeta        &W,
    const QString   &wave )
{
    QString text, err;
    vec_i16 buf;

    err = W.readTextFile( text, wave );
    if( !err.isEmpty() ) return err;

    err = W.parseText( buf, text, OBX_WAV_BUF_MAX );
    if( !err.isEmpty() ) return err;

    err = W.parseText( buf, text, 0 );
    if( !err.isEmpty() ) return err;

    return obx_wave_download_buf( W.freq, istr, W.nsamp, &buf[0] );
}

/* ---------------------------------------------------------------- */
/* OBX waveplayer hardware ---------------------------------------- */
/* ---------------------------------------------------------------- */

QString CStim::obx_wave_download_buf(
    double          freq,
    int             istr,
    int             len,
    const qint16    *src )
{
#ifdef HAVE_IMEC
    OBX             X( istr );
    NP_ErrorCode    err;

// Set frequency

    err = np_waveplayer_setSampleFrequency( X.slot, freq );
    if( err != SUCCESS ) {
        return
        QString("waveplayer_setSampleFrequency(slot %1, %2)%3")
        .arg( X.slot ).arg( freq ).arg( obx_errorString( err ) );
    }

// Load buffer

    err = np_waveplayer_writeBuffer( X.slot, src, len );
    if( err != SUCCESS ) {
        return
        QString("waveplayer_writeBuffer(slot %1)%2")
        .arg( X.slot ).arg( obx_errorString( err ) );
    }
#endif

    return QString();
}


// trig table:
// -2  : software
// -1  : SMA1
// 0-11: ADC0-11
//
// Note:
// There is one AI voltage range setting for all XA channels!
// If user is triggering waveplayer using an ADC channel, they
// should select AI voltage range compatible with trigger signal.
//
QString CStim::obx_wave_arm( int istr, int trig, bool loop )
{
#ifdef HAVE_IMEC
    OBX             X( istr );
    QVector<uint>   vCH;
    NP_ErrorCode    err;

// Check AO-0 available

    Subset::rngStr2Vec( vCH, X.AOStr() );

    if( !vCH.contains( 0 ) ) {
        return
        QString("Waveplayer channel 0 not enabled on Obx %1 slot %2.")
        .arg( istr ).arg( X.slot );
    }

// Check AI-trig available

    if( trig >= 0 ) {

        Subset::rngStr2Vec( vCH, X.AIStr() );
        if( !vCH.contains( trig ) ) {
            return
            QString("Trigger channel %1 not enabled on Obx %2 slot %3.")
            .arg( trig ).arg( istr ).arg( X.slot );
        }
    }

// Zero voltage on waveplayer channel

    err = np_DAC_setVoltage( X.slot, 0, 0 );
    if( err != SUCCESS ) {
        return
        QString("DAC_setVoltage(slot %1, 0, 0)%2")
        .arg( X.slot ).arg( obx_errorString( err ) );
    }

// Set trigger source

    switchmatrixinput_t trigSrc = SM_Input_SWTrigger2;

    switch( trig ) {
        case -1: trigSrc = SM_Input_SMA1; break;
        case  0: trigSrc = SM_Input_ADC0; break;
        case  1: trigSrc = SM_Input_ADC1; break;
        case  2: trigSrc = SM_Input_ADC2; break;
        case  3: trigSrc = SM_Input_ADC3; break;
        case  4: trigSrc = SM_Input_ADC4; break;
        case  5: trigSrc = SM_Input_ADC5; break;
        case  6: trigSrc = SM_Input_ADC6; break;
        case  7: trigSrc = SM_Input_ADC7; break;
        case  8: trigSrc = SM_Input_ADC8; break;
        case  9: trigSrc = SM_Input_ADC9; break;
        case 10: trigSrc = SM_Input_ADC10; break;
        case 11: trigSrc = SM_Input_ADC11; break;
        default: break;
    }

    err = np_switchmatrix_clear( X.slot, SM_Output_WavePlayerTrigger );
    if( err != SUCCESS ) {
        return
        QString("switchmatrix_clear(slot %1)%2")
        .arg( X.slot ).arg( obx_errorString( err ) );
    }

    err = np_switchmatrix_set(
            X.slot, SM_Output_WavePlayerTrigger, trigSrc, true );
    if( err != SUCCESS ) {
        return
        QString("switchmatrix_set(slot %1)%2")
        .arg( X.slot ).arg( obx_errorString( err ) );
    }

// Set trigger edge

    err = np_switchmatrix_setOutputTriggerEdge(
            X.slot, SM_Output_WavePlayerTrigger, triggeredge_rising );
    if( err != SUCCESS ) {
        return
        QString("np_switchmatrix_setOutputTriggerEdge(slot %1)%2")
        .arg( X.slot ).arg( obx_errorString( err ) );
    }

// Arm

    err = np_waveplayer_arm( X.slot, !loop );
    if( err != SUCCESS ) {
        return
        QString("waveplayer_arm(slot %1, %2)%3")
        .arg( X.slot ).arg( !loop ).arg( obx_errorString( err ) );
    }
#endif

    return QString();
}


QString CStim::obx_wave_start_stop( int istr, bool start )
{
#ifdef HAVE_IMEC
    OBX             X( istr );
    NP_ErrorCode    err;

    if( start ) {
        err = np_setSWTriggerEx( X.slot, swtrigger2 );
        if( err != SUCCESS ) {
            return
            QString("setSWTriggerEx(slot %1)%2")
            .arg( X.slot ).arg( obx_errorString( err ) );
        }
    }
    else {
        err = np_waveplayer_arm( X.slot, true );
        if( err != SUCCESS ) {
            return
            QString("waveplayer_arm(slot %1)%2")
            .arg( X.slot ).arg( obx_errorString( err ) );
        }
    }
#endif

    return QString();
}

/* ---------------------------------------------------------------- */
/* OBX DAC -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// In:  list: (AO-index,voltage)()...
// Out: error msg or empty.
//
QString CStim::obx_set_AO( int istr, const QString &chn_vlt )
{
// Decode slot, available AO

    OBX             X( istr );
    QVector<uint>   vAO;

    Subset::rngStr2Vec( vAO, X.AOStr() );

// Parse into ()

    QStringList prn  = chn_vlt.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         nprn = prn.size();

    if( !nprn ) {
        return
        QString("No (chan,voltage) pairs given for Obx %1 slot %2.")
        .arg( istr ).arg( X.slot );
    }

// Parse each ()

    QVector<double> vlt;
    QVector<int>    chn;

    foreach( const QString &s, prn ) {

        QStringList val = s.split(
                            QRegExp("^\\s+|\\s*,\\s*"),
                            QString::SkipEmptyParts );

        if( val.size() != 2 ) {
            return
            QString("Malformed chan-voltage pair on Obx %1 slot %2.")
            .arg( istr ).arg( X.slot );
        }

        bool    okv, okc;
        double  v = val[1].toDouble( &okv );
        int     c = val[0].toInt( &okc );

        if( !okv || v < -5.0 || v > 5.0 ) {
            return
            QString("Voltage '%1' not in range [-5,5] on Obx %2 slot %3.")
            .arg( val[1] ).arg( istr ).arg( X.slot );
        }

        if( !okc || !vAO.contains( c ) ) {
            return
            QString("Channel '%1' not enabled on Obx %2 slot %3.")
            .arg( val[0] ).arg( istr ).arg( X.slot );
        }

        vlt.push_back( v );
        chn.push_back( c );
    }

// Set values

#ifdef HAVE_IMEC
    for( int ip = 0; ip < nprn; ++ip ) {

        NP_ErrorCode err = np_DAC_setVoltage( X.slot, chn[ip], vlt[ip] );

        if( err != SUCCESS ) {
            return
            QString("DAC_setVoltage(slot %1, chn %2, %3)%4")
            .arg( X.slot ).arg( chn[ip] ).arg( vlt[ip] )
            .arg( obx_errorString( err ) );
        }
    }
#endif

    return QString();
}

/* ---------------------------------------------------------------- */
/* NI waveplayer client ------------------------------------------- */
/* ---------------------------------------------------------------- */

QString CStim::ni_wave_download_file(
    const QString   &outChan,
    const QString   &wave,
    bool            loop )
{
    WaveMeta    W;
    QString     err = W.readMetaFile( wave );

    if( !err.isEmpty() )
        return err;

    if( W.freq > OBX_WAV_FRQ_MAX ) {
        return
        QString("Wave.read: Sample frequency exceeds %1 in file '%2'.")
        .arg( OBX_WAV_FRQ_MAX ).arg( wave );
    }
    if( W.nsamp & 1 ) {
        return
        QString("Wave.read: Odd sample count %1 in file '%2'.")
        .arg( W.nsamp ).arg( wave );
    }
    if( W.nsamp > OBX_WAV_BUF_MAX ) {
        return
        QString("Wave.read: Sample count exceeds %1 in file '%2'.")
        .arg( OBX_WAV_BUF_MAX ).arg( wave );
    }

    if( W.isbin )
        return ni_wave_download_binFile( outChan, W, wave, loop );
    else
        return ni_wave_download_txtFile( outChan, W, wave, loop );
}


QString CStim::ni_wave_download_binFile(
    const QString   &outChan,
    const WaveMeta  &W,
    const QString   &wave,
    bool            loop )
{
// Open binary

    QString path = QString("%1/_Waves/%2.bin").arg( appPath() ).arg( wave );
    QFile   f( path );

    if( !f.open( QIODevice::ReadOnly ) ) {
        return
        QString("Waveplayer can't open wave binary file '%1'.")
        .arg( path );
    }

// Read binary

    std::vector<float>  fbuf( W.nsamp );
    float   *pf = &fbuf[0];

    if( !f.read( (char*)pf, W.nsamp*sizeof(float) ) ) {
        return
        QString("Waveplayer can't read wave binary file '%1'.")
        .arg( path );
    }

// Scale binary

    vec_i16 buf( W.nsamp );
    qint16  *pi     = &buf[0];
    double  scale   = 0x7FFF * W.wav_vpp / W.dev_vpp;

    for( int i = 0; i < W.nsamp; ++i, ++pf, ++pi )
        *pi = scale * *pf;

// Download binary

    return ni_wave_download_buf( outChan, W, &buf[0], loop );
}


QString CStim::ni_wave_download_txtFile(
    const QString   &outChan,
    WaveMeta        &W,
    const QString   &wave,
    bool            loop )
{
    QString text, err;
    vec_i16 buf;

    err = W.readTextFile( text, wave );
    if( !err.isEmpty() ) return err;

    err = W.parseText( buf, text, OBX_WAV_BUF_MAX );
    if( !err.isEmpty() ) return err;

    err = W.parseText( buf, text, 0 );
    if( !err.isEmpty() ) return err;

    return ni_wave_download_buf( outChan, W, &buf[0], loop );
}

/* ---------------------------------------------------------------- */
/* NI waveplayer hardware ----------------------------------------- */
/* ---------------------------------------------------------------- */

QString CStim::ni_wave_download_buf(
    const QString   &outChan,
    const WaveMeta  &W,
    const qint16    *src,
    bool            loop )
{
    ni_removeTask( outChan );

#ifdef HAVE_NIDAQmx
    TaskHandle  T;
    int32       nWritten;

    if( DAQmxErrChkNoJump( DAQmxCreateTask( "", &T ) )
     || DAQmxErrChkNoJump( DAQmxCreateAOVoltageChan(
                            T,
                            STR2CHR( outChan ),
                            "",
                            -W.dev_vpp,
                            W.dev_vpp,
                            DAQmx_Val_Volts,
                            NULL ) )
     || DAQmxErrChkNoJump( DAQmxCfgSampClkTiming(
                            T,
                            "",
                            W.freq,
                            DAQmx_Val_Rising,
                            (loop ? DAQmx_Val_ContSamps : DAQmx_Val_FiniteSamps),
                            W.nsamp ) )
     || DAQmxErrChkNoJump( DAQmxWriteBinaryI16(
                            T,
                            W.nsamp,
                            false,  // autostart
                            0,      // timeout seconds
                            DAQmx_Val_GroupByChannel,
                            src,
                            &nWritten,
                            NULL ) ) ) {

        QString e = ni_getError();
        ni_destroyTask( T );
        return e;
    }

    ni_setTask( outChan, T );
#endif

    return QString();
}


// outChan:  e.g. "dev/aoX"
// trigTerm: e.g. "/dev/pfiX"       -> hardware trigger
//           if missing initial '/' -> software trigger
//
QString CStim::ni_wave_arm(
    const QString   &outChan,
    const QString   &trigTerm )
{
#ifdef HAVE_NIDAQmx
    TaskHandle  T = ni_outChan2task( outChan );

    if( !T )
        return QString("ni_wave_arm bad outChan '%1'.").arg( outChan );

    if( trigTerm.startsWith( "/" ) ) {

        if( DAQmxErrChkNoJump( DAQmxCfgDigEdgeStartTrig(
                                T,
                                STR2CHR(trigTerm),
                                DAQmx_Val_Rising ) )
        ||  DAQmxErrChkNoJump( DAQmxStartTask( T ) ) ) {

            return ni_getError();
        }
    }
    else {
        if( DAQmxErrChkNoJump( DAQmxDisableStartTrig( T ) )
        ||  DAQmxErrChkNoJump( DAQmxTaskControl(
                                T,
                                DAQmx_Val_Task_Commit ) ) ) {

            return ni_getError();
        }
    }
#endif

    return QString();
}


QString CStim::ni_wave_start_stop(
    const QString   &outChan,
    bool            start )
{
#ifdef HAVE_NIDAQmx
    TaskHandle  T = ni_outChan2task( outChan );

    if( !T )
        return QString("ni_wave_start_stop bad outChan '%1'.").arg( outChan );

    if( start ) {
        if( DAQmxErrChkNoJump( DAQmxStartTask( T ) ) )
            return ni_getError();
    }
    else {
        if( DAQmxErrChkNoJump( DAQmxStopTask( T ) ) )
            return ni_getError();
    }
#endif

    return QString();
}


void CStim::ni_run_cleanup()
{
#ifdef HAVE_NIDAQmx
    QMap<QString,TaskHandle>::iterator
        it  = gNITasks.begin(),
        end = gNITasks.end();

    for( ; it != end; ++it ) {
        ni_destroyTask( it.value() );
        ni_set1AnalogValue( it.key(), 0 );
    }

    gNITasks.clear();
#endif
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString CStim::obx_errorString( int err )
{
#ifdef HAVE_IMEC
    char    buf[2048];
    size_t  n = np_getLastErrorMessage( buf, sizeof(buf) );

    if( n >= sizeof(buf) )
        n = sizeof(buf) - 1;

    buf[n] = 0;

    return QString(" error %1 '%2'.").arg( err ).arg( QString(buf) );
#else
    Q_UNUSED( err )
    return QString();
#endif
}


