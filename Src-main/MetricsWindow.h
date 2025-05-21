#ifndef METRICSWINDOW_H
#define METRICSWINDOW_H

#include <QWidget>
#include <QMap>
#include <QMutex>
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
                errSYNC,
                errMISS;
        MXErrFlags()
        :   errCOUNT(0), errSERDES(0), errLOCK(0),
            errPOP(0), errSYNC(0), errMISS(0)           {}
        MXErrFlags(
            quint32 count,
            quint32 serdes,
            quint32 lock,
            quint32 pop,
            quint32 sync,
            quint32 miss )
        :   errCOUNT(count), errSERDES(serdes), errLOCK(lock),
            errPOP(pop), errSYNC(sync), errMISS(miss)   {}
    };

    struct MXErrRec {
        QMap<QString,MXErrFlags>    flags;
        QMutex                      erfMtx;
        void init() {flags.clear();}
        void setFlags(
            const QString   &stream,
            quint32         count,
            quint32         serdes,
            quint32         lock,
            quint32         pop,
            quint32         sync,
            quint32         miss )
            {
                QMutexLocker    ml( &erfMtx );
                flags[stream]=MXErrFlags( count, serdes, lock, pop, sync, miss );
            }
    };

    struct MXPrfRec {
        QMap<QString,int>   fifoPct;
        QMap<QString,int>   awakePct;
        void init() {fifoPct.clear(); awakePct.clear();}
        void setFifo( const QString &stream, int maxFifo )
            {fifoPct[stream]=maxFifo;}
        void setAwake( const QString &stream, int pct )
            {awakePct[stream]=pct;}
    };

    struct MXDiskRec {
        double                  niFull, obFull, imFull, wbps, rbps;
        QMap<QString,double>    lags;
        int                     g, t;
        MXDiskRec() {init();}
        void init();
        void setGT( int g, int t )
            {this->g=g; this->t=t;}
        void setWrPerf(
            double  niFull,
            double  obFull,
            double  imFull,
            double  wbps,
            double  rbps )
            {
                this->niFull=niFull;
                this->obFull=obFull;
                this->imFull=imFull;
                this->wbps=wbps;this->rbps=rbps;
            }
        void setLag( double pct, const QString &stream )
            {lags[stream]=pct;}
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

    QString getErrFlags( int ip, int shank = -1 );

signals:
    void closed( QWidget *w );

public slots:
    void errUpdateFlags(
        const QString   &stream,
        quint32         count,
        quint32         serdes,
        quint32         lock,
        quint32         pop,
        quint32         sync,
        quint32         miss )
        {err.setFlags( stream, count, serdes, lock, pop, sync, miss );}

    void prfUpdateFifo( const QString &stream, int maxFifo )
        {prf.setFifo( stream, maxFifo );}
    void prfUpdateAwake( const QString &stream, int pct )
        {prf.setAwake( stream, pct );}

    void dskUpdateGT( int g, int t )
        {dsk.setGT( g, t );}
    void dskUpdateWrPerf(
        double  niFull,
        double  obFull,
        double  imFull,
        double  wbps,
        double  rbps )
        {dsk.setWrPerf( niFull, obFull, imFull, wbps, rbps );}
    void dskUpdateLag( double pct, const QString &stream )
        {dsk.setLag( pct, stream );}

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
    bool isActiveOK( const QString &stream ) const;
    void restoreScreenState();
    void saveScreenState() const;
};

#endif  // METRICSWINDOW_H


