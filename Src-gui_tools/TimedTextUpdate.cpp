
#include "TimedTextUpdate.h"
#include "Util.h"

#include <QTimer>




TimedTextUpdate::TimedTextUpdate( int periodMS, QObject *owner )
    : QObject(owner), txtPersist(0)
{
    QTimer  *timer = new QTimer( owner );
    ConnectUI( timer, SIGNAL(timeout()), this, SLOT(timerExpired()) );
    timer->start( periodMS );
}


void TimedTextUpdate::latestString( const QString &s, int textPersistMS )
{
    QMutexLocker    ml( &mtx );

    str         = s;
    txtPersist  = textPersistMS;
}


void TimedTextUpdate::timerExpired()
{
    QMutexLocker    ml( &mtx );

    if( str != prev ) {

        prev = str;
        emit draw( str, txtPersist );
    }
}


