#ifndef IMROEDITORLAUNCH_H
#define IMROEDITORLAUNCH_H

#include "IMROEditor_T0.h"
#include "IMROEditor_T1110.h"
#include "IMROEditor_T21.h"
#include "IMROEditor_T24.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool IMROEditorLaunch(
    QObject         *parent,
    QString         &outFile,
    const QString   &file,
    const QString   &pn,
    int             selectRow );

#endif  // IMROEDITORLAUNCH_H


