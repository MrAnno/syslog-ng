/*
 * Copyright (c) 2017 Balabit
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
 */


/* !REWRITE THIS TERRIBLE TEST HERE */

#include <criterion/criterion.h>
#include "varbindlist-scanner.h"
#include <stdio.h>

static void
_expect_no_more_tokens(VarBindListScanner *scanner)
{
  cr_expect_not(varbindlist_scanner_scan_next(scanner));
}

static void
_expect_next_key_type_value(VarBindListScanner *scanner, const gchar *key, const gchar *type,
                            const gchar *value)
{
  cr_expect(varbindlist_scanner_scan_next(scanner));
  cr_expect_str_eq(varbindlist_scanner_get_current_key(scanner), key);
  cr_expect_str_eq(varbindlist_scanner_get_current_type(scanner), type);
  cr_expect_str_eq(varbindlist_scanner_get_current_value(scanner), value);
}

VarBindListScanner *
create_scanner(void)
{
  VarBindListScanner *scanner = varbindlist_scanner_new();

  VarBindListScanner *cloned = varbindlist_scanner_clone(scanner);
  varbindlist_scanner_free(scanner);
  return cloned;
}


Test(varbindlist_scanner, test_poc_spaces)
{
  VarBindListScanner *scanner = create_scanner();

  varbindlist_scanner_input(scanner,
                            "iso.3.6.1.6.3.1.1.4.1.0 = OID: iso.3.6.1.4.1.18372.3.2.1.1.2.2       "
                            "iso.3.6.1.4.1.18372.3.2.1.1.1.6 = STRING: \"svc/w4joHeFNzpFNrC8u9umJhc/ssh_4eyes_user_subjects:3/ssh\"");

  _expect_next_key_type_value(scanner,
                              "iso.3.6.1.6.3.1.1.4.1.0",
                              "OID",
                              "iso.3.6.1.4.1.18372.3.2.1.1.2.2");
  _expect_next_key_type_value(scanner,
                              "iso.3.6.1.4.1.18372.3.2.1.1.1.6",
                              "STRING",
                              "\"svc/w4joHeFNzpFNrC8u9umJhc/ssh_4eyes_user_subjects:3/ssh\"");

  _expect_no_more_tokens(scanner);
}

Test(varbindlist_scanner, test_poc_tabs_and_spaces)
{
  VarBindListScanner *scanner = create_scanner();

  varbindlist_scanner_input(scanner, "\t iso.3.6.1.6.3.1.1.4.1.0 = OID: iso.3.6.1.4.1.18372.3.2.1.1.2.2\t"
                            "iso.3.6.1.4.1.18372.3.2.1.1.1.6 = STRING: \"svc/w4joHeFNzpFNrC8u9umJhc/ssh_4eyes_user_subjects:3/ssh\"\t\t"
                            "iso.1.2 = INTEGER: 40 \t"
                            "iso.3.4 = INTEGER: 30\t "
                            "iso.5.6 = INTEGER: 20  \t\t "
                            "iso.7.8 = INTEGER: 10");

  _expect_next_key_type_value(scanner,
                              "iso.3.6.1.6.3.1.1.4.1.0",
                              "OID",
                              "iso.3.6.1.4.1.18372.3.2.1.1.2.2");
  _expect_next_key_type_value(scanner,
                              "iso.3.6.1.4.1.18372.3.2.1.1.1.6",
                              "STRING",
                              "\"svc/w4joHeFNzpFNrC8u9umJhc/ssh_4eyes_user_subjects:3/ssh\"");
  _expect_next_key_type_value(scanner,
                              "iso.1.2",
                              "INTEGER",
                              "40");
  _expect_next_key_type_value(scanner,
                              "iso.3.4",
                              "INTEGER",
                              "30");
  _expect_next_key_type_value(scanner,
                              "iso.5.6",
                              "INTEGER",
                              "20");
  _expect_next_key_type_value(scanner,
                              "iso.7.8",
                              "INTEGER",
                              "10");

  _expect_no_more_tokens(scanner);
}

Test(varbindlist_scanner, test_poc_types)
{
  const gchar *input =
    ".iso.org.dod.internet.mgmt.mib-2.system.sysUpTime.0 = Timeticks: (875496867) 101 days, 7:56:08.67       "
    "iso.3.6.1.6.3.1.1.4.1.0 = OID: iso.3.6.1.4.1.8072.2.3.0.1       "
    "iso.3.6.1.4.1.8072.2.3.2.1 = INTEGER: 60   "
    "SNMP-VIEW-BASED-ACM-MIB::vacmSecurityModel.0.3.119.101.115 = STRING: \"random string\"   "
    "iso.3.2.2 = Gauge32: 22 "
    "iso.3.1.1 = Counter32: 11123123   "
    "iso.3.5.3 = Hex-STRING: A0 BB CC DD EF     "
    "iso.3.8.8 = NULL        "
    "iso.2.1.1 = Timeticks: (34234234) 3 days, 23:05:42.34   "
    "SNMP-VIEW-BASED-ACM-MIB::vacmSecurityModel.0.\"wes\" = IpAddress: 192.168.1.0";

  VarBindListScanner *scanner = create_scanner();

  varbindlist_scanner_input(scanner, input);

  _expect_next_key_type_value(scanner,
                              ".iso.org.dod.internet.mgmt.mib-2.system.sysUpTime.0",
                              "Timeticks",
                              "(875496867) 101 days, 7:56:08.67");
  _expect_next_key_type_value(scanner,
                              "iso.3.6.1.6.3.1.1.4.1.0",
                              "OID",
                              "iso.3.6.1.4.1.8072.2.3.0.1");
  _expect_next_key_type_value(scanner,
                              "iso.3.6.1.4.1.8072.2.3.2.1",
                              "INTEGER",
                              "60");
  _expect_next_key_type_value(scanner,
                              "SNMP-VIEW-BASED-ACM-MIB::vacmSecurityModel.0.3.119.101.115",
                              "STRING",
                              "\"random string\"");
  _expect_next_key_type_value(scanner,
                              "iso.3.2.2",
                              "Gauge32",
                              "22");
  _expect_next_key_type_value(scanner,
                              "iso.3.1.1",
                              "Counter32",
                              "11123123");
  _expect_next_key_type_value(scanner,
                              "iso.3.5.3",
                              "Hex-STRING",
                              "A0 BB CC DD EF");
  _expect_next_key_type_value(scanner,
                              "iso.3.8.8",
                              "",
                              "NULL");
  _expect_next_key_type_value(scanner,
                              "iso.2.1.1",
                              "Timeticks",
                              "(34234234) 3 days, 23:05:42.34");
  _expect_next_key_type_value(scanner,
                              "SNMP-VIEW-BASED-ACM-MIB::vacmSecurityModel.0.\"wes\"",
                              "IpAddress",
                              "192.168.1.0");

  _expect_no_more_tokens(scanner);
}
