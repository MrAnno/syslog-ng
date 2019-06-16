/*
 * Copyright (c) 2018 Balabit
 * Copyright (c) 2018-2019 László Várady <laszlo.varady@balabit.com>
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
#include "python-source.h"
#include "python-logmsg.h"
#include "python-helpers.h"
#include "str-utils.h"
#include "string-list.h"

typedef struct _PythonFetcher
{
  LogThreadedFetcher super;

  struct
  {
    PyObject *fetch_method;
    PyObject *open_method;
    PyObject *close_method;
    PyObject *request_exit_method;
  } py;

} PythonFetcher;

typedef struct _PyLogFetcher
{
  PyObject_HEAD
} PyLogFetcher;

static PyTypeObject py_log_fetcher_type;


static void
_pf_py_invoke_void_function(PythonSourceDriver *owner, PyObject *func, PyObject *arg)
{
  return _py_invoke_void_function(func, arg, python_sd_get_class(owner), python_sd_get_driver_id(owner));
}

static gboolean
_pf_py_invoke_bool_function(PythonSourceDriver *owner, PyObject *func, PyObject *arg)
{
  return _py_invoke_bool_function(func, arg, python_sd_get_class(owner), python_sd_get_driver_id(owner));
}

static void
_py_invoke_request_exit(PythonFetcher *self)
{
  PythonSourceDriver *owner = (PythonSourceDriver *) self->super.super.owner;
  _pf_py_invoke_void_function(owner, self->py.request_exit_method, NULL);
}

static gboolean
_py_invoke_open(PythonFetcher *self)
{
  PythonSourceDriver *owner = (PythonSourceDriver *) self->super.super.owner;
  return _pf_py_invoke_bool_function(owner, self->py.open_method, NULL);
}

static void
_py_invoke_close(PythonFetcher *self)
{
  PythonSourceDriver *owner = (PythonSourceDriver *) self->super.super.owner;
  _pf_py_invoke_void_function(owner, self->py.close_method, NULL);
}

static inline gboolean
_ulong_to_fetch_result(unsigned long ulong, ThreadedFetchResult *result)
{
  switch (ulong)
    {
    case THREADED_FETCH_ERROR:
    case THREADED_FETCH_NOT_CONNECTED:
    case THREADED_FETCH_SUCCESS:
    case THREADED_FETCH_TRY_AGAIN:
    case THREADED_FETCH_NO_DATA:
      *result = (ThreadedFetchResult) ulong;
      return TRUE;

    default:
      return FALSE;
    }
}

static ThreadedFetchResult
_py_invoke_fetch(PythonFetcher *self, LogMessage **msg)
{
  PythonSourceDriver *owner = (PythonSourceDriver *) self->super.super.owner;

  PyObject *ret = _py_invoke_function(self->py.fetch_method, NULL, python_sd_get_class(owner),
                  python_sd_get_driver_id(owner));

  if (!ret || !PyTuple_Check(ret) || PyTuple_Size(ret) > 2)
    goto error;

  PyObject *result = PyTuple_GetItem(ret, 0);
  if (!result || !PyLong_Check(result))
    goto error;

  ThreadedFetchResult fetch_result;
  if (!_ulong_to_fetch_result(PyLong_AsUnsignedLong(result), &fetch_result))
    goto error;

  if (fetch_result == THREADED_FETCH_SUCCESS)
    {
      PyLogMessage *pymsg = (PyLogMessage *) PyTuple_GetItem(ret, 1);
      if (!pymsg || !py_is_log_message((PyObject *) pymsg))
        goto error;

      /* keep a reference until the PyLogMessage instance is freed */
      *msg = log_msg_ref(pymsg->msg);
    }

  Py_XDECREF(ret);
  PyErr_Clear();
  return fetch_result;

error:
  msg_error("Error in Python fetcher, fetch() must return a tuple (FetchResult, LogMessage)",
            evt_tag_str("driver", python_sd_get_driver_id(owner)),
            evt_tag_str("class", python_sd_get_class(owner)));

  Py_XDECREF(ret);
  PyErr_Clear();

  return THREADED_FETCH_ERROR;
}

gboolean
_py_is_log_fetcher(PyObject *obj)
{
  return PyType_IsSubtype(Py_TYPE(obj), &py_log_fetcher_type);
}

static void
_py_free_methods(PythonFetcher *self)
{
  Py_CLEAR(self->py.fetch_method);
  Py_CLEAR(self->py.open_method);
  Py_CLEAR(self->py.close_method);
  Py_CLEAR(self->py.request_exit_method);
}


static gboolean
_py_lookup_fetch_method(PythonFetcher *self)
{
  PythonSourceDriver *owner = (PythonSourceDriver *) self->super.super.owner;

  self->py.fetch_method = _py_get_attr_or_null(python_sd_get_py_instance(owner), "fetch");

  if (!self->py.fetch_method)
    {
      msg_error("Error initializing Python fetcher, class does not have a fetch() method",
                evt_tag_str("driver", python_sd_get_driver_id(owner)),
                evt_tag_str("class", python_sd_get_class(owner)));
      return FALSE;
    }

  return TRUE;
}

static gboolean
_py_init_methods(PythonFetcher *self)
{
  PythonSourceDriver *owner = (PythonSourceDriver *) self->super.super.owner;

  if (!_py_lookup_fetch_method(self))
    return FALSE;

  self->py.request_exit_method = _py_get_attr_or_null(python_sd_get_py_instance(owner), "request_exit");
  self->py.open_method = _py_get_attr_or_null(python_sd_get_py_instance(owner), "open");
  self->py.close_method = _py_get_attr_or_null(python_sd_get_py_instance(owner), "close");

  return TRUE;
}

static gboolean
python_fetcher_open(LogThreadedFetcher *s)
{
  PythonFetcher *self = (PythonFetcher *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  gboolean result = _py_invoke_open(self);
  PyGILState_Release(gstate);

  return result;
}

static void
python_fetcher_close(LogThreadedFetcher *s)
{
  PythonFetcher *self = (PythonFetcher *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  _py_invoke_close(self);
  PyGILState_Release(gstate);
}

static void
python_fetcher_request_exit(LogThreadedFetcher *s)
{
  PythonFetcher *self = (PythonFetcher *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  _py_invoke_request_exit(self);
  PyGILState_Release(gstate);
}

static gboolean
_py_fetcher_init(PythonFetcher *self)
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  if (!_py_init_methods(self))
    {
      _py_free_methods(self);
      PyGILState_Release(gstate);
      return FALSE;
    }

  if (self->py.open_method)
    self->super.connect = python_fetcher_open;

  if (self->py.close_method)
    self->super.disconnect = python_fetcher_close;

  if (self->py.request_exit_method)
    self->super.request_exit = python_fetcher_request_exit;

  PyGILState_Release(gstate);
  return TRUE;
}

static LogThreadedFetchResult
python_fetcher_fetch(LogThreadedFetcher *s)
{
  PythonFetcher *self = (PythonFetcher *) s;
  LogThreadedFetchResult fetch_result;

  PyGILState_STATE gstate = PyGILState_Ensure();
  {
    LogMessage *msg = NULL;
    ThreadedFetchResult result = _py_invoke_fetch(self, &msg);

    fetch_result = (LogThreadedFetchResult)
    {
      result, msg
    };
  }
  PyGILState_Release(gstate);

  return fetch_result;
}

static gboolean
python_fetcher_init(LogPipe *s)
{
  PythonFetcher *self = (PythonFetcher *) s;

  self->super.time_reopen = 1;

  if (!_py_fetcher_init(self))
    return FALSE;

  return log_threaded_fetcher_init_method(s);
}

static void
python_fetcher_free(LogPipe *s)
{
  PythonFetcher *self = (PythonFetcher *) s;

  PyGILState_STATE gstate = PyGILState_Ensure();
  _py_free_methods(self);
  PyGILState_Release(gstate);

  log_threaded_fetcher_free_method(s);
}

LogThreadedSourceWorker *
python_fetcher_new(LogThreadedSourceDriver *drv)
{
  GlobalConfig *cfg = log_pipe_get_config(&drv->super.super.super);

  PythonFetcher *self = g_new0(PythonFetcher, 1);
  log_threaded_fetcher_init_instance(&self->super, cfg);

  self->super.fetch = python_fetcher_fetch;

  self->super.super.super.super.init = python_fetcher_init;
  self->super.super.super.super.free_fn = python_fetcher_free;

  return &self->super.super;
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
  PyDict_SetItemString(py_log_fetcher_type.tp_dict, "FETCH_TRY_AGAIN",
                       PyLong_FromLong(THREADED_FETCH_SUCCESS));
  PyDict_SetItemString(py_log_fetcher_type.tp_dict, "FETCH_NO_DATA",
                       PyLong_FromLong(THREADED_FETCH_SUCCESS));

  PyType_Ready(&py_log_fetcher_type);
  PyModule_AddObject(PyImport_AddModule("_syslogng"), "LogFetcher", (PyObject *) &py_log_fetcher_type);
}
