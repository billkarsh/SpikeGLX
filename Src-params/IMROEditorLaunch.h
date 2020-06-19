#ifndef IMROEDITORLAUNCH_H
#define IMROEDITORLAUNCH_H

#include "IMROEditor_T0.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool IMROEditorLaunch(
    QObject         *parent,
    QString         &outFile,
    const QString   &file,
    int             selectRow,
    int             type );

#endif  // IMROEDITORLAUNCH_H


