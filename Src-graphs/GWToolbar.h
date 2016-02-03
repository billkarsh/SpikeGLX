#ifndef GWTOOLBAR_H
#define GWTOOLBAR_H

#include <QToolBar>

namespace DAQ {
struct Params;
}

class GraphsWindow;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWToolbar : public QToolBar
{
    Q_OBJECT

private:
    GraphsWindow    *gw;
    DAQ::Params     &p;
    bool            paused;

public:
    GWToolbar( GraphsWindow *gw, DAQ::Params &p );

    void updateSortButText();
    void setSelName( const QString &name );
    bool getScales( double &xSpn, double &yScl ) const;
    QColor selectColor( QColor inColor );
    bool getFltCheck() const;
    bool getTrigCheck() const;
    void setTrigCheck( bool on );
    QString getRunLE() const;
    void setRunLE( const QString &name );
    void enableRunLE( bool enabled );
    void update();

public slots:
    void toggleFetcher();
};

#endif  // GWTOOLBAR_H


