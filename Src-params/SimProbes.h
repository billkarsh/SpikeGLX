#ifndef SIMPROBES_H
#define SIMPROBES_H

#include "IMROTbl.h"

#include <QMap>
#include <QSet>


struct SPAddr {
    int     s, p, d, e;
    SPAddr() : s(0), p(0), d(0), e(0)                   {}
    SPAddr( int s, int p, int d, int e )
        :   s(s), p(p), d(d), e(e)                      {}
    SPAddr( const PAddr& adr, int e )
        :   s(adr.slot), p(adr.port), d(adr.dock), e(e) {}
    bool operator<( const SPAddr &rhs ) const;
    bool operator==( const SPAddr &rhs ) const;
};

class SimProbes
{
private:
    QMap<SPAddr,QString>    maddr;
    QSet<int>               shwrslots;

public:
    QMap<SPAddr,QString> &getProbes()   {return maddr;}

    void loadSettings();
    void saveSettings( QMap<SPAddr,QString> &probes );

    void addHwrSlot( int slot );

    bool isSimSlot( int slot ) const;
    bool isSimProbe( const PAddr& adr ) const;

    QString file( const PAddr &adr ) const;
};

#endif  // SIMPROBES_H


