#!/usr/bin/env python3

import gdb
import re
import json

def syslogng_mainloop():
    return gdb.lookup_symbol("main_loop")[0].value().address

def syslogng_cfg():
    return syslogng_mainloop()["current_configuration"]

def syslogng_logmsg_get_message(logmsg):
    LM_V_MESSAGE = 3
    entry_type = gdb.lookup_type("NVEntry").pointer()
    byte_type = gdb.lookup_type("char").pointer()

    payload = logmsg["payload"]
    payload_size = payload["size"]

    ofs = payload["static_entries"][LM_V_MESSAGE - 1]
    top = payload.cast(byte_type)[payload_size]
    entry =  top.address[0-ofs].address.cast(entry_type)

    if entry["unset"]:
        return "[Empty MESSAGE]"

    if entry["indirect"]:
        return "[Indirect MESSAGE] - Not implemented"

    name_len = entry["name_len"]
    message = entry["vdirect"]["data"][name_len + 1].address
    return message.string()

def syslogng_dump_queue(fifo, subqueue_name):
    node_type = gdb.lookup_type("LogMessageQueueNode").pointer()
    subqueue = fifo[subqueue_name]
    subqueue_len = subqueue["len"]

    subqueue = subqueue["items"]

    messages = []
    while subqueue_len > 0:
        msg = subqueue["next"].cast(node_type)["msg"]

        messages.append(syslogng_logmsg_get_message(msg))

        subqueue = subqueue["next"]
        subqueue_len -= 1

    return messages

def syslogng_dump_wait_queue(fifo):
    return syslogng_dump_queue(fifo, "wait_queue")

def syslogng_dump_output_queue(fifo):
    return syslogng_dump_queue(fifo, "output_queue")

class SyslogNg(gdb.Command):
    """syslog-ng helper commands"""
    def __init__(self):
        super(SyslogNg, self).__init__("syslogng", gdb.COMMAND_USER, prefix=True)
    def invoke(self, arg, from_tty):
        gdb.write("\nAvailable syslog-ng commands, functions and variables:\n\n")
        commands = gdb.execute("help user-defined", to_string=True)
        functions = gdb.execute("help function", to_string=True)
        variables = gdb.execute("show conv", to_string=True)

        for line in commands.splitlines():
            if re.match("syslogng", line):
                gdb.write(line + "\n")
        gdb.write("\n")

        for line in functions.splitlines():
            if re.search("syslogng", line):
                gdb.write(line + "\n")
        gdb.write("\n")

        for line in variables.splitlines():
            res = re.search(r"(\$syslogngvar.*) =", line)
            if re.search("syslogngvar", line):
                gdb.write(res.group(1) + "\n")
        gdb.write("\n")

class SyslogNgInfo(gdb.Command):
    """Print short info about the syslog-ng instance"""
    def __init__(self):
        super(SyslogNgInfo, self).__init__("syslogng info", gdb.COMMAND_USER)
    def invoke(self, arg, from_tty):
        resolved_conf = gdb.lookup_global_symbol("resolvedConfigurablePaths")
        gdb.write("\nresolvedConfigurablePaths = {}\n\n".format(resolved_conf.value()))

class SyslogNgListPipes(gdb.Command):
    """List all initialized pipe objects"""
    def __init__(self):
        super(SyslogNgListPipes, self).__init__("syslogng list-pipes", gdb.COMMAND_USER)
    def invoke(self, arg, from_tty):
        logpipe_type = gdb.lookup_type("LogPipe").pointer()
        logdriver_type = gdb.lookup_type("LogDriver")
        initialized_pipes = syslogng_cfg()["tree"]["initialized_pipes"]
        pipe_array = initialized_pipes["pdata"]
        pipe_array_len = initialized_pipes["len"]

        gdb.write("\n")
        for i in range(0, pipe_array_len):
            logpipe = pipe_array[i].cast(logpipe_type)
            if not logpipe:
                continue

            logpipe = logpipe.dereference()
            plugin_name = logpipe["plugin_name"].string() if logpipe["plugin_name"] else None

            if plugin_name:
                logdriver = logpipe.cast(logdriver_type)
                expr_node = logpipe["expr_node"]
                if expr_node and expr_node["filename"]:
                    location = "{}:{}:{}".format(expr_node["filename"].string(), expr_node["line"], expr_node["column"])
                else:
                    location = "[unknown location]"

                id = logdriver["id"].string() if logdriver["id"] else "[no ID]"
                gdb.write("[{}]\t {}:{} ({})\n".format(i, id, plugin_name, location))

        gdb.write("\n")

class SyslogNgGetPipe(gdb.Function):
    """$syslogng_get_pipe(index) - get pipe object by index"""
    def __init__(self):
        super(SyslogNgGetPipe, self).__init__("syslogng_get_pipe")
    def invoke(self, index):
        initialized_pipes = syslogng_cfg()["tree"]["initialized_pipes"]
        pipe_array = initialized_pipes["pdata"]
        pipe_array_len = initialized_pipes["len"]
        logpipe_type = gdb.lookup_type("LogPipe").pointer()

        if not index in range(0, pipe_array_len):
            gdb.write('Invalid index, use syslogng list-pipes\n')
            return gdb.Value(0)

        return pipe_array[index].cast(logpipe_type)

class SyslogNgGetMsg(gdb.Function):
    """$syslogng_get_msg(msg) - get msg"""
    def __init__(self):
        super(SyslogNgGetMsg, self).__init__("syslogng_get_msg")
    def invoke(self, msg):
        return syslogng_logmsg_get_message(msg)

class SyslogNgDumpQueue(gdb.Function):
    """$syslogng_dump_queue(log_queue) - dump destination queue"""
    def __init__(self):
        super(SyslogNgDumpQueue, self).__init__("syslogng_dump_queue")
    def invoke(self, log_queue):
        log_queue_fifo_type = gdb.lookup_type("LogQueueFifo").pointer()
        fifo = log_queue.cast(log_queue_fifo_type)

        messages = syslogng_dump_wait_queue(fifo)
        messages += syslogng_dump_output_queue(fifo)

        gdb.write(json.dumps(messages, indent=2) + "\n")
        return gdb.Value(0)

def main():
    gdb.execute("set print pretty on")

    # TODO: gdb < 8 compat
    #gdb.set_convenience_variable("syslogngvar_mainloop", syslogng_mainloop())
    #gdb.set_convenience_variable("syslogngvar_globalconfig", syslogng_cfg())
    #gdb.set_convenience_variable("syslogngvar_config", syslogng_cfg()["original_config"]["str"])
    #gdb.set_convenience_variable("syslogngvar_preprocessedconfig", syslogng_cfg()["preprocess_config"]["str"])
    #gdb.set_convenience_variable("syslogngvar_paths", gdb.lookup_global_symbol("resolvedConfigurablePaths").value())

    SyslogNg()
    SyslogNgInfo()
    SyslogNgListPipes()
    SyslogNgGetPipe()
    SyslogNgDumpQueue()
    SyslogNgGetMsg()

if gdb.lookup_symbol("main_loop")[0] and gdb.lookup_global_symbol("resolvedConfigurablePaths"):
    main()
else:
    gdb.write("Can not find syslog-ng symbols, exiting.\n")
