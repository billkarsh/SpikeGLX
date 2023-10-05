#ifndef ANATOMY_H
#define ANATOMY_H

#include <QMutex>
#include <QString>

#include <vector>

class ShankView;
struct IMROTbl;
class MGraphX;
class MGraphY;

class QTextEdit;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct AnatomyRgn {
    QString lbl;
    int     row0,   // inclusive
            rowN,   // inclusive
            anaclr; // set in colorTraces()
    quint8  shank,
            r, g, b;

    AnatomyRgn( int sk ) : shank(sk)   {}
};

struct Anatomy {
    std::vector<AnatomyRgn> rgn;
    mutable QMutex          rgnMtx;

    void parse( const QString &elems, const IMROTbl *roTbl, int sk );
    void fillLegend( QTextEdit *leg ) const;
    void colorShanks( ShankView *view, bool on ) const;
    void colorTraces( MGraphX *theX, std::vector<MGraphY> &vY, bool on );
};


#endif  // ANATOMY_H


