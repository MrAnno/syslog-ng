/*
 * Copyright (c) 2010-2015 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2010-2015 Viktor Juhasz <viktor.juhasz@balabit.com>
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

#include "messages.h"
#include "org_syslog_ng_InternalMessageSender.h"

static void _append_message_tags(JNIEnv *env, jobjectArray tags, EVTREC *msg_event)
{
  jclass internal_message_tag = (*env)->FindClass(env, "org/syslog_ng/InternalMessageTag");
  jmethodID internal_message_tag_name = (*env)->GetMethodID(env, internal_message_tag, "getTagName", "()Ljava/lang/String;");
  jmethodID internal_message_tag_value = (*env)->GetMethodID(env, internal_message_tag, "getTagValue", "()Ljava/lang/String;");

  jsize size = (*env)->GetArrayLength(env, tags);
  for (jint i = 0; i < size; ++i)
    {
      jobject message_tag = (*env)->GetObjectArrayElement(env, tags, i);
      jstring tag_name = (*env)->CallObjectMethod(env, message_tag, internal_message_tag_name);
      jstring tag_value = (*env)->CallObjectMethod(env, message_tag, internal_message_tag_value);

      const char *tag_name_str = (*env)->GetStringUTFChars(env, tag_name, NULL);
      const char *tag_value_str = (*env)->GetStringUTFChars(env, tag_value, NULL);

      msg_event_add_tag(msg_event, evt_tag_str(tag_name_str, tag_value_str));

      (*env)->ReleaseStringUTFChars(env, tag_name, tag_name_str);
      (*env)->ReleaseStringUTFChars(env, tag_value, tag_value_str);
    }
}

JNIEXPORT void JNICALL
Java_org_syslog_1ng_InternalMessageSender_createInternalMessage(JNIEnv *env, jclass cls, jint pri, jstring message, jobjectArray tags)
{
  if ((pri != org_syslog_ng_InternalMessageSender_MsgDebug) || debug_flag)
    {
      const char *c_str = (*env)->GetStringUTFChars(env, message, 0);

      EVTREC *msg_event = msg_event_create_from_desc(pri, c_str);
      _append_message_tags(env, tags, msg_event);
      msg_event_suppress_recursions_and_send(msg_event);

      (*env)->ReleaseStringUTFChars(env, message, c_str);
    }
}
