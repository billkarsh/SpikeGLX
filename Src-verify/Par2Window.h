#ifndef PAR2WINDOW_H
#define PAR2WINDOW_H

#include <QWidget>
#include <QProcess>
#include <QMutex>

namespace Ui {
class Par2Window;
}

class QCloseEvent;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Par2Worker : public QObject
{
    Q_OBJECT

public:
    enum Op {
        Unknown,
        Verify,
        Create,
        Repair
    };

private:
    mutable QMutex  procMtx,
                    killMtx;
    QProcess        *process;
    QString         file;
    Op              op;
    int             rPct;
    int             pid;
    bool            killed;

public:
    Par2Worker( const QString &file, Op op, int rPct = 5 )
    :   process(0), file(file), op(op),
        rPct(rPct), pid(0), killed(false)   {}
    virtual ~Par2Worker()                   {killProc();}

signals:
    void updateFilename( const QString &name );
    void report( const QString &s );
    void error( const QString &err );
    void finished();

public slots:
    void run()      {go( file, op, rPct );}
    void cancel()   {killProc();}

private slots:
    void procStarted();
    void procFinished( int exitCode, QProcess::ExitStatus status );
    void readyOutput();
    void procError( QProcess::ProcessError err );

private:
    bool isProc()
        {
            QMutexLocker    ml( &procMtx );
            return process != 0;
        }
    bool firstKill()
        {
            QMutexLocker    ml( &killMtx );
            if( killed )
                return false;
            killed = true;
            return true;
        }
    void go( const QString &file, Op op, int rPct );
    void processID();
    void killProc();
};


class Par2Window : public QWidget
{
    Q_OBJECT

private:
    Par2Worker      *worker;
    Ui::Par2Window  *p2wUI;
    Par2Worker::Op  op;

public:
    Par2Window( QWidget *parent = 0 );
    virtual ~Par2Window();

signals:
    void closed( QWidget *w );

public slots:
    void updateFilename( const QString &name );
    void report( const QString &s );
    void finished();

private slots:
    void browseBut();
    void radioChecked();
    void goBut();

protected:
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );
};

#endif  // PAR2WINDOW_H


