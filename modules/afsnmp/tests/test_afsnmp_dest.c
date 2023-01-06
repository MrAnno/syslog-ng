/*
 * Copyright (c) 2019 Balabit
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include <criterion/criterion.h>

#include "apphook.h"
#include "afsnmpdest.h"
#include "logpipe.h"
#include <unistd.h>

void _abcde(LogDriver *s, const gchar *tzz)
{
  SNMPDestDriver *self = (SNMPDestDriver *) s;

  g_free(self->template_options.time_zone[LTZ_LOCAL]);
  self->template_options.time_zone[LTZ_LOCAL] = g_strdup(tzz);
}

Test(test_snmp_dest, set_time_zonee)
{
  app_startup();
  GlobalConfig *cfg = cfg_new_snippet();

  LogDriver *driver = snmpdest_dd_new(cfg);
  SNMPDestDriver *snmp = (SNMPDestDriver *) driver;

  snmpdest_dd_set_time_zone(driver, "time_zone");
  cr_assert_str_eq(snmp->template_options.time_zone[0], "time_zone");


  log_pipe_unref(&driver->super);
  cfg_free(cfg);
  app_shutdown();
}


Test(test_snmp_dest, set_time_zone_wth)
{
  app_startup();
  GlobalConfig *cfg = cfg_new_snippet();

  LogDriver *driver = snmpdest_dd_new(cfg);
  SNMPDestDriver *snmp = (SNMPDestDriver *) driver;

  _abcde(driver, "time_zone");
  cr_assert_str_eq(snmp->template_options.time_zone[LTZ_LOCAL], "time_zone");

  log_pipe_unref(&driver->super);
  cfg_free(cfg);
  app_shutdown();
}
