#ifndef MRANNO_SOURCE_H
#define MRANNO_SOURCE_H

#include "cfg.h"
#include "mranno-source-options.h"

typedef struct MrAnnoSource MrAnnoSource;

MrAnnoSource *mranno_source_new(GlobalConfig *cfg);
gboolean mranno_source_init(MrAnnoSource *self);
gboolean mranno_source_deinit(MrAnnoSource *self);
void mranno_source_free(MrAnnoSource *self);

void mranno_source_set_options(MrAnnoSource *self, MrAnnoSourceOptions *options,
                               const gchar *stats_id, const gchar *stats_instance,
                               gboolean threaded, gboolean pos_tracked, LogExprNode *expr_node);


#endif /* MRANNO_SOURCE_H */
