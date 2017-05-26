#ifndef TIMEDTEXTUPDATE_H
#define TIMEDTEXTUPDATE_H

#include <QObject>
#include <QMutex>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TimedTextUpdate : public QObject
{
    Q_OBJECT

private:
    mutable QMutex  mtx;
    QString         str,
                    prev;
    int             txtPersist;

public:
    TimedTextUpdate( int periodMS, QObject *owner );

    void latestString( const QString &s, int textPersistMS = 0 );

signals:
    void draw( QString s, int textPersistMS = 0 );

private slots:
    void timerExpired();
};

#endif  // TIMEDTEXTUPDATE_H


