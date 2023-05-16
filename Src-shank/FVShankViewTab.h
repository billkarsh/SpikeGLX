#ifndef FVSHANKVIEWTAB_H
#define FVSHANKVIEWTAB_H

#include "Anatomy.h"
#include "Heatmap.h"

#include <QObject>
#include <QMap>

namespace Ui {
class FVShankViewTab;
}

class ShankCtlBase;
class ShankMap;
class ChanMap;

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVShankViewTab : public QObject
{
    Q_OBJECT

private:
    struct UsrSettings {
        int     yPix,
                what,
                thresh,     // uV
                inarow,
                rng[3];     // {rate, uV, uV}
        bool    colorShanks,
                colorTraces;

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
    };

    struct SBG {
        int s, b, g;
        SBG() : s(0), b(0), g(0)                        {}
        SBG( int s, int b, int g ) : s(s), b(b), g(g)   {}
        bool operator<( const SBG &rhs ) const
        {
            if( s < rhs.s )
                return true;
            if( s > rhs.s )
                return false;
            if( b < rhs.b )
                return true;
            if( b > rhs.b )
                return false;
            return g < rhs.g;
        }
    };

private:
    ShankCtlBase        *SC;
    Ui::FVShankViewTab  *fvTabUI;
    const DataFile      *df;
    ShankMap            *MW;
    ChanMap             *chanMap;
    const double        *svySums[3];
    QMap<int,SBG>       w_ig2sbg;
    QMap<SBG,int>       w_sbg2ig;
    UsrSettings         set;
    Anatomy             anat;
    Heatmap             heat;
    bool                lfp;

public:
    FVShankViewTab(
        ShankCtlBase    *SC,
        QWidget         *tab,
        const DataFile  *df );
    virtual ~FVShankViewTab();

    void init( const ShankMap *map );
    void setAnatomyPP( const QString &elems, int sk );

    bool isLFP() const          {return lfp;}
    void setWhat( int what )    {set.what = what;}
    void mapChanged( const ShankMap *map );
    void selChan( int sh, int bk, int ig );

    void putInit();
    void putSamps( const vec_i16 &_data );
    void putDone();

    void cursorOver( int ig );
    void lbutClicked( int ig );

    void loadSettings( QSettings &S )       {set.loadSettings( S );}
    void saveSettings( QSettings &S ) const {set.saveSettings( S );}

public slots:
    void syncYPix( int y );

private slots:
    void ypixChanged( int y );
    void howChanged( int i );
    void updtBut();
    void whatChanged( int i );
    void threshChanged( int t );
    void inarowChanged( int s );
    void rangeChanged( int r );
    void chanBut();
    void shanksCheck( bool on );
    void tracesCheck( bool on );
    void helpBut();

private:
    void makeWorldMap();
    void color();
};

#endif  // FVSHANKVIEWTAB_H


