#ifndef FRAMEPOOL_H
#define FRAMEPOOL_H

#include <QObject>
#include <QList>
#include <QGLFormat>
#include <QMutex>

class QMessageBox;
class QWidget;
class QFrame;
class QTimer;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define MAX_GRAPHS_PER_TAB    36

class FramePool : public QObject
{
    Q_OBJECT

private:
    QGLFormat       fmt;
    QMessageBox     *statusDlg;
    QWidget         *frameParent;
    QTimer          *creationTimer;
    QList<QFrame*>  thePool;
    double          tPerGraph;
    mutable QMutex  poolMtx;
    int             maxGraphs;

public:
    FramePool();
    virtual ~FramePool();

    const QGLFormat &getFmt() {return fmt;}

    bool ready()  {return thePool.count() >= maxGraphs;}
    bool isStatusDlg( QObject *watched );

    void showStatusDlg();

    QFrame* getFrame( bool nograph );
    void allFramesToPool( const QVector<QFrame*> &vF );

signals:
    void poolReady();

private slots:
    void timerCreate1();

private:
    void create1( bool noGLGraph = false );
    void allCreated();
};

#endif  // FRAMEPOOL_H


