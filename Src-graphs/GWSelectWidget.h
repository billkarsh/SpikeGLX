#ifndef GWSELECTWIDGET_H
#define GWSELECTWIDGET_H

#include <QWidget>

namespace DAQ {
struct Params;
}

class GraphsWindow;

class QCheckBox;
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
    QCheckBox           *rchk;
    QComboBox           *lCB,
                        *rCB;
    int                 lIdx,
                        rIdx;

public:
    GWSelectWidget( GraphsWindow *gw, const DAQ::Params &p );

    bool lChanged();
    bool rChanged();
    bool rChecked();
    void updateSelections();

    int lType();
    int rType();

    QString lStream();
    QString rStream();

public slots:
    void setFocus( bool right );
};

#endif  // GWSELECTWIDGET_H


