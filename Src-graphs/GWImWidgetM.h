#ifndef GWIMWIDGETM_H
#define GWIMWIDGETM_H

#include "GWWidgetM.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWImWidgetM : public GWWidgetM
{
    Q_OBJECT

private:
    bool    hipass;

public:
    GWImWidgetM( GraphsWindow *gw, DAQ::Params &p );
    virtual ~GWImWidgetM();

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

#endif  // GWIMWIDGETM_H


