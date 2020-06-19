#ifndef SHANKCTL_H
#define SHANKCTL_H

#include "SGLTypes.h"

#include <QWidget>
#include <QMutex>

namespace Ui {
class ShankWindow;
}

namespace DAQ {
struct Params;
}

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankCtl : public QWidget
{
    Q_OBJECT

protected:
    struct UsrSettings {
        double  updtSecs;
        int     yPix,
                what,
                thresh, // uV
                inarow,
                rng[3]; // {rate, uV, uV}
    };

    class Tally {
    private:
        const DAQ::Params   &p;
        std::vector<int>    vmin,
                            vmax;
        double              sumSamps;
        int                 ip,
                            chunksDone,
                            chunksReqd,
                            nPads;
    public:
        std::vector<double> sums;
    public:
        Tally( const DAQ::Params &p ) : p(p) {}
        void init( double sUpdt, int ip );
        void updtChanged( double s );
        void zeroData();
        bool countSpikes(
            const short *data,
            int         ntpts,
            int         nchans,
            int         c0,
            int         cLim,
            int         thresh,
            int         inarow );
        bool accumPkPk(
            const short *data,
            int         ntpts,
            int         nchans,
            int         c0,
            int         cLim );
    };

protected:
    const DAQ::Params   &p;
    Ui::ShankWindow     *scUI;
    UsrSettings         set;
    Tally               tly;
    Biquad              *hipass,
                        *lopass;
    int                 nzero,
                        jpanel;
    mutable QMutex      drawMtx;

public:
    ShankCtl( const DAQ::Params &p, int jpanel, QWidget *parent = 0 );
    virtual ~ShankCtl();

    virtual void init() = 0;
    virtual void mapChanged() = 0;

    void showDialog();
    void update();
    void selChan( int ic, const QString &name );

    virtual void putScans( const vec_i16 &_data ) = 0;

signals:
    void selChanged( int ic, bool shift );
    void closed( QWidget *w );

public slots:
    virtual void cursorOver( int ic, bool shift ) = 0;
    virtual void lbutClicked( int ic, bool shift ) = 0;

private slots:
    void ypixChanged( int y );
    void whatChanged( int i );
    void threshChanged( int t );
    void inarowChanged( int s );
    void updtChanged( double s );
    void rangeChanged( int r );
    void chanButClicked();
    void helpButClicked();

protected:
    void baseInit( int ip );

    void zeroFilterTransient( short *data, int ntpts, int nchans );

    void dcAve(
        std::vector<int>    &ave,
        short               *data,
        int                 maxInt,
        int                 ntpts,
        int                 nchans,
        int                 c0,
        int                 cLim );

    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

    virtual void updateFilter( bool lock ) = 0;

    virtual void loadSettings() = 0;
    virtual void saveSettings() const = 0;

private:
};

#endif  // SHANKCTL_H


