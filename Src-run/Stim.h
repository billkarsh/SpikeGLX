#ifndef STIM_H
#define STIM_H

#include <QString>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class CStim
{
public:
    static void stimtest();
    static QString obx_set_AO( int istr, const QString &chn_vlt );

private:
    static QString makeErrorString( int err );
};

#endif  // STIM_H


