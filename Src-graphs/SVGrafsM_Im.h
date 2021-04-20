#ifndef SVGRAFSM_IM_H
#define SVGRAFSM_IM_H

#include "SVGrafsM.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsM_Im : public SVGrafsM
{
    Q_OBJECT

private:
    QAction             *imroAction,
                        *stdbyAction;
    std::vector<int>    muxTbl;
    int                 nADC,
                        nGrp;
    const int           ip,
                        jpanel;

public:
    SVGrafsM_Im(
        GraphsWindow        *gw,
        const DAQ::Params   &p,
        int                 ip,
        int                 jpanel );

    virtual void putScans( vec_i16 &data, quint64 headCt );
    virtual void updateRHSFlags();

    virtual int chanCount() const;
    virtual int neurChanCount() const;
    virtual bool isImec() const         {return true;}
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool checked );
    virtual void nameLocalFilters( QComboBox *CB ) const;
    virtual void setLocalFilters( int &rin, int &rout, int iflt );

public slots:
    virtual void bandSelChanged( int sel );
    virtual void sAveSelChanged( int sel );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy );

    void externSelectChan( int ic, bool shift );

    void setAudioL();
    void setAudioR();
    void editImro();
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
    bool okToPause();
    bool stdbyDialog( QString &stdbyStr );
    bool chanMapDialog( QString &cmFile );
    bool saveDialog( QString &saveStr );
};

#endif  // SVGRAFSM_IM_H


