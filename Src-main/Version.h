#ifndef VERSION_H
#define VERSION_H

#define APPNAME             "SpikeGLX"
#define VERS_SGLX           0x20250525

#ifdef HAVE_NXT
#define VERS_SGLX_STR       "SpikeGLX v.20250525 NP3 v4.1.3"
#define VERS_IMEC_BS        "3.0.196"
#define VERS_IMEC_BS_BYTES  2319232
#define VERS_IMEC_BSC       "4.0.216"
#define VERS_IMEC_BSC_BYTES 1608404
#else
#define VERS_SGLX_STR       "SpikeGLX v.20250525 NP1 NP2 v4.1.3"
#define VERS_IMEC_BS        "3.0.226"
#define VERS_IMEC_BS_BYTES  2300544
#define VERS_IMEC_BSC       "4.0.233"
#define VERS_IMEC_BSC_BYTES 1617920
#endif

#endif  // VERSION_H


