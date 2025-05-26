#ifndef SVYPRB_H
#define SVYPRB_H

#include "DAQ.h"

#include <QObject>

class DataFile;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SvySBTT
{
    qint64  t1, t2; // transition start, end
    int     s,  b;  // new shank, bank

    SvySBTT() : t1(0), t2(0), s(0), b(0)    {}
    SvySBTT( int s, int b, qint64 t1, qint64 t2 )
        :   t1(t1), t2(t2), s(s), b(b)      {}
    static SvySBTT fromString( const QString &s_in );
};


struct SvyVSBTT
{
    int                     maxbank,
                            nmaps;
    std::vector<SvySBTT>    e;

    SvyVSBTT() : nmaps(0)   {}
    void fromMeta( const DataFile *df );
};


class SvyPrbRun : public QObject
{
    Q_OBJECT

private:
    DAQ::Params         oldParams;
    std::vector<int>    vCurShnk,
                        vCurBank;
    QVector<QString>    vSBTT;
    int                 irunbank,
                        nrunbank;

public:
    SvyPrbRun() : QObject(0), irunbank(0), nrunbank(0) {}

    void initRun();
    int msPerBnk( bool first = false );
    bool nextBank();

public slots:
    void finish();
};

#endif  // SVYPRB_H


