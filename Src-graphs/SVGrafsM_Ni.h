#ifndef SVGRAFSM_NI_H
#define SVGRAFSM_NI_H

#include "SVGrafsM.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsM_Ni : public SVGrafsM
{
    Q_OBJECT

public:
    SVGrafsM_Ni(
        GraphsWindow        *gw,
        const DAQ::Params   &p,
        int                 jpanel );

    virtual void putSamps( vec_i16 &data, quint64 headCt );
    virtual void updateRHSFlags();

    virtual int chanCount() const;
    virtual int neurChanCount() const;
    virtual int analogChanCount() const;
    virtual bool isImec() const         {return false;}
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool );
    virtual void nameLocalFilters( QComboBox *CB ) const;
    virtual void setLocalFilters( int &rin, int &rout, int iflt );

public slots:
    virtual void bandSelChanged( int sel );
    virtual void sAveSelChanged( int sel );

private slots:
    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double, double, int );

    void externSelectChan( int ic );

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
    double scalePlotValue( double v, double gain ) const;
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

#endif  // SVGRAFSM_NI_H


