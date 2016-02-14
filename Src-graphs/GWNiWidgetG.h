#ifndef GWNIWIDGETG_H
#define GWNIWIDGETG_H

#include "GWWidgetG.h"

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWNiWidgetG : public GWWidgetG
{
    Q_OBJECT

private:
    Biquad          *hipass;
    mutable QMutex  hipassMtx;

public:
    GWNiWidgetG( GraphsWindow *gw, DAQ::Params &p );
    virtual ~GWNiWidgetG();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual bool isChanAnalog( int ic ) const;
    virtual void hipassChecked( bool checked );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y );
    virtual void myClickGraph( double x, double y );

protected:
    virtual int myChanCount();
    virtual double mySampRate();
    virtual void mySort_ig2ic();
    virtual int myGrfPerTab() const;
    virtual QString myChanName( int ic ) const;
    virtual QBitArray& mySaveBits();
    virtual void myCustomXSettings( int ic );
    virtual QString mySettingsGrpName() {return "PlotOptions_nidq";}

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

#endif  // GWNIWIDGETG_H


