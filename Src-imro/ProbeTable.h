#ifndef PROBETBL_H
#define PROBETBL_H

#include <QString>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class CProbeTbl
{
public:
    static void ss2ini();
    static void extini();
    static void sortini();
    static void ini2json();
    static void json2ini();
    static void sortjson2ini();
    static void parsejson();

private:
};

#endif  // PROBETBL_H


