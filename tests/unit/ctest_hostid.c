#include <criterion/criterion.h>

#include "syslog-ng.h"
#include "host-id.h"
#include "logmsg/logmsg.h"
#include "apphook.h"
#include "cfg.h"
#include "mainloop.h"

#include <unistd.h>

static GlobalConfig *
create_cfg(void)
{
  GlobalConfig *cfg;

  cfg = cfg_new(0x0302);

  return cfg;
}

static PersistState *
create_persist_state(const gchar *persist_file)
{
  PersistState *state;

  state = persist_state_new(persist_file);

  cr_assert(persist_state_start(state),
    "Error starting persist state object [%s]", persist_file);
  return state;
}

static guint32
load_hostid_from_persist(const gchar *persist_file)
{
  PersistState *state;
  PersistEntryHandle handle;
  HostIdState *host_id_state;
  gsize size;
  guint8 version;
  guint32 result;

  state = create_persist_state(persist_file);
  handle = persist_state_lookup_entry(state, HOST_ID_PERSIST_KEY, &size, &version);
  cr_assert_neq(handle, 0, "cannot find hostid in persist file");
  host_id_state = persist_state_map_entry(state, handle);
  result = host_id_state->host_id;
  persist_state_unmap_entry(state, handle);
  persist_state_free(state);
  return result;
}

static void
init_mainloop_with_persist_file(const gchar *persist_file)
{
  GlobalConfig *cfg;

  cfg = create_cfg();

  cr_assert(main_loop_initialize_state(cfg, persist_file),
    "main_loop_initialize_state failed");

  cfg_free(cfg);
}

static void
init_mainloop_with_newly_created_persist_file(const gchar *persist_file)
{
  unlink(persist_file);

  init_mainloop_with_persist_file(persist_file);
}

static void
create_persist_file_with_hostid(const gchar *persist_file, guint32 hostid)
{
  PersistState *state;
  PersistEntryHandle handle;
  HostIdState *host_id_state;

  unlink(persist_file);
  state = create_persist_state(persist_file);
  handle = persist_state_alloc_entry(state, HOST_ID_PERSIST_KEY, sizeof(HostIdState));
  host_id_state = persist_state_map_entry(state, handle);
  host_id_state->host_id = hostid;
  persist_state_unmap_entry(state, handle);
  persist_state_commit(state);
  persist_state_free(state);
}


/*
 * Tests
 */

TestSuite(hostid, .init = app_startup, .fini = app_shutdown);

#ifndef __hpux
Test(hostid, test_if_hostid_generated_when_persist_file_not_exists)
{
  const gchar *persist_file = "test_hostid1.persist";
  guint32 hostid;
  init_mainloop_with_newly_created_persist_file(persist_file);

  hostid = load_hostid_from_persist(persist_file);

  cr_assert_eq(hostid, host_id_get(),
    "read hostid(%u) differs from the newly generated hostid(%u)",
    hostid, host_id_get());

  unlink(persist_file);
}

Test(hostid, test_if_hostid_remain_unchanged_when_persist_file_exists)
{
  const gchar *persist_file = "test_hostid2.persist";
  const int hostid = 323;
  create_persist_file_with_hostid(persist_file, hostid);

  init_mainloop_with_persist_file(persist_file);

  cr_assert_eq(host_id_get(), hostid,
    "loaded hostid(%d) differs from expected (%d)",
    host_id_get(), hostid);

  unlink(persist_file);
}
#endif
