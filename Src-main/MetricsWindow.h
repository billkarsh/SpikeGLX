#ifndef METRICSWINDOW_H
#define METRICSWINDOW_H

#include <QWidget>
#include <QMap>
#include <QTimer>

namespace Ui {
class MetricsWindow;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class MetricsWindow : public QWidget
{
    Q_OBJECT

private:
    struct MXErrFlags {
        quint32 errCOUNT,
                errSERDES,
                errLOCK,
                errPOP,
                errSYNC;
        MXErrFlags()
        :   errCOUNT(0), errSERDES(0), errLOCK(0),
            errPOP(0), errSYNC(0)       {}
        MXErrFlags(
            quint32 count,
            quint32 serdes,
            quint32 lock,
            quint32 pop,
            quint32 sync )
        :   errCOUNT(count), errSERDES(serdes), errLOCK(lock),
            errPOP(pop), errSYNC(sync)  {}
    };

    struct MXErrRec {
        QMap<int,MXErrFlags>    flags;
        void init() {flags.clear();}
        void setFlags(
            int     ip,
            quint32 count,
            quint32 serdes,
            quint32 lock,
            quint32 pop,
            quint32 sync )
            {flags[ip]=MXErrFlags( count, serdes, lock, pop, sync );}
    };

    struct MXPrfRec {
        QMap<int,int>   fifoPct;
        QMap<int,int>   awakePct;
        void init() {fifoPct.clear(); awakePct.clear();}
        void setFifo( int ip, int maxFifo )
            {fifoPct[ip]=maxFifo;}
        void setAwake( int ip0, int ipN, int pct )
            {
                for( int ip = ip0; ip <= ipN; ++ip )
                    awakePct[ip]=pct;
            }
    };

    struct MXDiskRec {
        double              imFull, niFull, wbps, rbps;
        QMap<int,double>    lags;
        int                 g, t;
        MXDiskRec() {init();}
        void init();
        void setGT( int g, int t )
            {this->g=g; this->t=t;}
        void setWrPerf(
            double  imFull,
            double  niFull,
            double  wbps,
            double  rbps )
            {
                this->imFull=imFull; this->niFull=niFull;
                this->wbps=wbps; this->rbps=rbps;
            }
        void setLag( double pct, int ip )
            {lags[ip]=pct;}
    };

private:
    Ui::MetricsWindow   *mxUI;
    QTimer              mxTimer;
    MXErrRec            err;
    MXPrfRec            prf;
    MXDiskRec           dsk;
    qreal               defSize;
    QColor              defColor;
    int                 defWeight,
                        erLines,
                        erMaxLines;
    bool                isRun;

public:
    MetricsWindow( QWidget *parent = 0 );
    virtual ~MetricsWindow();

    void showDialog();

    void runInit();
    void runStart();
    void runEnd();

signals:
    void closed( QWidget *w );

public slots:
    void errUpdateFlags(
        int     ip,
        quint32 count,
        quint32 serdes,
        quint32 lock,
        quint32 pop,
        quint32 sync )
        {err.setFlags( ip, count, serdes, lock, pop, sync );}

    void prfUpdateFifo( int ip, int maxFifo )
        {prf.setFifo( ip, maxFifo );}
    void prfUpdateAwake( int ip0, int ipN, int pct )
        {prf.setAwake( ip0, ipN, pct );}

    void dskUpdateGT( int g, int t )
        {dsk.setGT( g, t );}
    void dskUpdateWrPerf(
        double  imFull,
        double  niFull,
        double  wbps,
        double  rbps )
        {dsk.setWrPerf( imFull, niFull, wbps, rbps );}
    void dskUpdateLag( double pct, int ip )
        {dsk.setLag( pct, ip );}

    void logAppendText( const QString &txt, const QColor &clr );

private slots:
    void updateMx();
    void help();
    void save();

protected:
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void showEvent( QShowEvent *e );
    virtual void hideEvent( QHideEvent *e );
    virtual void changeEvent( QEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void saveScreenState();
    void restoreScreenState();
};

#endif  // METRICSWINDOW_H


