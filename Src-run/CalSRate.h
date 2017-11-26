#ifndef CALSRATE_H
#define CALSRATE_H

#include "DAQ.h"

#include <QObject>

class DataFile;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class CalSRate : public QObject
{
    Q_OBJECT

private:
    struct Bin {
        QVector<double> C;
        qint64          bmin,
                        bmax;
        int             n;

        // We expect count variation of order +/- 1.
        // True glitches that split a counting window cause
        // large variation: +/- 1000 or more. We set nominal
        // width to +/- 20 to accommodate modest clock drift.
        Bin() : bmin(0), bmax(0), n(0)                  {}
        Bin( qint64 c ) : bmin(c-20), bmax(c+20), n(1)  {C.push_back(c);}
        bool isIn( qint64 c )
        {
            if( c >= bmin && c <= bmax ) {
                C.push_back( c );
                ++n;
                return true;
            }
            return false;
        }
    };

private:
    DAQ::Params oldParams;
    double      calRunTZero;

public:
    void fromArbFile( const QString &file );
    void initCalRun();
    void initCalRunTimer();
    int calRunElapsedMS();

public slots:
    void finishCalRun();

private:
    void doCalRunFile(
        double          &av,
        double          &se,
        QString         &result,
        const QString   &runName,
        const QString   &stream );

    void CalcRateIM(
        QString         &err,
        double          &av,
        double          &se,
        const QString   &file );

    void CalcRateNI(
        QString         &err,
        double          &av,
        double          &se,
        const QString   &file );

    void scanDigital(
        QString         &err,
        double          &av,
        double          &se,
        DataFile        *df,
        double          syncPer,
        int             syncChan,
        int             dword );

    void scanAnalog(
        QString         &err,
        double          &av,
        double          &se,
        DataFile        *df,
        double          syncPer,
        double          syncThresh,
        int             syncChan );
};

#endif  // CALSRATE_H


