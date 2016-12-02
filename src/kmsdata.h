#ifndef KMSDATA_SERVER_H
#define KMSDATA_SERVER_H

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef NO_INTERNAL_DATA

#include "types.h"

extern uint8_t DefaultKmsData[];
__pure size_t getDefaultKmsDataSize();

#endif // NO_INTERNAL_DATA

#endif // KMSDATA_SERVER_H

