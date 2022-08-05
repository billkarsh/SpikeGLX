#ifndef SVSHANKVIEWTAB_H
#define SVSHANKVIEWTAB_H

#include "SGLTypes.h"

#include <QObject>

namespace Ui {
class SVShankViewTab;
}

namespace DAQ {
struct Params;
}

class ShankCtlBase;
class Biquad;
class ShankMap;

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVShankViewTab : public QObject
{
    Q_OBJECT

private:
    struct UsrSettings {
        double  updtSecs;
        int     yPix,
                what,
                thresh, // uV
                inarow,
                rng[3]; // {rate, uV, uV}

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
    };

private:
    class Tally {
    private:
        const DAQ::Params   &p;
        std::vector<int>    vmin,
                            vmax;
        double              VMAX,
                            sumSamps;
        int                 js,
                            ip,
                            chunksDone,
                            chunksReqd,
                            maxInt,
                            nAP;
    public:
        std::vector<double> sums;
    public:
        Tally( const DAQ::Params &p, int js, int ip )
            :   p(p), js(js), ip(ip)    {}
        void init( double sUpdt, double VMAX, int maxInt, int nAP );
        void updtChanged( double s );
        void zeroData();
        double gain( int i );
        bool accumSpikes(
            const short *data,
            int         ntpts,
            int         thresh,
            int         inarow );
        void normSpikes();
        bool accumPkPk( const short *data, int ntpts );
        void normPkPk( int what );
    };

private:
    double              VMAX;
    ShankCtlBase        *SC;
    Ui::SVShankViewTab  *svTabUI;
    const DAQ::Params   &p;
    UsrSettings         set;
    Tally               tly;
    Biquad              *hipass,
                        *lopass;
    int                 maxInt,
                        nzero,
                        nAP,
                        nNu,
                        nC,
                        js,
                        ip;

public:
    SVShankViewTab(
        ShankCtlBase        *SC,
        QWidget             *tab,
        const DAQ::Params   &p,
        int                 js,
        int                 ip );
    virtual ~SVShankViewTab();

    void init();

    void setWhat( int what )    {set.what = what;}
    void selChan( int ic, const QString &name );

    void putScans( const vec_i16 &_data );

    void loadSettings( QSettings &S )       {set.loadSettings( S );}
    void saveSettings( QSettings &S ) const {set.saveSettings( S );}

    void winClosed();

public slots:
    void syncYPix( int y );

private slots:
    void ypixChanged( int y );
    void whatChanged( int i );
    void threshChanged( int t );
    void inarowChanged( int s );
    void updtChanged( double s );
    void rangeChanged( int r );
    void chanBut();
    void helpBut();

private:
    void updateFilter( bool lock );
    void zeroFilterTransient( short *data, int ntpts, int nchans );
    void color();
};

#endif  // SVSHANKVIEWTAB_H


