#ifndef STIM_H
#define STIM_H

#include "SGLTypes.h"

#include <QString>

#define OBX_WAV_FRQ_MAX 500000.0
#define OBX_WAV_BUF_MAX 16777214

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct WaveMeta {
    double  freq,
            wav_vpp,
            dev_vpp;
    int     nsamp;
    bool    isbin;
    QString readMetaFile( const QString &wave );
    QString writeMetaFile( const QString &wave ) const;
    QString readTextFile( QString &text, const QString &wave );
    QString writeTextFile( const QString &wave, QString &text );
    QString parseText( vec_i16 &buf, const QString &text, int testMax );
};

class CStim
{
public:
// Debug

    static void stimteststart();

// OBX waveplayer client

    static QString obx_wave_download_file( int istr, const QString &wave );
    static QString obx_wave_download_binFile(
        int             istr,
        const WaveMeta  &W,
        const QString   &wave );
    static QString obx_wave_download_txtFile(
        int             istr,
        WaveMeta        &W,
        const QString   &wave );

// OBX waveplayer hardware

    static QString obx_wave_download_buf(
        double          freq,
        int             istr,
        int             len,
        const qint16    *src );
    static QString obx_wave_arm( int istr, int trig, bool loop );
    static QString obx_wave_start_stop( int istr, bool start );

// OBX DAC

    static QString obx_set_AO( int istr, const QString &chn_vlt );

// NI waveplayer client

    static QString ni_wave_download_file(
        const QString   &outChan,
        const QString   &wave,
        bool            loop );
    static QString ni_wave_download_binFile(
        const QString   &outChan,
        const WaveMeta  &W,
        const QString   &wave,
        bool            loop );
    static QString ni_wave_download_txtFile(
        const QString   &outChan,
        WaveMeta        &W,
        const QString   &wave,
        bool            loop );

// NI waveplayer hardware

    static QString ni_wave_download_buf(
        const QString   &outChan,
        const WaveMeta  &W,
        const qint16    *src,
        bool            loop );
    static QString ni_wave_arm(
        const QString   &outChan,
        const QString   &trigTerm );
    static QString ni_wave_start_stop(
        const QString   &outChan,
        bool            start );

    static void ni_run_cleanup();

private:
    static QString obx_errorString( int err );
};

#endif  // STIM_H


