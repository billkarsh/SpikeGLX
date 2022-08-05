#ifndef FVSHANKVIEWTAB_H
#define FVSHANKVIEWTAB_H

#include "SGLTypes.h"

#include <QObject>

namespace Ui {
class FVShankViewTab;
}

class ShankCtlBase;
class Biquad;
class ShankMap;
class ChanMap;
class DataFile;

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVShankViewTab : public QObject
{
    Q_OBJECT

private:
    struct UsrSettings {
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
        const DataFile      *df;
        std::vector<int>    vmin,
                            vmax;
        double              VMAX,
                            sumSamps;
        int                 maxInt,
                            nNu;
    public:
        std::vector<double> sums;
    public:
        Tally( const DataFile *df, double VMAX ) : df(df), VMAX(VMAX)   {}
        void init( int maxInt, int nNu );
        void zeroData();
        void accumSpikes(
            const short *data,
            int         ntpts,
            int         thresh,
            int         inarow );
        void normSpikes();
        void accumPkPk( const short *data, int ntpts );
        void normPkPk();
    };

private:
    double              VMAX;
    ShankCtlBase        *SC;
    Ui::FVShankViewTab  *svTabUI;
    const DataFile      *df;
    ChanMap             *chanMap;
    UsrSettings         set;
    Tally               tly;
    Biquad              *hipass,
                        *lopass;
    int                 maxInt,
                        nzero,
                        nNu,
                        nC;
    bool                lfp;

public:
    FVShankViewTab(
        ShankCtlBase    *SC,
        QWidget         *tab,
        const DataFile  *df );
    virtual ~FVShankViewTab();

    void init( const ShankMap *map );

    bool isLFP() const          {return lfp;}
    void setWhat( int what )    {set.what = what;}
    void mapChanged( const ShankMap *map );
    void selChan( int ig );

    void putInit();
    void putScans( const vec_i16 &_data );
    void putDone();

    void cursorOver( int ig );

    void loadSettings( QSettings &S )       {set.loadSettings( S );}
    void saveSettings( QSettings &S ) const {set.saveSettings( S );}

public slots:
    void syncYPix( int y );

private slots:
    void ypixChanged( int y );
    void whatChanged( int i );
    void threshChanged( int t );
    void inarowChanged( int s );
    void rangeChanged( int r );
    void chanBut();
    void helpBut();

private:
    void updateFilter( bool lock );
    void zeroFilterTransient( short *data, int ntpts, int nchans );
    void color();
};

#endif  // FVSHANKVIEWTAB_H


