#include <criterion/criterion.h>

#include "apphook.h"
#include "timeutils.h"
#include "logstamp.h"

#include <stdio.h>

void
_set_tz(const char *tz)
{
  static char envbuf[64];

  snprintf(envbuf, sizeof(envbuf), "TZ=%s", tz);
  putenv(envbuf);
  tzset();
  clean_time_cache();
}

int
_timezone_exists(const char *time_zone)
{
  TimeZoneInfo *info;

  _set_tz(time_zone);
  info = time_zone_info_new(time_zone);

  if (info)
    {
      time_zone_info_free(info);
      return TRUE;
    }
  return FALSE;
}

void
assert_timezone(const time_t stamp_to_test, const char* time_zone)
{
  TimeZoneInfo *info;
  time_t offset, expected_offset;

  _set_tz(time_zone);
  info = time_zone_info_new(time_zone);
  offset = time_zone_info_get_offset(info, stamp_to_test);
  expected_offset = get_local_timezone_ofs(stamp_to_test);

  cr_assert_eq(offset, expected_offset,
    "unixtimestamp: %ld TimeZoneName (%s) localtime offset(%ld), timezone file offset(%ld)",
    (glong) stamp_to_test, time_zone, (glong) expected_offset, (glong) offset);

  time_zone_info_free(info);

}

void
test_timezone(const time_t stamp_to_test, const char* time_zone)
{
  if (!_timezone_exists(time_zone))
    {
      printf("SKIP: %s\n", time_zone);
      return;
    }

  assert_timezone(stamp_to_test, time_zone);
  assert_timezone(stamp_to_test - 6*30*24*60*60, time_zone);
  assert_timezone(stamp_to_test + 6*30*24*6, time_zone);
}

void
assert_zone_offset(const gchar *zone, time_t utc, long expected_ofs)
{
  long ofs;

  _set_tz(zone);

  ofs = get_local_timezone_ofs(utc);

  cr_assert_eq(ofs, expected_ofs,
    "Timezone offset mismatch: zone: %s, %ld, expected %ld\n", zone, ofs, expected_ofs);
}


/*
 * Tests
 */

TestSuite(zone, .init = app_startup, .fini = app_shutdown);

Test(zone, test_logstamp)
{
  LogStamp stamp;
  GString *target = g_string_sized_new(32);

  stamp.tv_sec = 1129319257;
  stamp.tv_usec = 123456;
  stamp.zone_offset = 0;

  /* formats */
  log_stamp_format(&stamp, target, TS_FMT_BSD, 3600, 3);
  cr_assert_str_eq(target->str, "Oct 14 20:47:37.123");

  log_stamp_format(&stamp, target, TS_FMT_ISO, 3600, 3);
  cr_assert_str_eq(target->str, "2005-10-14T20:47:37.123+01:00");

  log_stamp_format(&stamp, target, TS_FMT_FULL, 3600, 3);
  cr_assert_str_eq(target->str, "2005 Oct 14 20:47:37.123");

  log_stamp_format(&stamp, target, TS_FMT_UNIX, 3600, 3);
  cr_assert_str_eq(target->str, "1129319257.123");

  /* timezone offsets */
  log_stamp_format(&stamp, target, TS_FMT_ISO, 5400, 3);
  cr_assert_str_eq(target->str, "2005-10-14T21:17:37.123+01:30");

  log_stamp_format(&stamp, target, TS_FMT_ISO, -3600, 3);
  cr_assert_str_eq(target->str, "2005-10-14T18:47:37.123-01:00");

  log_stamp_format(&stamp, target, TS_FMT_ISO, -5400, 3);
  cr_assert_str_eq(target->str, "2005-10-14T18:17:37.123-01:30");

  /* boundary testing */
  stamp.tv_sec = 0;
  stamp.tv_usec = 0;

  log_stamp_format(&stamp, target, TS_FMT_ISO, -1, 0);
  cr_assert_str_eq(target->str, "1970-01-01T00:00:00+00:00");

  g_string_free(target, TRUE);
}

Test(zone, test_zones)
{
  time_t now;

  /* 2005-10-14 21:47:37 CEST, dst enabled */
  assert_zone_offset("MET-1METDST", 1129319257, 7200);
  /* 2005-11-14 10:10:00 CET, dst disabled */
  assert_zone_offset("MET-1METDST", 1131959400, 3600);

  /* 2005-10-14 21:47:37 GMT, no DST */
  assert_zone_offset("GMT", 1129319257, 0);
  /* 2005-11-14 10:10:00 GMT, no DST */
  assert_zone_offset("GMT", 1131959400, 0);

  /* 2005-04-03 01:30:00 ESD, DST disabled */
  assert_zone_offset("EST5EDT", 1112509800, -5*3600);
  /* 2005-04-03 01:59:59 ESD, DST disabled */
  assert_zone_offset("EST5EDT", 1112511599, -5*3600);
  /* 2005-04-03 03:00:00 EDT, DST enabled */
  assert_zone_offset("EST5EDT", 1112511600, -4*3600);
  /* 2005-04-03 03:00:01 EDT, DST enabled */
  assert_zone_offset("EST5EDT", 1112511601, -4*3600);
  /* 2005-10-30 01:59:59 EDT, DST enabled */
  assert_zone_offset("EST5EDT", 1130651999, -4*3600);
  /* 2005-10-30 01:00:00 EST, DST disabled */
  assert_zone_offset("EST5EDT", 1130652000, -5*3600);

#ifdef __linux__

  /* NOTE: not all of our build systems have been upgraded to work correctly
   * with post 2007 years. Therefore we restrict their tests to Linux which
   * work ok. */

  /* USA DST change in 2007, 2nd sunday of March instead of 1st Sunday of April */
  /* 2007-03-11 01:00:00 EST, DST disabled */
  assert_zone_offset("EST5EDT", 1173592800, -5*3600);
  /* 2007-03-11 01:59:59 EST, DST disabled */
  assert_zone_offset("EST5EDT", 1173596399, -5*3600);
# if __GLIBC__ && __GLIBC_MINOR__ > 3
  /* Except on legacy systems lacking updated timezone info.
   * Like Debian Potato ... */
  /* 2007-03-11 03:00:00 EST, DST enabled */
  assert_zone_offset("EST5EDT", 1173596400, -4*3600);
  /* 2007-11-04 01:59:59 EST, DST enabled */
  assert_zone_offset("EST5EDT", 1194155999, -4*3600);
# endif
  /* 2007-11-04 01:00:00 EST, DST disabled */
  assert_zone_offset("EST5EDT", 1194156000, -5*3600);
#endif

#ifdef __linux__
  /* Oct 31 01:59:59 2004 (EST) +1000 */
  assert_zone_offset("Australia/Victoria", 1099151999, 10*3600);
  /* Oct 31 03:00:00 2004 (EST) +1100 */
  assert_zone_offset("Australia/Victoria", 1099152000, 11*3600);
  /* Mar 27 02:59:59 2005 (EST) +1100 */
  assert_zone_offset("Australia/Victoria", 1111852799, 11*3600);
  /* Mar 27 02:00:00 2005 (EST) +1000 */
  assert_zone_offset("Australia/Victoria", 1111852800, 10*3600);

  /* Oct  2 01:59:59 2004 (NZT) +1200 */
  assert_zone_offset("NZ", 1128175199, 12*3600);
  /* Oct  2 03:00:00 2004 (NZDT) +1300 */
  assert_zone_offset("NZ", 1128175200, 13*3600);
  /* Mar 20 02:59:59 2005 (NZDT) +1300 */
  assert_zone_offset("NZ", 1111240799, 13*3600);
  /* Mar 20 02:00:00 2005 (NZT) +1200 */
  assert_zone_offset("NZ", 1111240800, 12*3600);

  now = time(NULL);

  test_timezone(now, "America/Louisville");
  //test_timezone(now, "Hongkong");
  test_timezone(now, "Europe/Budapest");
  test_timezone(1288486800, "Europe/Budapest");
  test_timezone(1288486860, "Europe/Budapest");
  test_timezone(1288486740, "Europe/Budapest");
  test_timezone(1288486800-1800, "Europe/Budapest");
  test_timezone(1288486800+1800, "Europe/Budapest");
  test_timezone(1288486800-3600, "Europe/Budapest");
  test_timezone(1288486800+3600, "Europe/Budapest");
  test_timezone(1288486800-3601, "Europe/Budapest");
  test_timezone(1288486800+3601, "Europe/Budapest");
  test_timezone(now, "Asia/Kuala_Lumpur");
  test_timezone(now, "CST6CDT");
  test_timezone(now, "US/Pacific");
  test_timezone(now, "US/Indiana-Starke");
  test_timezone(now, "US/Samoa");
  test_timezone(now, "US/Arizona");
  test_timezone(now, "US/Aleutian");
  test_timezone(now, "US/Michigan");
  test_timezone(now, "US/Alaska");
  test_timezone(now, "US/Central");
  test_timezone(now, "US/Hawaii");
  test_timezone(now, "US/East-Indiana");
  test_timezone(now, "US/Eastern");
  test_timezone(now, "US/Mountain");
  test_timezone(now, "Pacific/Noumea");
  test_timezone(now, "Pacific/Samoa");
  test_timezone(now, "Pacific/Apia");
  test_timezone(now, "Pacific/Auckland");
  test_timezone(now, "Pacific/Fakaofo");
  test_timezone(now, "Pacific/Saipan");
  test_timezone(now, "Pacific/Easter");
  test_timezone(now, "Pacific/Norfolk");
  test_timezone(now, "Pacific/Kosrae");
  test_timezone(now, "Pacific/Tarawa");
  test_timezone(now, "Pacific/Tahiti");
  test_timezone(now, "Pacific/Pago_Pago");
  test_timezone(now, "Pacific/Majuro");
  test_timezone(now, "Pacific/Guam");
  test_timezone(now, "Pacific/Ponape");
  test_timezone(now, "Pacific/Tongatapu");
  test_timezone(now, "Pacific/Funafuti");
  test_timezone(now, "Pacific/Wallis");
  test_timezone(now, "Pacific/Efate");
  test_timezone(now, "Pacific/Marquesas");
  test_timezone(now, "Pacific/Enderbury");
  test_timezone(now, "Pacific/Pitcairn");
  test_timezone(now, "Pacific/Yap");
  test_timezone(now, "Pacific/Wake");
  test_timezone(now, "Pacific/Johnston");
  test_timezone(now, "Pacific/Kwajalein");
  test_timezone(now, "Pacific/Chatham");
  test_timezone(now, "Pacific/Gambier");
  test_timezone(now, "Pacific/Galapagos");
  test_timezone(now, "Pacific/Kiritimati");
  test_timezone(now, "Pacific/Honolulu");
  test_timezone(now, "Pacific/Truk");
  test_timezone(now, "Pacific/Midway");
  test_timezone(now, "Pacific/Fiji");
  test_timezone(now, "Pacific/Rarotonga");
  test_timezone(now, "Pacific/Guadalcanal");
  test_timezone(now, "Pacific/Nauru");
  test_timezone(now, "Pacific/Palau");
  test_timezone(now, "Pacific/Port_Moresby");
  test_timezone(now, "Pacific/Niue");
  test_timezone(now, "GMT");
  test_timezone(now, "Hongkong");
  test_timezone(now, "ROK");
  test_timezone(now, "Navajo");
  test_timezone(now, "ROC");
  test_timezone(now, "WET");
  test_timezone(now, "posixrules");
  test_timezone(now, "CET");
  test_timezone(now, "MET");
  test_timezone(now, "MST");
  test_timezone(now, "Turkey");
  test_timezone(now, "Zulu");
  test_timezone(now, "GMT+0");
  test_timezone(now, "Canada/Saskatchewan");
  test_timezone(now, "Canada/Pacific");
  test_timezone(now, "Canada/Yukon");
  test_timezone(now, "Canada/East-Saskatchewan");
  test_timezone(now, "Canada/Newfoundland");
  test_timezone(now, "Canada/Central");
  test_timezone(now, "Canada/Eastern");
  test_timezone(now, "Canada/Atlantic");
  test_timezone(now, "Canada/Mountain");
  test_timezone(now, "Singapore");
  test_timezone(now, "UCT");
  test_timezone(now, "Poland");
  test_timezone(now, "Chile/Continental");
  test_timezone(now, "Chile/EasterIsland");
  test_timezone(now, "Portugal");
  test_timezone(now, "Egypt");
  test_timezone(now, "Japan");
  test_timezone(now, "Iran");
  test_timezone(now, "EET");
  test_timezone(now, "Europe/Oslo");
  test_timezone(now, "Europe/Bratislava");
  test_timezone(now, "Europe/Gibraltar");
  test_timezone(now, "Europe/Skopje");
  test_timezone(now, "Europe/Simferopol");
  test_timezone(now, "Europe/Zurich");
  test_timezone(now, "Europe/Vienna");
  test_timezone(now, "Europe/Paris");
  test_timezone(now, "Europe/Jersey");
  test_timezone(now, "Europe/Tallinn");
  test_timezone(now, "Europe/Madrid");
  test_timezone(now, "Europe/Volgograd");
  test_timezone(now, "Europe/Zaporozhye");
  test_timezone(now, "Europe/Mariehamn");
  test_timezone(now, "Europe/Vaduz");
  test_timezone(now, "Europe/Moscow");
  test_timezone(now, "Europe/Stockholm");
  test_timezone(now, "Europe/Minsk");
  test_timezone(now, "Europe/Andorra");
  test_timezone(now, "Europe/Istanbul");
  test_timezone(now, "Europe/Tiraspol");
  test_timezone(now, "Europe/Podgorica");
  test_timezone(now, "Europe/Bucharest");
  test_timezone(now, "Europe/Ljubljana");
  test_timezone(now, "Europe/Brussels");
  test_timezone(now, "Europe/Amsterdam");
  test_timezone(now, "Europe/Riga");
  test_timezone(now, "Europe/Tirane");
  test_timezone(now, "Europe/Berlin");
  test_timezone(now, "Europe/Guernsey");
  test_timezone(now, "Europe/Warsaw");
  test_timezone(now, "Europe/Vatican");
  test_timezone(now, "Europe/Malta");
  test_timezone(now, "Europe/Nicosia");
  test_timezone(now, "Europe/Budapest");
  test_timezone(now, "Europe/Kaliningrad");
  test_timezone(now, "Europe/Sarajevo");
  test_timezone(now, "Europe/Isle_of_Man");
  test_timezone(now, "Europe/Rome");
  test_timezone(now, "Europe/London");
  test_timezone(now, "Europe/Vilnius");
  test_timezone(now, "Europe/Samara");
  test_timezone(now, "Europe/Belfast");
  test_timezone(now, "Europe/Athens");
  test_timezone(now, "Europe/Copenhagen");
  test_timezone(now, "Europe/Belgrade");
  test_timezone(now, "Europe/Sofia");
  test_timezone(now, "Europe/San_Marino");
  test_timezone(now, "Europe/Helsinki");
  test_timezone(now, "Europe/Uzhgorod");
  test_timezone(now, "Europe/Monaco");
  test_timezone(now, "Europe/Prague");
  test_timezone(now, "Europe/Zagreb");
  test_timezone(now, "Europe/Lisbon");
  test_timezone(now, "Europe/Chisinau");
  test_timezone(now, "Europe/Dublin");
  test_timezone(now, "Europe/Luxembourg");
  test_timezone(now, "Europe/Kiev");
  test_timezone(now, "Jamaica");
  test_timezone(now, "Indian/Chagos");
  test_timezone(now, "Indian/Comoro");
  test_timezone(now, "Indian/Mauritius");
  test_timezone(now, "Indian/Mayotte");
  test_timezone(now, "Indian/Kerguelen");
  test_timezone(now, "Indian/Maldives");
  test_timezone(now, "Indian/Antananarivo");
  test_timezone(now, "Indian/Mahe");
  test_timezone(now, "Indian/Cocos");
  test_timezone(now, "Indian/Christmas");
  test_timezone(now, "Indian/Reunion");
  test_timezone(now, "Africa/Accra");
  test_timezone(now, "Africa/Lubumbashi");
  test_timezone(now, "Africa/Bangui");
  test_timezone(now, "Africa/Asmara");
  test_timezone(now, "Africa/Freetown");
  test_timezone(now, "Africa/Mbabane");
  test_timezone(now, "Africa/Djibouti");
  test_timezone(now, "Africa/Ndjamena");
  test_timezone(now, "Africa/Porto-Novo");
  test_timezone(now, "Africa/Nouakchott");
  test_timezone(now, "Africa/Brazzaville");
  test_timezone(now, "Africa/Tunis");
  test_timezone(now, "Africa/Dar_es_Salaam");
  test_timezone(now, "Africa/Lusaka");
  test_timezone(now, "Africa/Banjul");
  test_timezone(now, "Africa/Sao_Tome");
  test_timezone(now, "Africa/Monrovia");
  test_timezone(now, "Africa/Lagos");
  test_timezone(now, "Africa/Conakry");
  test_timezone(now, "Africa/Tripoli");
  test_timezone(now, "Africa/Algiers");
  test_timezone(now, "Africa/Casablanca");
  test_timezone(now, "Africa/Lome");
  test_timezone(now, "Africa/Bamako");
  test_timezone(now, "Africa/Nairobi");
  test_timezone(now, "Africa/Douala");
  test_timezone(now, "Africa/Addis_Ababa");
  test_timezone(now, "Africa/El_Aaiun");
  test_timezone(now, "Africa/Ceuta");
  test_timezone(now, "Africa/Harare");
  test_timezone(now, "Africa/Libreville");
  test_timezone(now, "Africa/Johannesburg");
  test_timezone(now, "Africa/Timbuktu");
  test_timezone(now, "Africa/Niamey");
  test_timezone(now, "Africa/Windhoek");
  test_timezone(now, "Africa/Bissau");
  test_timezone(now, "Africa/Maputo");
  test_timezone(now, "Africa/Kigali");
  test_timezone(now, "Africa/Dakar");
  test_timezone(now, "Africa/Ouagadougou");
  test_timezone(now, "Africa/Gaborone");
  test_timezone(now, "Africa/Khartoum");
  test_timezone(now, "Africa/Bujumbura");
  test_timezone(now, "Africa/Luanda");
  test_timezone(now, "Africa/Malabo");
  test_timezone(now, "Africa/Asmera");
  test_timezone(now, "Africa/Maseru");
  test_timezone(now, "Africa/Abidjan");
  test_timezone(now, "Africa/Kinshasa");
  test_timezone(now, "Africa/Blantyre");
  test_timezone(now, "Africa/Cairo");
  test_timezone(now, "Africa/Kampala");
  test_timezone(now, "Africa/Mogadishu");
  test_timezone(now, "Universal");
  test_timezone(now, "EST");
  test_timezone(now, "Greenwich");
  test_timezone(now, "Eire");
  test_timezone(now, "Asia/Qatar");
  test_timezone(now, "Asia/Makassar");
  test_timezone(now, "Asia/Colombo");
  test_timezone(now, "Asia/Chungking");
  test_timezone(now, "Asia/Macau");
  test_timezone(now, "Asia/Choibalsan");
  test_timezone(now, "Asia/Rangoon");
  test_timezone(now, "Asia/Harbin");
  test_timezone(now, "Asia/Yerevan");
  test_timezone(now, "Asia/Brunei");
  test_timezone(now, "Asia/Omsk");
  test_timezone(now, "Asia/Chongqing");
  test_timezone(now, "Asia/Tbilisi");
  test_timezone(now, "Asia/Tehran");
  test_timezone(now, "Asia/Manila");
  test_timezone(now, "Asia/Yakutsk");
  test_timezone(now, "Asia/Qyzylorda");
  test_timezone(now, "Asia/Dhaka");
  test_timezone(now, "Asia/Singapore");
  test_timezone(now, "Asia/Jakarta");
  test_timezone(now, "Asia/Novosibirsk");
  test_timezone(now, "Asia/Saigon");
  test_timezone(now, "Asia/Krasnoyarsk");
  test_timezone(now, "Asia/Kabul");
  test_timezone(now, "Asia/Bahrain");
  test_timezone(now, "Asia/Urumqi");
  test_timezone(now, "Asia/Thimbu");
  test_timezone(now, "Asia/Ulan_Bator");
  test_timezone(now, "Asia/Taipei");
  test_timezone(now, "Asia/Tashkent");
  test_timezone(now, "Asia/Dacca");
  test_timezone(now, "Asia/Aqtau");
  test_timezone(now, "Asia/Seoul");
  test_timezone(now, "Asia/Istanbul");
  test_timezone(now, "Asia/Tel_Aviv");
  test_timezone(now, "Asia/Almaty");
  test_timezone(now, "Asia/Phnom_Penh");
  test_timezone(now, "Asia/Baku");
  test_timezone(now, "Asia/Anadyr");
  test_timezone(now, "Asia/Aqtobe");
  test_timezone(now, "Asia/Kuwait");
  test_timezone(now, "Asia/Irkutsk");
  test_timezone(now, "Asia/Ulaanbaatar");
  test_timezone(now, "Asia/Tokyo");
  test_timezone(now, "Asia/Gaza");
  test_timezone(now, "Asia/Riyadh87");
  test_timezone(now, "Asia/Yekaterinburg");
  test_timezone(now, "Asia/Riyadh88");
  test_timezone(now, "Asia/Nicosia");
  test_timezone(now, "Asia/Jayapura");
  test_timezone(now, "Asia/Shanghai");
  test_timezone(now, "Asia/Pyongyang");
  test_timezone(now, "Asia/Macao");
  test_timezone(now, "Asia/Dushanbe");
  test_timezone(now, "Asia/Kuching");
  test_timezone(now, "Asia/Vientiane");
  test_timezone(now, "Asia/Riyadh");
  test_timezone(now, "Asia/Dili");
  test_timezone(now, "Asia/Samarkand");
  test_timezone(now, "Asia/Ashkhabad");
  test_timezone(now, "Asia/Calcutta");
  test_timezone(now, "Asia/Hong_Kong");
  test_timezone(now, "Asia/Sakhalin");
  test_timezone(now, "Asia/Beirut");
  test_timezone(now, "Asia/Damascus");
  test_timezone(now, "Asia/Katmandu");
  test_timezone(now, "Asia/Jerusalem");
  test_timezone(now, "Asia/Riyadh89");
  test_timezone(now, "Asia/Vladivostok");
  test_timezone(now, "Asia/Kamchatka");
  test_timezone(now, "Asia/Dubai");
  test_timezone(now, "Asia/Kashgar");
  test_timezone(now, "Asia/Ashgabat");
  test_timezone(now, "Asia/Amman");
  test_timezone(now, "Asia/Karachi");
  test_timezone(now, "Asia/Bangkok");
  test_timezone(now, "Asia/Oral");
  test_timezone(now, "Asia/Thimphu");
  test_timezone(now, "Asia/Bishkek");
  test_timezone(now, "Asia/Baghdad");
  test_timezone(now, "Asia/Kuala_Lumpur");
  test_timezone(now, "Asia/Pontianak");
  test_timezone(now, "Asia/Ujung_Pandang");
  test_timezone(now, "Asia/Muscat");
  test_timezone(now, "Asia/Aden");
  test_timezone(now, "Asia/Hovd");
  test_timezone(now, "Asia/Magadan");
  test_timezone(now, "EST5EDT");
  test_timezone(now, "PST8PDT");
  test_timezone(now, "Atlantic/Bermuda");
  test_timezone(now, "Atlantic/St_Helena");
  test_timezone(now, "Atlantic/Cape_Verde");
  test_timezone(now, "Atlantic/Stanley");
  test_timezone(now, "Atlantic/Azores");
  test_timezone(now, "Atlantic/Jan_Mayen");
  test_timezone(now, "Atlantic/Reykjavik");
  test_timezone(now, "Atlantic/Madeira");
  test_timezone(now, "Atlantic/Canary");
  test_timezone(now, "Atlantic/Faeroe");
  test_timezone(now, "Atlantic/Faroe");
  test_timezone(now, "Atlantic/South_Georgia");
  test_timezone(now, "Kwajalein");
  test_timezone(now, "UTC");
  test_timezone(now, "GMT-0");
  test_timezone(now, "MST7MDT");
  test_timezone(now, "GB-Eire");
  test_timezone(now, "PRC");
  test_timezone(now, "Arctic/Longyearbyen");
  test_timezone(now, "Cuba");
  test_timezone(now, "Israel");
  test_timezone(now, "Etc/GMT-3");
  test_timezone(now, "Etc/GMT+1");
  test_timezone(now, "Etc/GMT-5");
  test_timezone(now, "Etc/GMT");
  test_timezone(now, "Etc/GMT-13");
  test_timezone(now, "Etc/GMT-1");
  test_timezone(now, "Etc/GMT-9");
  test_timezone(now, "Etc/Zulu");
  test_timezone(now, "Etc/GMT+0");
  test_timezone(now, "Etc/UCT");
  test_timezone(now, "Etc/GMT+12");
  test_timezone(now, "Etc/GMT+9");
  test_timezone(now, "Etc/GMT-6");
  test_timezone(now, "Etc/Universal");
  test_timezone(now, "Etc/GMT-2");
  test_timezone(now, "Etc/Greenwich");
  test_timezone(now, "Etc/GMT+3");
  test_timezone(now, "Etc/GMT+8");
  test_timezone(now, "Etc/UTC");
  test_timezone(now, "Etc/GMT-0");
  test_timezone(now, "Etc/GMT-14");
  test_timezone(now, "Etc/GMT+10");
  test_timezone(now, "Etc/GMT+4");
  test_timezone(now, "Etc/GMT+5");
  test_timezone(now, "Etc/GMT-12");
  test_timezone(now, "Etc/GMT-8");
  test_timezone(now, "Etc/GMT+7");
  test_timezone(now, "Etc/GMT-11");
  test_timezone(now, "Etc/GMT+6");
  test_timezone(now, "Etc/GMT0");
  test_timezone(now, "Etc/GMT-7");
  test_timezone(now, "Etc/GMT+11");
  test_timezone(now, "Etc/GMT-4");
  test_timezone(now, "Etc/GMT+2");
  test_timezone(now, "Etc/GMT-10");
  test_timezone(now, "HST");
  test_timezone(now, "Iceland");
  test_timezone(now, "Mexico/BajaNorte");
  test_timezone(now, "Mexico/BajaSur");
  test_timezone(now, "Mexico/General");
  test_timezone(now, "Mideast/Riyadh87");
  test_timezone(now, "Mideast/Riyadh88");
  test_timezone(now, "Mideast/Riyadh89");
  test_timezone(now, "Antarctica/Davis");
  test_timezone(now, "Antarctica/DumontDUrville");
  test_timezone(now, "Antarctica/Syowa");
  test_timezone(now, "Antarctica/Palmer");
  test_timezone(now, "Antarctica/Casey");
  test_timezone(now, "Antarctica/Rothera");
  test_timezone(now, "Antarctica/Mawson");
  test_timezone(now, "Antarctica/McMurdo");
  test_timezone(now, "Antarctica/South_Pole");
  test_timezone(now, "Antarctica/Vostok");
  test_timezone(now, "America/Curacao");
  test_timezone(now, "America/St_Lucia");
  test_timezone(now, "America/Managua");
  test_timezone(now, "America/Lima");
  test_timezone(now, "America/Nipigon");
  test_timezone(now, "America/Cayenne");
  test_timezone(now, "America/Detroit");
  test_timezone(now, "America/Kentucky/Louisville");
  test_timezone(now, "America/Kentucky/Monticello");
  test_timezone(now, "America/Belem");
  test_timezone(now, "America/Jujuy");
  test_timezone(now, "America/Godthab");
  test_timezone(now, "America/Guatemala");
  test_timezone(now, "America/Atka");
  test_timezone(now, "America/Montreal");
  test_timezone(now, "America/Resolute");
  test_timezone(now, "America/Thunder_Bay");
  test_timezone(now, "America/North_Dakota/New_Salem");
  test_timezone(now, "America/North_Dakota/Center");
  test_timezone(now, "America/Panama");
  test_timezone(now, "America/Los_Angeles");
  test_timezone(now, "America/Whitehorse");
  test_timezone(now, "America/Santiago");
  test_timezone(now, "America/Iqaluit");
  test_timezone(now, "America/Dawson");
  test_timezone(now, "America/Juneau");
  test_timezone(now, "America/Thule");
  test_timezone(now, "America/Fortaleza");
  test_timezone(now, "America/Montevideo");
  test_timezone(now, "America/Tegucigalpa");
  test_timezone(now, "America/Port-au-Prince");
  test_timezone(now, "America/Guadeloupe");
  test_timezone(now, "America/Coral_Harbour");
  test_timezone(now, "America/Monterrey");
  test_timezone(now, "America/Anguilla");
  test_timezone(now, "America/Antigua");
  test_timezone(now, "America/Campo_Grande");
  test_timezone(now, "America/Buenos_Aires");
  test_timezone(now, "America/Maceio");
  test_timezone(now, "America/New_York");
  test_timezone(now, "America/Puerto_Rico");
  test_timezone(now, "America/Noronha");
  test_timezone(now, "America/Sao_Paulo");
  test_timezone(now, "America/Cancun");
  test_timezone(now, "America/Aruba");
  test_timezone(now, "America/Yellowknife");
  test_timezone(now, "America/Knox_IN");
  test_timezone(now, "America/Halifax");
  test_timezone(now, "America/Grand_Turk");
  test_timezone(now, "America/Vancouver");
  test_timezone(now, "America/Bogota");
  test_timezone(now, "America/Santo_Domingo");
  test_timezone(now, "America/Tortola");
  test_timezone(now, "America/Blanc-Sablon");
  test_timezone(now, "America/St_Thomas");
  test_timezone(now, "America/Scoresbysund");
  test_timezone(now, "America/Jamaica");
  test_timezone(now, "America/El_Salvador");
  test_timezone(now, "America/Montserrat");
  test_timezone(now, "America/Martinique");
  test_timezone(now, "America/Nassau");
  test_timezone(now, "America/Indianapolis");
  test_timezone(now, "America/Pangnirtung");
  test_timezone(now, "America/Port_of_Spain");
  test_timezone(now, "America/Mexico_City");
  test_timezone(now, "America/Denver");
  test_timezone(now, "America/Dominica");
  test_timezone(now, "America/Eirunepe");
  test_timezone(now, "America/Atikokan");
  test_timezone(now, "America/Glace_Bay");
  test_timezone(now, "America/Rainy_River");
  test_timezone(now, "America/St_Barthelemy");
  test_timezone(now, "America/Miquelon");
  test_timezone(now, "America/Indiana/Vevay");
  test_timezone(now, "America/Indiana/Vincennes");
  test_timezone(now, "America/Indiana/Marengo");
  test_timezone(now, "America/Indiana/Petersburg");
  test_timezone(now, "America/Indiana/Tell_City");
  test_timezone(now, "America/Indiana/Knox");
  test_timezone(now, "America/Indiana/Indianapolis");
  test_timezone(now, "America/Indiana/Winamac");
  test_timezone(now, "America/Menominee");
  test_timezone(now, "America/Porto_Velho");
  test_timezone(now, "America/Phoenix");
  test_timezone(now, "America/Argentina/San_Juan");
  test_timezone(now, "America/Argentina/Jujuy");
  test_timezone(now, "America/Argentina/Ushuaia");
  test_timezone(now, "America/Argentina/Buenos_Aires");
  test_timezone(now, "America/Argentina/La_Rioja");
  test_timezone(now, "America/Argentina/ComodRivadavia");
  test_timezone(now, "America/Argentina/Tucuman");
  test_timezone(now, "America/Argentina/Rio_Gallegos");
  test_timezone(now, "America/Argentina/Mendoza");
  test_timezone(now, "America/Argentina/Cordoba");
  test_timezone(now, "America/Argentina/Catamarca");
  test_timezone(now, "America/Dawson_Creek");
  test_timezone(now, "America/Merida");
  test_timezone(now, "America/Moncton");
  test_timezone(now, "America/Goose_Bay");
  test_timezone(now, "America/Grenada");
  test_timezone(now, "America/Barbados");
  test_timezone(now, "America/Tijuana");
  test_timezone(now, "America/Regina");
  test_timezone(now, "America/Ensenada");
  test_timezone(now, "America/Louisville");
  test_timezone(now, "America/Edmonton");
  test_timezone(now, "America/Bahia");
  test_timezone(now, "America/Nome");
  test_timezone(now, "America/Guayaquil");
  test_timezone(now, "America/La_Paz");
  test_timezone(now, "America/Costa_Rica");
  test_timezone(now, "America/Mazatlan");
  test_timezone(now, "America/Havana");
  test_timezone(now, "America/Marigot");
  test_timezone(now, "America/Mendoza");
  test_timezone(now, "America/Virgin");
  test_timezone(now, "America/Manaus");
  test_timezone(now, "America/Rosario");
  test_timezone(now, "America/Boa_Vista");
  test_timezone(now, "America/Winnipeg");
  test_timezone(now, "America/Paramaribo");
  test_timezone(now, "America/Danmarkshavn");
  test_timezone(now, "America/Caracas");
  test_timezone(now, "America/Swift_Current");
  test_timezone(now, "America/St_Johns");
  test_timezone(now, "America/Araguaina");
  test_timezone(now, "America/Adak");
  test_timezone(now, "America/Cordoba");
  test_timezone(now, "America/Fort_Wayne");
  test_timezone(now, "America/Catamarca");
  test_timezone(now, "America/Recife");
  test_timezone(now, "America/Toronto");
  test_timezone(now, "America/Anchorage");
  test_timezone(now, "America/St_Vincent");
  test_timezone(now, "America/St_Kitts");
  test_timezone(now, "America/Chihuahua");
  test_timezone(now, "America/Cayman");
  test_timezone(now, "America/Belize");
  test_timezone(now, "America/Cambridge_Bay");
  test_timezone(now, "America/Cuiaba");
  test_timezone(now, "America/Chicago");
  test_timezone(now, "America/Guyana");
  test_timezone(now, "America/Inuvik");
  test_timezone(now, "America/Asuncion");
  test_timezone(now, "America/Porto_Acre");
  test_timezone(now, "America/Hermosillo");
  test_timezone(now, "America/Shiprock");
  test_timezone(now, "America/Rio_Branco");
  test_timezone(now, "America/Yakutat");
  test_timezone(now, "America/Rankin_Inlet");
  test_timezone(now, "America/Boise");
  test_timezone(now, "Brazil/West");
  test_timezone(now, "Brazil/Acre");
  test_timezone(now, "Brazil/East");
  test_timezone(now, "Brazil/DeNoronha");
  test_timezone(now, "GMT0");
  test_timezone(now, "Libya");
  test_timezone(now, "W-SU");
  test_timezone(now, "NZ-CHAT");
  test_timezone(now, "Factory");
  test_timezone(now, "GB");
  test_timezone(now, "Australia/West");
  test_timezone(now, "Australia/Canberra");
  test_timezone(now, "Australia/Broken_Hill");
  test_timezone(now, "Australia/Eucla");
  test_timezone(now, "Australia/Currie");
  test_timezone(now, "Australia/South");
  test_timezone(now, "Australia/Lindeman");
  test_timezone(now, "Australia/Hobart");
  test_timezone(now, "Australia/Perth");
  test_timezone(now, "Australia/Yancowinna");
  test_timezone(now, "Australia/Victoria");
  test_timezone(now, "Australia/Sydney");
  test_timezone(now, "Australia/North");
  test_timezone(now, "Australia/Adelaide");
  test_timezone(now, "Australia/ACT");
  test_timezone(now, "Australia/Melbourne");
  test_timezone(now, "Australia/Tasmania");
  test_timezone(now, "Australia/Darwin");
  test_timezone(now, "Australia/Brisbane");
  test_timezone(now, "Australia/LHI");
  test_timezone(now, "Australia/NSW");
  test_timezone(now, "Australia/Queensland");
  test_timezone(now, "Australia/Lord_Howe");
  test_timezone(now, "NZ");
#endif
}

