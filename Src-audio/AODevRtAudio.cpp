
#include "AOCtl.h"
#include "AODevRtAudio.h"
#include "Util.h"

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static AODevRtAudio *ME;

/* ---------------------------------------------------------------- */
/* AODevRtAudio --------------------------------------------------- */
/* ---------------------------------------------------------------- */

AODevRtAudio::AODevRtAudio( AOCtl *aoC, const DAQ::Params &p )
    :   AODevBase( aoC, p ), rta(0), ready(false)
{
}


AODevRtAudio::~AODevRtAudio()
{
    devStop();
}

/* ---------------------------------------------------------------- */
/* Development tests  --------------------------------------------- */
/* ---------------------------------------------------------------- */

void AODevRtAudio::test1()
{
    RtAudio *audio = 0;

// Init

    Log();

    try {
        audio = new RtAudio;
    }
    catch( RtAudioError &e ) {
        Log() << e.what();
        return;
    }

    Log() << "API: " << audio->getCurrentApi();

// Determine the number of devices available

    int devices = audio->getDeviceCount();

// Scan through devices for various capabilities

    RtAudio::DeviceInfo info;

    for( int i = 0; i < devices; ++i ) {

        try {
            info = audio->getDeviceInfo( i );
        }
        catch( RtAudioError &e ) {
            Log() << e.what();
            break;
        }

        if( info.probed ) {
            // Print, for example, max output channels for each device
            Log() << "device = " << i << ": " << info.name.c_str();
            Log() << "maximum output channels = " << info.outputChannels;
            Log() << "default " << info.isDefaultOutput;
            Log() << "format " << info.nativeFormats;

            int n = info.sampleRates.size();

            for( int k = 0; k < n; ++k )
                Log() << info.sampleRates[k];
            Log();
        }
    }

// Clean up
    delete audio;
}


// Two-channel sawtooth wave generator for FLOAT64 data.
//
static int saw(
    void                *outputBuffer,
    void                *inputBuffer,
    uint                nBufferFrames,
    double              streamTime,
    RtAudioStreamStatus status,
    void                *userData )
{
    Q_UNUSED( inputBuffer )
    Q_UNUSED( streamTime )

    double  *buffer = (double*)outputBuffer;
    double  *lastValues = (double*)userData;

    if( status )
        Log() << "Stream underflow detected!";

#define AMP   1.0

    // Write interleaved audio data.
    for( uint i = 0; i < nBufferFrames; ++i ) {

        for( uint j = 0; j < 2; ++j ) {

            *buffer++ = lastValues[j];

            lastValues[j] += AMP/200 * (j+1+(j*0.1));

            if( lastValues[j] >= AMP )
                lastValues[j] -= 2*AMP;
        }
    }

    return 0;
}


// Two-channel sawtooth wave generator for FLOAT64 data.
//
void AODevRtAudio::test2()
{
    RtAudio dac;

    Log();
    Log() << "API: " << dac.getCurrentApi();

    if( dac.getDeviceCount() < 1 ) {
        Log() << "No audio devices.";
        return;
    }

    RtAudio::StreamParameters   prm;

    prm.deviceId            = dac.getDefaultOutputDevice();
    prm.nChannels           = 2;
    prm.firstChannel        = 0;
    uint    sampleRate      = 44100;
    uint    bufferFrames    = 256; // 256 sample frames
    double  data[2];

    try {
        dac.openStream( &prm, NULL, RTAUDIO_FLOAT64,
            sampleRate, &bufferFrames, &saw, (void*)&data );
        dac.startStream();
    }
    catch( RtAudioError &e ) {
        Log() << e.what();
        return;
    }

    Log(); guiBreathe();

    msleep( 5000 );

    try {
        // Stop the stream
        dac.stopStream();
    }
    catch( RtAudioError &e ) {
        Log() << e.what();
    }

    if( dac.isStreamOpen() )
        dac.closeStream();
}


// Two-channel sawtooth wave generator for SINT16 data.
//
static int saw16(
    void                *outputBuffer,
    void                *inputBuffer,
    uint                nBufferFrames,
    double              streamTime,
    RtAudioStreamStatus status,
    void                *userData )
{
    Q_UNUSED( inputBuffer )
    Q_UNUSED( streamTime )

    qint16  *buffer = (qint16*)outputBuffer;
    qint16  *lastValues = (qint16*)userData;

    if( status )
        Log() << "Stream underflow detected!";

#define A   32767

    // Write interleaved audio data.
    for( uint i = 0; i < nBufferFrames; ++i ) {

        for( uint j = 0; j < 2; ++j ) {

            *buffer++ = lastValues[j];

            lastValues[j] += A/200 * (j+1+(j*0.1));

            if( lastValues[j] >= A )
                lastValues[j] -= 2*A;
        }
    }

    return 0;
}


// Two-channel sawtooth wave generator for SINT16 data.
//
void AODevRtAudio::test3()
{
    RtAudio dac;

    Log();
    Log() << "API: " << dac.getCurrentApi();

    if( dac.getDeviceCount() < 1 ) {
        Log() << "No audio devices.";
        return;
    }

    RtAudio::StreamParameters   prm;

    prm.deviceId            = dac.getDefaultOutputDevice();
    prm.nChannels           = 2;
    prm.firstChannel        = 0;
    uint    sampleRate      = 44100;
    uint    bufferFrames    = 256; // 256 sample frames
    qint16  data[2];

    try {
        dac.openStream( &prm, NULL, RTAUDIO_SINT16,
            sampleRate, &bufferFrames, &saw16, (void*)&data );
        dac.startStream();
    }
    catch( RtAudioError &e ) {
        Log() << e.what();
        return;
    }

    Log(); guiBreathe();

    msleep( 5000 );

    try {
        // Stop the stream
        dac.stopStream();
    }
    catch( RtAudioError &e ) {
        Log() << e.what();
    }

    if( dac.isStreamOpen() )
        dac.closeStream();
}

/* ---------------------------------------------------------------- */
/* Device api  ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// BK: Initially I am supporting DirectSound only:
// The latency is ~100 ms for nidq (120 ms for imec)
// which is about the same as WASAPI, but WASAPI has
// occasional artifacts (every ~6 seconds or so) even
// after I fixed the WASAPI interpolation in RtAudio.
// Also, DirectSound is supported in Windows XP..10.
//
// Testing with ASIO4All was problematic. SpikeGLX
// sample rates are simply not supported by ASIO,
// at least not through the RtAudio API. Simulating
// sample rates that ARE supported produced much
// more crackle and hum than DirectSound.
//
int AODevRtAudio::getOutChanCount( QString &err )
{
    RtAudio *audio;

    try {
        audio = new RtAudio( RtAudio::WINDOWS_DS );
    }
    catch( RtAudioError &e ) {
        err = QString("Audio error: %1").arg( e.what() );
        return 0;
    }

// Determine the number of devices available

    int ndev    = audio->getDeviceCount(),
        nchan   = 0;

// Scan through devices for capabilities

    for( int idev = 0; idev < ndev; ++idev ) {

        RtAudio::DeviceInfo info;

        try {
            info = audio->getDeviceInfo( idev );
        }
        catch( RtAudioError &e ) {
            err = QString("Audio error: %1").arg( e.what() );
            break;
        }

        if( info.probed && info.isDefaultOutput ) {

            nchan = info.outputChannels;
            break;
        }
    }

    delete audio;

    return nchan;
}


bool AODevRtAudio::doAutoStart()
{
    if( aoC->usr.autoStart ) {

        QString err;

        if( aoC->valid( err ) )
            return true;
        else
            Error() << "Audio autostart failed: [" << err << "].";
    }

    return false;
}


// Latency as a function of sampPerCall
// ====================================
// LH = real hardware latency on scope
// LM = software latency metric
//
// Simulating 4 imec probes:
// -------------------------
// spc = 256    LM growth extreme
// spc = 300    LM growth large
// spc = 400    LM growth slow
// spc = 512    LM small and stable
//
// One real imec probe (Win 8.1):
// ------------------------------
// spc = 192    LH growth extreme
// spc = 200    LH 130 ms
// spc = 256    LH 135 ms (auto reset @ LM = 60) <- select
// spc = 512    LH 150 ms (auto reset @ LM = 50)
// spc = 1024   LH 240 ms
//
// Nidq with USB-6366 (Win 7):
// ---------------------------
// spc = 128    LH growth extreme
// spc = 160    LH growth large
// spc = 192    LH growth slow
// spc = 200    LH 90  ms (auto reset @ LM = 20)
// spc = 256    LH 110 ms (auto reset @ LM = 20) <- select
// spc = 512    LH 150 ms (auto reset @ LM = 10)
// spc = 1024   LH 260 ms
//
bool AODevRtAudio::devStart( const AIQ *imQ, const AIQ *niQ )
{
// Connect to driver

    try {
        rta = new RtAudio( RtAudio::WINDOWS_DS );
    }
    catch( RtAudioError &e ) {
        Error() << "Audio error: " << e.what();
        return false;
    }

    if( rta->getDeviceCount() < 1 ) {
        Error() << "Audio error: No audio devices.";
        return false;
    }

// Init local data

    AOCtl::Derived  &drv = aoC->drv;

    drv.usr2drv( aoC );

    ME          = this;
    this->aiQ   = (drv.streamID >= 0 ? imQ : niQ);
    fromCt      = 0;
    latSum      = 0.0;
    latCt       = 0;

    RtAudio::StreamParameters   prm;

    prm.deviceId        = rta->getDefaultOutputDevice();
    prm.nChannels       = aoC->nDevChans;
    prm.firstChannel    = 0;

    uint sampPerCall    = 256;

// Start audio stream

    try {
        rta->openStream(
                &prm, NULL, RTAUDIO_SINT16, drv.srate, &sampPerCall,
                (prm.nChannels == 2 ? callbackStereo : callbackMono) );

        rta->startStream();
    }
    catch( RtAudioError &e ) {
        Error() << "Audio error: " << e.what();
        return false;
    }

// Done

    ready = true;

    return true;
}


void AODevRtAudio::devStop()
{
    ready = false;

    if( rta ) {

        try {

            if( rta->isStreamRunning() )
                rta->stopStream();

            if( rta->isStreamOpen() )
                rta->closeStream();
        }
        catch( RtAudioError &e ) {
            Error() << "Audio error: " << e.what();
        }

        delete rta;
        rta = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// nChan is either {1,2}.
// iChan is either {0,1}.
//
void AODevRtAudio::filter(
    qint16  *data,
    int     ntpts,
    int     nChan,
    int     ichan )
{
    AOCtl::Derived  &drv = aoC->drv;

    if( drv.loCut > -1 || drv.hiCut > -1 ) {

        if( drv.loCut > -1 ) {
            drv.hipass.apply1BlockwiseMem1(
                data, drv.maxBits, ntpts, nChan, ichan );
        }

        if( drv.hiCut > -1 ) {
            drv.lopass.apply1BlockwiseMem1(
                data, drv.maxBits, ntpts, nChan, ichan );
        }
    }
}


void AODevRtAudio::latency()
{
    const AOCtl::Derived    &drv = aoC->drv;

    double L = qMax( 0.0, 1000 * (aiQ->endCount() - fromCt) / drv.srate );

    latSum += L;
    ++latCt;

// Average about 2 sec worth: 2 sec = N*256/30000; N ~ 200

    if( latCt < 200 )
        return;

    L       = latSum / latCt;
    latSum  = 0.0;
    latCt   = 0;

    if( L >= drv.maxLatency )
        aoC->restart();

//    Log() << L;
}


int AODevRtAudio::callbackMono(
    void                *outputBuffer,
    void                *inputBuffer,
    uint                nBufferFrames,
    double              streamTime,
    RtAudioStreamStatus status,
    void                *userData )
{
    Q_UNUSED( inputBuffer )
    Q_UNUSED( streamTime )
    Q_UNUSED( userData )

    if( status )
        Debug() << "Audio stream underflow detected.";

    QMutexLocker    ml( &ME->aoC->aoMtx );

    const AOCtl::Derived    &drv = ME->aoC->drv;
    qint16                  *dst = (qint16*)outputBuffer;
    qint64                  headCt;

// Fetch data from stream

    for( int tries = 0; tries < 100; ++tries ) {

        // Fetch

        if( !ME->fromCt ) {
            headCt = ME->aiQ->getNewestNScansMono(
                            dst, nBufferFrames, drv.lChan );
        }
        else {
            headCt = ME->aiQ->getNScansFromCtMono(
                        dst, ME->fromCt, nBufferFrames, drv.lChan );
        }

        if( headCt >= 0 )
            goto fetched;

        usleep( 1000 );  // wait ~30 samples
    }

// Fetch failed

    Warning() << "Audio getting no samples.";

#if 0
    // Stopping from within callback crashes sometimes...
    return 1;
#else
    // Stop from outside instead.
    memset( dst, 0, nBufferFrames*sizeof(qint16) );
    ME->aoC->restart();
#endif

// Mark next fetch point

fetched:
    ME->fromCt = headCt + nBufferFrames;

// Latency

    ME->latency();

// Filter channels

    if( drv.lChan < drv.nNeural )
        ME->filter( dst, nBufferFrames, 1, 0 );

// Apply volume

    for( uint t = 0; t < nBufferFrames; ++t ) {

        dst[t] = drv.vol( dst[t], drv.lVol );
    }

    return 0;
}


int AODevRtAudio::callbackStereo(
    void                *outputBuffer,
    void                *inputBuffer,
    uint                nBufferFrames,
    double              streamTime,
    RtAudioStreamStatus status,
    void                *userData )
{
    Q_UNUSED( inputBuffer )
    Q_UNUSED( streamTime )
    Q_UNUSED( userData )

    if( status )
        Debug() << "Audio stream underflow detected.";

    QMutexLocker    ml( &ME->aoC->aoMtx );

    const AOCtl::Derived    &drv = ME->aoC->drv;
    qint16                  *dst = (qint16*)outputBuffer;
    qint64                  headCt;

// Fetch data from stream

    for( int tries = 0; tries < 100; ++tries ) {

        // Fetch

        if( !ME->fromCt ) {
            headCt = ME->aiQ->getNewestNScansStereo(
                            dst, nBufferFrames,
                            drv.lChan, drv.rChan );
        }
        else {
            headCt = ME->aiQ->getNScansFromCtStereo(
                        dst, ME->fromCt, nBufferFrames,
                        drv.lChan, drv.rChan );
        }

        if( headCt >= 0 )
            goto fetched;

        usleep( 1000 );  // wait ~30 samples
    }

// Fetch failed

    Warning() << "Audio getting no samples.";

#if 0
    // Stopping from within callback crashes sometimes...
    return 1;
#else
    // Stop from outside instead.
    memset( dst, 0, 2*nBufferFrames*sizeof(qint16) );
    ME->aoC->restart();
#endif

// Mark next fetch point

fetched:
    ME->fromCt = headCt + nBufferFrames;

// Latency

    ME->latency();

// Filter channels

    if( drv.lChan < drv.nNeural )
        ME->filter( dst, nBufferFrames, 2, 0 );

    if( drv.rChan < drv.nNeural )
        ME->filter( dst, nBufferFrames, 2, 1 );

// Apply volume

    for( uint t = 0; t < nBufferFrames; ++t ) {

        int j = 2*t;

        dst[j]      = drv.vol( dst[j], drv.lVol );
        dst[j+1]    = drv.vol( dst[j+1], drv.rVol );
    }

    return 0;
}


