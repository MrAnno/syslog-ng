#ifndef CLIBTEST_PERSIST_LIB_H_INCLUDED
#define CLIBTEST_PERSIST_LIB_H_INCLUDED

#include "persist-state.h"

PersistState *create_persist_state_for_test(const gchar *name);
PersistState *clean_and_create_persist_state_for_test(const gchar *name);
void cancel_and_destroy_persist_state(PersistState *state);
void commit_and_destroy_persist_state(PersistState *state);
void commit_and_free_persist_state(PersistState *state);
PersistState *restart_persist_state(PersistState *state);

#endif
