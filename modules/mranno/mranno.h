#ifndef MRANNO_H
#define MRANNO_H

#include "driver.h"
#include "mranno-source-options.h"

typedef struct MrAnnoSourceDriver MrAnnoSourceDriver;

LogDriver* mranno_sd_new(GlobalConfig *cfg);
MrAnnoSourceOptions *mranno_sd_get_source_options(LogDriver *s);

#endif /* MRANNO_H */
