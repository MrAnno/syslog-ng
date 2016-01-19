/*
 * Copyright (c) 2014 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2014 Viktor Juhasz <viktor.juhasz@balabit.com>
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


package org.syslog_ng;

public class InternalMessageTag {
  private String tagName;
  private String tagValue;

  public InternalMessageTag(String tagName, int tagValue) {
    this(tagName, Integer.toString(tagValue));
  }

  public InternalMessageTag(String tagName, String tagValue) {
    this.tagName = tagName;
    this.tagValue = tagValue;
  }

  public String getTagName() {
    return tagName;
  }

  public String getTagValue() {
    return tagValue;
  }
}
