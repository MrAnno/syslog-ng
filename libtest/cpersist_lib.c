#include <criterion/criterion.h>

#include "cpersist_lib.h"
#include <stdlib.h>
#include <unistd.h>

PersistState *
create_persist_state_for_test(const gchar *name)
{
  PersistState *state = persist_state_new(name);

  cr_assert(persist_state_start(state), "Error starting persist_state object");

  return state;
};

PersistState *
clean_and_create_persist_state_for_test(const gchar *name)
{
  unlink(name);

  return create_persist_state_for_test(name);
};

PersistState *
restart_persist_state(PersistState *state)
{
  PersistState *new_state;

  gchar* name = g_strdup(persist_state_get_filename(state));

  persist_state_commit(state);
  persist_state_free(state);

  new_state = create_persist_state_for_test(name);

  g_free(name);
  return new_state;
};

void
cancel_and_destroy_persist_state(PersistState *state)
{
  gchar *filename = g_strdup(persist_state_get_filename(state));

  persist_state_cancel(state);
  persist_state_free(state);

  unlink(filename);
  g_free(filename);
};

void
commit_and_free_persist_state(PersistState *state)
{
  persist_state_commit(state);
  persist_state_free(state);
};

void
commit_and_destroy_persist_state(PersistState *state)
{
  gchar *filename = g_strdup(persist_state_get_filename(state));

  commit_and_free_persist_state(state);

  unlink(filename);
  g_free(filename);
};
