#ifndef GWIMWIDGETG_H
#define GWIMWIDGETG_H

#include "GWWidgetG.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWImWidgetG : public GWWidgetG
{
    Q_OBJECT

private:
    bool    hipass;

public:
    GWImWidgetG( GraphsWindow *gw, DAQ::Params &p );
    virtual ~GWImWidgetG();

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
    virtual QString mySettingsGrpName() {return "PlotOptions_imec";}

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

#endif  // GWIMWIDGETG_H


