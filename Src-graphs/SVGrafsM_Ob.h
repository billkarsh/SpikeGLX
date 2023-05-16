#ifndef SVGRAFSM_OB_H
#define SVGRAFSM_OB_H

#include "SVGrafsM.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsM_Ob : public SVGrafsM
{
    Q_OBJECT

public:
    SVGrafsM_Ob(
        GraphsWindow        *gw,
        const DAQ::Params   &p,
        int                 ip,
        int                 jpanel );

    virtual void putSamps( vec_i16 &data, quint64 headCt );
    virtual void updateRHSFlags();

    virtual int chanCount() const;
    virtual int neurChanCount() const       {return 0;}
    virtual int analogChanCount() const;
    virtual bool isImec() const             {return false;}
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool checked );
    virtual void setLocalFilters( int &rin, int &rout, int iflt );

private slots:
    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy );

    void setAudioL();
    void setAudioR();
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
    double scalePlotValue( double v ) const;
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit ) const;
    bool chanMapDialog( QString &cmFile );
    bool saveDialog( QString &saveStr );
};

#endif  // SVGRAFSM_OB_H


