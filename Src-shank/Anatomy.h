#ifndef ANATOMY_H
#define ANATOMY_H

#include <QString>

#include <vector>

class ShankView;
class IMROTbl;

class QTextEdit;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct AnatomyRgn {
    QString lbl;
    int     row0,
            rowN;
    quint8  shank,
            r, g, b;

    AnatomyRgn( int sk ) : shank(sk)   {}
};

struct Anatomy {
    std::vector<AnatomyRgn> rgn;

    void parse( const QString &elems, const IMROTbl *roTbl, int sk );
    void fillLegend( QTextEdit *leg );
    void colorShanks( ShankView *view, bool on );
};


#endif  // ANATOMY_H


