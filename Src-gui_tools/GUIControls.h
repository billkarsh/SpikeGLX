#ifndef GUICONTROLS_H
#define GUICONTROLS_H

class QComboBox;
class QString;

/* ---------------------------------------------------------------- */
/* Functions ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void FillExtantStreamCB( QComboBox *CB, bool isNidq, int nProbes );

inline void FillStreamCB( QComboBox *CB, int nProbes )
    {FillExtantStreamCB( CB, true, nProbes );}

bool SelStreamCBItem( QComboBox *CB, const QString &selStream );

#endif  // GUICONTROLS_H


