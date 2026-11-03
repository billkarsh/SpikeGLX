#ifndef SVTOOLSMR_H
#define SVTOOLSMR_H

#include "SVToolsM.h"
#include "ToolBut.h"
#include "TspikeEditor.h"

/* ---------------------------------------------------------------- */
/* Tspike --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

namespace Ui {
class TSpikeEditor;
}

class TSpikeEditor : public DlgEditor {
    Q_OBJECT
private:
    SVGrafsM            *gr;
    Ui::TSpikeEditor    *tsUI;
    TSpikeSettings      T;
public:
    TSpikeEditor( QWidget *parent, SVGrafsM *gr );
    virtual ~TSpikeEditor();
public slots:
    virtual void init();
};

/* ---------------------------------------------------------------- */
/* SVToolsMR ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Toolbar for SViewM:
//
// Sel Clr Max [secs^] [BL^] -<S> [T^] [N^] [P^]
//
class SVToolsMR : public SVToolsM
{
    Q_OBJECT

public:
    SVToolsMR( SVGrafsM *gr ) : SVToolsM(gr)    {}
    virtual void init();

    virtual void setSelName( const QString &name );
    virtual void update();
};

#endif  // SVTOOLSMR_H


