#ifndef SVGRAFSM_IMR_H
#define SVGRAFSM_IMR_H

#include "SVGrafsM.h"

// Do update( int ip ) only if whole slot paused
//#define PAUSEWHOLESLOT

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsM_ImR : public SVGrafsM
{
    Q_OBJECT

private:
    QAction         *tracesAction,
                    *rasterAction,
                    *spike1Action,
                    *spike2Action,
                    *spike3Action,
                    *spike4Action,
                    *stdbyAction;
    TSpikeSettings  tsp;

public:
    SVGrafsM_ImR(
        GraphsWindow        *gw,
        const DAQ::Params   &p,
        int                 ip,
        int                 jpanel );

    virtual void putSamps( vec_i16 &data, quint64 headCt );
    virtual void updateRHSFlags();
    virtual void updateProbe( bool shankMap, bool chanMap );
    virtual void setAnatomyPP( const QString &elems, int sk );

    virtual int chanCount() const;
    virtual int neurChanCount() const;
    virtual int analogChanCount() const {return neurChanCount();}
    virtual bool isImec() const         {return true;}
    virtual bool isRaster() const       {return true;}
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool checked );
    virtual void nameLocalFilters( QComboBox *CB ) const;
    virtual void setLocalFilters( int &rin, int &rout, int iflt );

    virtual void setTspike( const TSpikeSettings& T );
    virtual void getTspike( TSpikeSettings& T ) const;

public slots:
    virtual void sAveSelChanged( int sel );
    virtual void rasNMaxChanged( int val );
    virtual void rasPMaxChanged( int val );

private slots:
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy );

    void externSelectChan( int ic, bool shift );
    void colorTraces();

    void setTraces();
    void setRaster();
    void setAudioL();
    void setAudioB();
    void setAudioR();
    void setSpike1();
    void setSpike2();
    void setSpike3();
    void setSpike4();
    void editStdby();
    void editChanMap();
    void editSaved( QString sInit = QString() );

protected:
    virtual void myInit();
    virtual double mySampRate() const;
    virtual void mySort_ig2ic();
    virtual QString myChanName( int ic ) const;
    virtual const QBitArray mySaveBits() const;
    virtual int mySetUsrTypes();
    virtual void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &vmax,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit ) const;

    virtual void loadSettings();
    virtual void saveSettings() const;

private:
    void setAudio( int LBR );
    void setSpike( int gp );
    double scalePlotValue( double v, double gain ) const;
#ifdef PAUSEWHOLESLOT
    bool okToPause();
#endif
    bool stdbyDialog( QString &stdbyStr );
    bool chanMapDialog( QString &cmFile );
};

#endif  // SVGRAFSM_IMR_H


