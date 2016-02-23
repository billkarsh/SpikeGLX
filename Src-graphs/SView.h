#ifndef SVIEW_H
#define SVIEW_H

#include <QWidget>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class SVGrafsG_Im;
class SVGrafsG_Ni;
class SVGrafsM_Im;
class SVGrafsM_Ni;

/* ---------------------------------------------------------------- */
/* Stream Viewers ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// SVToolsG toolbar
// -----------------
// SVGrafsG_Im widget.
//
class SViewG_Im : public QWidget
{
public:
    SViewG_Im( SVGrafsG_Im* &vw, GraphsWindow *gw, DAQ::Params &p );
};


// SVToolsG toolbar
// -----------------
// SVGrafsG_Ni widget.
//
class SViewG_Ni : public QWidget
{
public:
    SViewG_Ni( SVGrafsG_Ni* &vw, GraphsWindow *gw, DAQ::Params &p );
};


// SVToolsM toolbar
// -----------------
// SVGrafsM_Im widget.
//
class SViewM_Im : public QWidget
{
public:
    SViewM_Im( SVGrafsM_Im* &vw, GraphsWindow *gw, DAQ::Params &p );
};


// SVToolsM toolbar
// -----------------
// SVGrafsM_Ni widget.
//
class SViewM_Ni : public QWidget
{
public:
    SViewM_Ni( SVGrafsM_Ni* &vw, GraphsWindow *gw, DAQ::Params &p );
};

#endif  // SVIEW_H


