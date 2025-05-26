#ifndef SVSHANKVIEWTAB_H
#define SVSHANKVIEWTAB_H

#include "Anatomy.h"
#include "Heatmap.h"

#include <QObject>

namespace Ui {
class SVShankViewTab;
}

class ShankCtlBase;

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVShankViewTab : public QObject
{
    Q_OBJECT

private:
    struct UsrSettings {
        double  updtSecs;
        int     yPix,
                what,
                thresh, // uV
                inarow,
                rng[3]; // {rate, uV, uV}
        bool    colorShanks,
                colorTraces;

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
    };

private:
    ShankCtlBase        *SC;
    Ui::SVShankViewTab  *svTabUI;
    const DAQ::Params   &p;
    UsrSettings         set;
    Anatomy             anat;
    Heatmap             heat;
    int                 chunksDone,
                        chunksReqd;

public:
    SVShankViewTab(
        ShankCtlBase        *SC,
        QWidget             *tab,
        const DAQ::Params   &p,
        int                 js,
        int                 ip );
    virtual ~SVShankViewTab();

    void init();
    void setAnatomyPP( const QString &elems, int ip, int sk );
    void colorTraces( MGraphX *theX, std::vector<MGraphY> &vY );
    QString getLbl( int s, int r )  {return anat.getLbl( s, r );}

    void setWhat( int what )        {set.what = what;}
    void mapChanged( const ShankMap *S );
    void selChan( int ic, const QString &name );

    void putSamps( const vec_i16 &_data, quint64 headCt );

    void loadSettings( QSettings &S )       {set.loadSettings( S );}
    void saveSettings( QSettings &S ) const {set.saveSettings( S );}

    void winClosed();

public slots:
    void syncYPix( int y );

private slots:
    void ypixChanged( int y );
    void whatChanged( int i );
    void threshChanged( int t );
    void inarowChanged( int s );
    void updtChanged( double s );
    void rangeChanged( int r );
    void chanBut();
    void shanksCheck( bool on );
    void tracesCheck( bool on );
    void helpBut();

private:
    void setReqdChunks( double s );
    void resetAccum( bool resetFlt );
    void color();
};

#endif  // SVSHANKVIEWTAB_H


