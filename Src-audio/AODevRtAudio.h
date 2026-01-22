#ifndef AODEVRTAUDIO_H
#define AODEVRTAUDIO_H

#include <qglobal.h>
#ifdef Q_OS_WIN

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
    double      latSum;
    int         latCt;
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
    virtual bool readyForScans() const  {return ready;}
    virtual bool devStart(
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &imQf,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ );
    virtual void devStop();

private:
    void filter(
        qint16  *data,
        int     ntpts,
        int     nChan,
        int     ichan );

    void latency();

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

#endif	// Q_OS_WIN

#endif  // AODEVRTAUDIO_H


