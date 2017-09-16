#ifndef GUICONTROLS_H
#define GUICONTROLS_H

class QComboBox;
class QString;

/* ---------------------------------------------------------------- */
/* Functions ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void FillStreamCB( QComboBox *CB, int nProbes );
void SelStreamCBItem( QComboBox *CB, QString selStream );

#endif  // GUICONTROLS_H


