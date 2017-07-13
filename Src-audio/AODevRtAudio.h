#ifndef AODEVRTAUDIO_H
#define AODEVRTAUDIO_H

#include "AODevBase.h"
#include "RtAudio.h"
#include "AIQ.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// RtAudio-based audio output
//
class AODevRtAudio : public AODevBase
{
private:
    RtAudio     *rta;
    quint64     fromCt;
    bool        ready;

public:
    AODevRtAudio( AOCtl *aoC, const DAQ::Params &p );
    virtual ~AODevRtAudio();

    // Development tests
    virtual void test1();
    virtual void test2();
    virtual void test3();

    // Device api
    virtual int getOutChanCount( QString &err );
    virtual bool doAutoStart();
    virtual bool readyForScans()    {return ready;}
    virtual bool devStart( const AIQ *imQ, const AIQ *niQ );
    virtual void devStop();

private:
    void filter(
        qint16  *data,
        int     ntpts,
        int     nChan,
        int     ichan );

    static int callbackMono(
        void                *outputBuffer,
        void                *inputBuffer,
        uint                nBufferFrames,
        double              streamTime,
        RtAudioStreamStatus status,
        void                *userData );

    static int callbackStereo(
        void                *outputBuffer,
        void                *inputBuffer,
        uint                nBufferFrames,
        double              streamTime,
        RtAudioStreamStatus status,
        void                *userData );
};

#endif  // AODEVRTAUDIO_H


