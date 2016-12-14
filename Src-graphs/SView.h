#ifndef SVIEW_H
#define SVIEW_H

#include <QWidget>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class SVGrafsM_Im;
class SVGrafsM_Ni;

/* ---------------------------------------------------------------- */
/* Stream Viewers ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// SVToolsM toolbar
// -----------------
// SVGrafsM_Im widget.
//
class SViewM_Im : public QWidget
{
public:
    SViewM_Im(
        SVGrafsM_Im         *&vw,
        GraphsWindow        *gw,
        const DAQ::Params   &p );
};


// SVToolsM toolbar
// -----------------
// SVGrafsM_Ni widget.
//
class SViewM_Ni : public QWidget
{
public:
    SViewM_Ni(
        SVGrafsM_Ni*        &vw,
        GraphsWindow        *gw,
        const DAQ::Params   &p );
};

#endif  // SVIEW_H


