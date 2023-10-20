#ifndef SVGRAFSM_IM_H
#define SVGRAFSM_IM_H

#include "SVGrafsM.h"

// Do update( int ip ) only if whole slot paused
//#define PAUSEWHOLESLOT

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsM_Im : public SVGrafsM
{
    Q_OBJECT

private:
    QAction             *stdbyAction;
    std::vector<int>    muxTbl;
    int                 nADC,
                        nGrp;

public:
    SVGrafsM_Im(
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
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool checked );
    virtual void nameLocalFilters( QComboBox *CB ) const;
    virtual void setLocalFilters( int &rin, int &rout, int iflt );

public slots:
    virtual void bandSelChanged( int sel );
    virtual void sAveSelChanged( int sel );

private slots:
    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy );

    void externSelectChan( int ic, bool shift );
    void colorTraces();

    void setAudioL();
    void setAudioB();
    void setAudioR();
    void setSpike1();
    void setSpike2();
    void setSpike3();
    void setSpike4();
    void editStdby();
    void editChanMap();
    void editSaved();

protected:
    virtual void myInit();
    virtual double mySampRate() const;
    virtual void mySort_ig2ic();
    virtual QString myChanName( int ic ) const;
    virtual const QBitArray& mySaveBits() const;
    virtual int mySetUsrTypes();

    virtual void loadSettings();
    virtual void saveSettings() const;

private:
    void setAudio( int LBR );
    void setSpike( int gp );
    void sAveApplyDmxTbl(
        const ShankMap  &SM,
        qint16          *d,
        int             ntpts,
        int             nC,
        int             nAP,
        int             dwnSmp );
    double scalePlotValue( double v, double gain ) const;
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit ) const;
#ifdef PAUSEWHOLESLOT
    bool okToPause();
#endif
    bool stdbyDialog( QString &stdbyStr );
    bool chanMapDialog( QString &cmFile );
};

#endif  // SVGRAFSM_IM_H


