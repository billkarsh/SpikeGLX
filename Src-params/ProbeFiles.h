#ifndef PROBEFILES_H
#define PROBEFILES_H

#include <QMap>
#include <QSet>


struct PFAddr {
    int s, p, d;
    PFAddr() : s(0), p(0), d(0)                         {}
    PFAddr( int s, int p, int d ) : s(s), p(p), d(d)    {}
    bool operator<( const PFAddr &rhs ) const;
};

class ProbeFiles
{
private:
    QMap<PFAddr,QString>    mfiles;
    QSet<int>               shwrslots;

public:
    void loadSettings();

    void addHwrSlot( int slot );

    bool isSimSlot( int slot ) const;
    bool isSimProbe( int slot, int port, int dock ) const;

    QString file( int slot, int port, int dock ) const;
};

#endif  // PROBEFILES_H


