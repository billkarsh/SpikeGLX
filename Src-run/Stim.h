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
    QString readMetaFile( const QString &fmeta );
    QString writeMetaFile( const QString &fmeta ) const;
    QString readTextFile( QString &text, const QString &fmeta );
    QString writeTextFile( const QString &fmeta, QString &text );
    QString parseText( vec_i16 &buf, const QString &text, int testMax );
};

class CStim
{
public:
// Debug

    static void stimteststart();

// OBX waveplayer client

    static QString obx_wave_download_file( int istr, const QString &fmeta );
    static QString obx_wave_download_binFile(
        int             istr,
        const WaveMeta  &W,
        const QString   &fmeta );
    static QString obx_wave_download_txtFile(
        int             istr,
        WaveMeta        &W,
        const QString   &fmeta );

// OBX waveplayer hardware

    static QString obx_wave_download_buf(
        double          freq,
        int             istr,
        int             len,
        const qint16    *src );
    static QString obx_wave_arm( int istr, int trig, bool loop );
    static QString obx_wave_start_stop( int istr, bool start );

// DAC

    static QString obx_set_AO( int istr, const QString &chn_vlt );

private:
    static QString makeErrorString( int err );
};

#endif  // STIM_H


