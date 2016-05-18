#ifndef SUBSET_H
#define SUBSET_H

#include "SGLTypes.h"

#include <QBitArray>
#include <QString>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Subset
{
public:
    Subset() {}

    static void bits2Vec( QVector<uint> &v, const QBitArray &b );
    static void vec2Bits( QBitArray &b, const QVector<uint> &v );
    static void canonVec( QVector<uint> &vo, const QVector<uint> &vi );

    static void defaultBits( QBitArray &b, int nChans );
    static void defaultVec( QVector<uint> &v, int nChans );

    static bool isAllChansStr( const QString &s );

    static QString cmdStr2Bits(
        QBitArray       &chanBits,
        const QBitArray &allBits,
        const QString   &s,
        int             nTotalBits );

    static QString bits2Str( const QBitArray &b );
    static QString bits2RngStr( const QBitArray &b );
    static QString vec2RngStr( const QVector<uint> &v );
    static bool rngStr2Bits( QBitArray &b, const QString &s );
    static bool rngStr2Vec( QVector<uint> &v, const QString &s );

    static void subset(
        vec_i16             &dst,
        vec_i16             &src,
        const QVector<uint> &iKeep,
        int                 nchans );

    static uint downsample(
        vec_i16         &dst,
        vec_i16         &src,
        int             nchans,
        int             dnsmp );

    static uint downsampleNeural(
        vec_i16         &dst,
        vec_i16         &src,
        int             nchans,
        int             dnsmp );
};

#endif  // SUBSET_H


