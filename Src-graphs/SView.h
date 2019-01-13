#ifndef SVIEW_H
#define SVIEW_H

#include <QWidget>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class SVGrafsM;

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
        SVGrafsM*           &vw,
        GraphsWindow        *gw,
        const DAQ::Params   &p,
        int                 ip,
        int                 jpanel );
};


// SVToolsM toolbar
// -----------------
// SVGrafsM_Ni widget.
//
class SViewM_Ni : public QWidget
{
public:
    SViewM_Ni(
        SVGrafsM*           &vw,
        GraphsWindow        *gw,
        const DAQ::Params   &p,
        int                 jpanel );
};

#endif  // SVIEW_H


