#ifndef GWNIWIDGETM_H
#define GWNIWIDGETM_H

#include "GWWidgetM.h"

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWNiWidgetM : public GWWidgetM
{
    Q_OBJECT

private:
    Biquad          *hipass;
    mutable QMutex  hipassMtx;

public:
    GWNiWidgetM( GraphsWindow *gw, DAQ::Params &p );
    virtual ~GWNiWidgetM();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual bool isChanAnalog( int ic ) const;
    virtual void hipassChecked( bool checked );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );

protected:
    virtual int myChanCount();
    virtual double mySampRate();
    virtual void mySort_ig2ic();
    virtual int myGrfPerTab() const;
    virtual QString myChanName( int ic ) const;
    virtual QBitArray& mySaveBits();
    virtual int mySetUsrTypes();
    virtual QString mySettingsGrpName() {return "PlotOptions_nidq";}
    virtual QString myDefClr0()         {return "ffeedd82";}
    virtual QString myDefClr1()         {return "ff44eeff";}
    virtual QString myDefClr2()         {return "ff44eeff";}

private:
    double scalePlotValue( double v, double gain );
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit );
};

#endif  // GWNIWIDGETM_H


