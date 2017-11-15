#ifndef CALSRATE_H
#define CALSRATE_H

#include "DAQ.h"

#include <QObject>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class CalSRate : public QObject
{
    Q_OBJECT

private:
    DAQ::Params oldParams;

public:
    CalSRate();

public slots:
    void finish();

private:
    void CalcRateIM( double &av, double &se, const DAQ::Params &p );
    void CalcRateNI( double &av, double &se, const DAQ::Params &p );
};

#endif  // CALSRATE_H


