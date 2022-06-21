#ifndef FVW_SHANKCTL_H
#define FVW_SHANKCTL_H

#include "SGLTypes.h"

#include <QDialog>
#include <QMutex>

namespace Ui {
class FVW_ShankWindow;
}

class Biquad;
class ShankMap;
class ChanMap;
class DataFile;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVW_ShankCtl : public QDialog
{
    Q_OBJECT

protected:
    struct UsrSettings {
        int     yPix,
                what,
                thresh, // uV
                inarow,
                rng[3]; // {rate, uV, uV}
    };

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
            int         nchans,
            int         c0,
            int         cLim,
            int         thresh,
            int         inarow );
        void normSpikes();
        void accumPkPk(
            const short *data,
            int         ntpts,
            int         nchans,
            int         c0,
            int         cLim );
        void normPkPkAP();
        void normPkPkLF();
    };

protected:
    double              VMAX;
    const DataFile      *df;
    Ui::FVW_ShankWindow *scUI;
    ChanMap             *chanMap;
    UsrSettings         set;
    Tally               tly;
    Biquad              *hipass,
                        *lopass;
    int                 maxInt,
                        nzero,
                        nC,
                        nNu;
    bool                lfp;
    mutable QMutex      drawMtx;

public:
    FVW_ShankCtl( const DataFile *df, QWidget *parent = 0 );
    virtual ~FVW_ShankCtl();

    virtual void init( const ShankMap *map ) = 0;
    void mapChanged( const ShankMap *map );

    void showDialog();
    void selChan( int ig );

    void putInit();
    void putScans( const vec_i16 &_data );
    void putDone();

signals:
    void feedMe();
    void selChanged( int ig );
    void closed( QWidget *w );

public slots:
    void cursorOver( int ig );
    void lbutClicked( int ig );

private slots:
    void ypixChanged( int y );
    void whatChanged( int i );
    void threshChanged( int t );
    void inarowChanged( int s );
    void rangeChanged( int r );
    void chanBut();
    void helpBut();

protected:
    void baseInit();

    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

    virtual void loadSettings() = 0;
    virtual void saveSettings() const = 0;

    virtual QString screenStateName() const = 0;
    void restoreScreenState();
    void saveScreenState() const;

private:
    void updateFilter( bool lock );
    void zeroFilterTransient( short *data, int ntpts, int nchans );
    void color();
    void update();
    void dcAve(
        std::vector<int>    &ave,
        short               *data,
        int                 maxInt,
        int                 ntpts,
        int                 nchans,
        int                 c0,
        int                 cLim );
};

#endif  // FVW_SHANKCTL_H


