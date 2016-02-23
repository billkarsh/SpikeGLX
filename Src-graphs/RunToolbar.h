#ifndef RUNTOOLBAR_H
#define RUNTOOLBAR_H

#include <QToolBar>

namespace DAQ {
struct Params;
}

class GraphsWindow;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class RunToolbar : public QToolBar
{
    Q_OBJECT

private:
    GraphsWindow    *gw;
    DAQ::Params     &p;
    bool            paused;

public:
    RunToolbar( GraphsWindow *gw, DAQ::Params &p );

    void setTrigEnable( bool on, bool block = false );
    QString getRunLE() const;
    void setRunLE( const QString &name );
    void enableRunLE( bool enabled );
    void update();

public slots:
    void trigButClicked( bool checked );
    void toggleFetcher();
};

#endif  // RUNTOOLBAR_H


