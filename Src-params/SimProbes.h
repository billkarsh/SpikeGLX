#ifndef SIMPROBES_H
#define SIMPROBES_H

#include <QMap>
#include <QSet>


struct SPAddr {
    int s, p, d;
    SPAddr() : s(0), p(0), d(0)                         {}
    SPAddr( int s, int p, int d ) : s(s), p(p), d(d)    {}
    bool operator<( const SPAddr &rhs ) const;
};

class SimProbes
{
private:
    QMap<SPAddr,QString>    maddr;
    QSet<int>               shwrslots;

public:
    void loadSettings();

    void addHwrSlot( int slot );

    bool isSimSlot( int slot ) const;
    bool isSimProbe( int slot, int port, int dock ) const;

    QString file( int slot, int port, int dock ) const;
};

#endif  // SIMPROBES_H


