/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018 László Várady <laszlo.varady@balabit.com>
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

#include "python-fetcher.h"
#include "python-logmsg.h"
#include "python-helpers.h"
#include "logthrsource/logthrfetcherdrv.h"
#include "str-utils.h"
#include "string-list.h"

typedef struct _PythonFetcherDriver
{
  LogThreadedFetcherDriver super;

  gchar *class;
  GList *loaders;
  GHashTable *options;

  struct
  {
    PyObject *class;
    PyObject *instance;
    PyObject *fetch_method;
    PyObject *open_method;
    PyObject *close_method;
    PyObject *request_exit_method;
  } py;
} PythonFetcherDriver;

typedef struct _PyLogFetcher
{
  PyObject_HEAD
  PythonFetcherDriver *driver;
} PyLogFetcher;

static PyTypeObject py_log_fetcher_type;


void
python_fetcher_set_class(LogDriver *s, gchar *filename)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  g_free(self->class);
  self->class = g_strdup(filename);
}

void
python_fetcher_set_option(LogDriver *s, gchar *key, gchar *value)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;
  gchar *normalized_key = __normalize_key(key);
  g_hash_table_insert(self->options, normalized_key, g_strdup(value));
}

void
python_fetcher_set_loaders(LogDriver *s, GList *loaders)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  string_list_free(self->loaders);
  self->loaders = loaders;
}

static const gchar *
python_fetcher_format_stats_instance(LogThreadedSourceDriver *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;
  static gchar persist_name[1024];

  if (s->super.super.super.persist_name)
    g_snprintf(persist_name, sizeof(persist_name), "python-fetcher,%s", s->super.super.super.persist_name);
  else
    g_snprintf(persist_name, sizeof(persist_name), "python-fetcher,%s", self->class);

  return persist_name;
}

static void
_pf_py_invoke_void_method_by_name(PythonFetcherDriver *self, const gchar *method_name)
{
  _py_invoke_void_method_by_name(self->py.instance, method_name, self->class, self->super.super.super.super.id);
}

static gboolean
_pf_py_invoke_bool_method_by_name_with_args(PythonFetcherDriver *self, const gchar *method_name)
{
  return _py_invoke_bool_method_by_name_with_args(self->py.instance, method_name, self->options, self->class,
                                                  self->super.super.super.super.id);
}

static void
_pf_py_invoke_void_function(PythonFetcherDriver *self, PyObject *func, PyObject *arg)
{
  return _py_invoke_void_function(func, arg, self->class, self->super.super.super.super.id);
}

static gboolean
_pf_py_invoke_bool_function(PythonFetcherDriver *self, PyObject *func, PyObject *arg)
{
  return _py_invoke_bool_function(func, arg, self->class, self->super.super.super.super.id);
}

static gboolean
_py_invoke_init(PythonFetcherDriver *self)
{
  return _pf_py_invoke_bool_method_by_name_with_args(self, "init");
}

static void
_py_invoke_deinit(PythonFetcherDriver *self)
{
  _pf_py_invoke_void_method_by_name(self, "deinit");
}

static void
_py_invoke_request_exit(PythonFetcherDriver *self)
{
  _pf_py_invoke_void_function(self, self->py.request_exit_method, NULL);
}

static gboolean
_py_invoke_open(PythonFetcherDriver *self)
{
  return _pf_py_invoke_bool_function(self, self->py.open_method, NULL);
}

static void
_py_invoke_close(PythonFetcherDriver *self)
{
  _pf_py_invoke_void_function(self, self->py.close_method, NULL);
}

static inline gboolean
_ulong_to_fetch_result(unsigned long ulong, ThreadedFetchResult *result)
{
  switch (ulong)
    {
    case THREADED_FETCH_ERROR:
    case THREADED_FETCH_NOT_CONNECTED:
    case THREADED_FETCH_SUCCESS:
      *result = (ThreadedFetchResult) ulong;
      return TRUE;

    default:
      return FALSE;
    }
}

static LogThreadedFetchResult
_py_invoke_fetch(PythonFetcherDriver *self)
{
  PyObject *ret = _py_invoke_function(self->py.fetch_method, NULL, self->class, self->super.super.super.super.id);

  if (!ret || !PyTuple_Check(ret) || PyTuple_Size(ret) > 2)
    goto error;

  PyObject *result = PyTuple_GetItem(ret, 0);
  PyLogMessage *pymsg = (PyLogMessage *) PyTuple_GetItem(ret, 1);

  if (!result || !PyLong_Check(result))
    goto error;

  LogThreadedFetchResult fetch_result = { .msg = NULL };
  if (!_ulong_to_fetch_result(PyLong_AsUnsignedLong(result), &fetch_result.result))
    goto error;

  if (fetch_result.result == THREADED_FETCH_SUCCESS)
    {
      if (!pymsg || !py_is_log_message((PyObject *) pymsg))
        goto error;

      /* keep a reference until the PyLogMessage instance is freed */
      fetch_result.msg = log_msg_ref(pymsg->msg);
    }

  Py_XDECREF(ret);
  PyErr_Clear();
  return fetch_result;

error:
  msg_error("Error in Python fetcher, fetch() must return a tuple (FetchResult, LogMessage)",
            evt_tag_str("driver", self->super.super.super.super.id),
            evt_tag_str("class", self->class));

  Py_XDECREF(ret);
  PyErr_Clear();

  LogThreadedFetchResult result_error = { THREADED_FETCH_ERROR, NULL };
  return result_error;
}

static gboolean
_py_is_log_fetcher(PyObject *obj)
{
  return PyType_IsSubtype(Py_TYPE(obj), &py_log_fetcher_type);
}

static gboolean
_py_init_bindings(PythonFetcherDriver *self)
{
  self->py.class = _py_resolve_qualified_name(self->class);
  if (!self->py.class)
    {
      gchar buf[256];

      msg_error("Error looking Python driver class",
                evt_tag_str("driver", self->super.super.super.super.id),
                evt_tag_str("class", self->class),
                evt_tag_str("exception", _py_format_exception_text(buf, sizeof(buf))));
      _py_finish_exception_handling();
      return FALSE;
    }

  self->py.instance = _py_invoke_function(self->py.class, NULL, self->class, self->super.super.super.super.id);
  if (!self->py.instance)
    {
      gchar buf[256];

      msg_error("Error instantiating Python driver class",
                evt_tag_str("driver", self->super.super.super.super.id),
                evt_tag_str("class", self->class),
                evt_tag_str("exception", _py_format_exception_text(buf, sizeof(buf))));
      _py_finish_exception_handling();
      return FALSE;
    }

  if (!_py_is_log_fetcher(self->py.instance))
    {
      msg_error("Error initializing Python fetcher, class is not a subclass of LogFetcher",
                evt_tag_str("driver", self->super.super.super.super.id),
                evt_tag_str("class", self->class));
      return FALSE;
    }

  ((PyLogFetcher *) self->py.instance)->driver = self;

  self->py.fetch_method = _py_get_attr_or_null(self->py.instance, "fetch");
  if (!self->py.fetch_method)
    {
      msg_error("Error initializing Python fetcher, class does not have a fetch() method",
                evt_tag_str("driver", self->super.super.super.super.id),
                evt_tag_str("class", self->class));
      return FALSE;
    }

  self->py.request_exit_method = _py_get_attr_or_null(self->py.instance, "request_exit");
  self->py.open_method = _py_get_attr_or_null(self->py.instance, "open");
  self->py.close_method = _py_get_attr_or_null(self->py.instance, "close");

  return TRUE;
}

static void
_py_free_bindings(PythonFetcherDriver *self)
{
  Py_CLEAR(self->py.class);
  Py_CLEAR(self->py.instance);
  Py_CLEAR(self->py.fetch_method);
  Py_CLEAR(self->py.open_method);
  Py_CLEAR(self->py.close_method);
  Py_CLEAR(self->py.request_exit_method);
}

static gboolean
_py_init_object(PythonFetcherDriver *self)
{
  if (!_py_get_attr_or_null(self->py.instance, "init"))
    {
      msg_debug("Missing Python method, init()",
                evt_tag_str("driver", self->super.super.super.super.id),
                evt_tag_str("class", self->class));
      return TRUE;
    }

  if (!_py_invoke_init(self))
    {
      msg_error("Error initializing Python driver object, init() returned FALSE",
                evt_tag_str("driver", self->super.super.super.super.id),
                evt_tag_str("class", self->class));
      return FALSE;
    }
  return TRUE;
}

static gboolean
python_fetcher_open(LogThreadedFetcherDriver *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  gboolean result = _py_invoke_open(self);
  PyGILState_Release(gstate);

  return result;
}

static void
python_fetcher_close(LogThreadedFetcherDriver *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  _py_invoke_close(self);
  PyGILState_Release(gstate);
}

static LogThreadedFetchResult
python_fetcher_fetch(LogThreadedFetcherDriver *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  LogThreadedFetchResult result = _py_invoke_fetch(self);
  PyGILState_Release(gstate);

  return result;
}

static void
python_fetcher_request_exit(LogThreadedFetcherDriver *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  _py_invoke_request_exit(self);
  PyGILState_Release(gstate);
}

static gboolean
python_fetcher_init(LogPipe *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  if (!self->class)
    {
      msg_error("Error initializing Python fetcher: no script specified!",
                evt_tag_str("driver", self->super.super.super.super.id));
      return FALSE;
    }

  self->super.time_reopen = 1;

  PyGILState_STATE gstate = PyGILState_Ensure();

  _py_perform_imports(self->loaders);
  if (!_py_init_bindings(self))
    goto fail;

  if (self->py.open_method)
    self->super.connect = python_fetcher_open;

  if (self->py.close_method)
    self->super.disconnect = python_fetcher_close;

  if (self->py.request_exit_method)
    self->super.request_exit = python_fetcher_request_exit;

  if (!_py_init_object(self))
    goto fail;

  PyGILState_Release(gstate);

  msg_verbose("Python fetcher initialized",
              evt_tag_str("driver", self->super.super.super.super.id),
              evt_tag_str("class", self->class));

  return log_threaded_fetcher_driver_init_method(s);

fail:
  PyGILState_Release(gstate);
  return FALSE;
}

static gboolean
python_fetcher_deinit(LogPipe *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  _py_invoke_deinit(self);
  PyGILState_Release(gstate);

  return log_threaded_fetcher_driver_deinit_method(s);
}

static void
python_fetcher_free(LogPipe *s)
{
  PythonFetcherDriver *self = (PythonFetcherDriver *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  _py_free_bindings(self);
  PyGILState_Release(gstate);

  g_free(self->class);
  g_hash_table_unref(self->options);
  string_list_free(self->loaders);

  log_threaded_fetcher_driver_free_method(s);
}

LogDriver *
python_fetcher_new(GlobalConfig *cfg)
{
  PythonFetcherDriver *self = g_new0(PythonFetcherDriver, 1);

  log_threaded_fetcher_driver_init_instance(&self->super, cfg);
  self->super.super.super.super.super.init = python_fetcher_init;
  self->super.super.super.super.super.deinit = python_fetcher_deinit;
  self->super.super.super.super.super.free_fn = python_fetcher_free;

  self->super.super.format_stats_instance = python_fetcher_format_stats_instance;
  self->super.super.worker_options.super.stats_level = STATS_LEVEL0;
  self->super.super.worker_options.super.stats_source = SCS_PYTHON;

  self->super.fetch = python_fetcher_fetch;

  self->options = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  return &self->super.super.super.super;
}


static PyTypeObject py_log_fetcher_type =
{
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  .tp_name = "LogFetcher",
  .tp_basicsize = sizeof(PyLogFetcher),
  .tp_dealloc = (destructor) PyObject_Del,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_doc = "The LogFetcher class is a base class for custom Python fetchers.",
  .tp_new = PyType_GenericNew,
  0,
};

void
py_log_fetcher_init(void)
{
  py_log_fetcher_type.tp_dict = PyDict_New();
  PyDict_SetItemString(py_log_fetcher_type.tp_dict, "FETCH_ERROR",
                       PyLong_FromLong(THREADED_FETCH_ERROR));
  PyDict_SetItemString(py_log_fetcher_type.tp_dict, "FETCH_NOT_CONNECTED",
                       PyLong_FromLong(THREADED_FETCH_NOT_CONNECTED));
  PyDict_SetItemString(py_log_fetcher_type.tp_dict, "FETCH_SUCCESS",
                       PyLong_FromLong(THREADED_FETCH_SUCCESS));

  PyType_Ready(&py_log_fetcher_type);
  PyModule_AddObject(PyImport_AddModule("syslogng"), "LogFetcher", (PyObject *) &py_log_fetcher_type);
}
