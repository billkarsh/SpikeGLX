#ifndef GWSELECTWIDGET_H
#define GWSELECTWIDGET_H

#include <QWidget>

namespace DAQ {
struct Params;
}

class GraphsWindow;

class QComboBox;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWSelectWidget : public QWidget
{
    Q_OBJECT

private:
    GraphsWindow        *gw;
    const DAQ::Params   &p;
    QComboBox           *lCB,   // left what
                        *rCB,   // right what
                        *gCB;   // grid {0=L-only, 1=right, 2=bottom}
    int                 lIdx,
                        rIdx;

public:
    GWSelectWidget( GraphsWindow *gw, const DAQ::Params &p );

    bool lChanged();
    bool rChanged();
    int grid();
    void updateSelections();

    int ljsip( int &ip );
    int rjsip( int &ip );

    QString lStream();
    QString rStream();

public slots:
    void setFocus( bool right );
};

#endif  // GWSELECTWIDGET_H


