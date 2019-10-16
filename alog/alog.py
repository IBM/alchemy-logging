"""
BEGIN_COPYRIGHT

IBM Confidential
OCO Source Materials

5727-I17
(C) Copyright IBM Corp. 2017 All Rights Reserved.

The source code for this program is not published or otherwise
divested of its trade secrets, irrespective of what has been
deposited with the U.S. Copyright Office.

END_COPYRIGHT
"""
import time
import json
import traceback
import logging
from datetime import datetime, timedelta
from threading import get_ident

## Formatters ##################################################################

g_thread_id_enabled = False

class AlogFormatterBase(logging.Formatter):
    """Base class with common functionality for alog formatters.
    """
    def __init__(self):
        self._indent = 0
        logging.Formatter.__init__(self)

    def formatTime(self, record, datefmt=None):
        """A wrapper for the parent formatTime that returns UTC timezone
        time stamp inISO format.

        Args:
            record (LogRecord):  Log record to pull the created date from.
            datefmt (str):       Ignored.

        Returns:
            A string representation of created datetime.
        """
        return datetime.utcfromtimestamp(record.created).isoformat()

    def indent(self):
        """Add a level of indentation.
        """
        self._indent += 1

    def deindent(self):
        """Remove a level of indentation.
        """
        if self._indent > 0:
            self._indent -= 1

class AlogJsonFormatter(AlogFormatterBase):
    """Log formatter which prints messages a single-line json.
    """
    _FIELDS_TO_PRINT = ['name', 'levelname', 'asctime', 'message', 'exc_text',
                        'region-id', 'org-id', 'tran-id', 'watson-txn-id', 'channel']

    def __init__(self):
        AlogFormatterBase.__init__(self)

    @staticmethod
    def _map_to_common_key_name(log_record_keyname):
        if log_record_keyname == 'levelname':
            return 'level_str'
        elif log_record_keyname == 'asctime':
            return 'timestamp'
        elif log_record_keyname == 'exc_text':
            return 'exception'
        elif log_record_keyname == 'name':
            return 'channel'
        else:
            return log_record_keyname

    def _extract_fields_from_record_as_dict(self, record):
        """Extracts the fields we want out of log record and puts them into an dict
        for easy jsonification.

        Args:
            record (logging.LogRecord): The log record object to extract from.

        Returns:
            The relevant fields pulled out from the log record object and
            initialized into a dictionary.
        """
        out = {}
        for field_name in self._FIELDS_TO_PRINT:
            if hasattr(record, field_name):
                record_field = getattr(record, field_name)
                if isinstance(record_field, dict):
                    out.update(record_field)
                else:
                    out[self._map_to_common_key_name(field_name)] = record_field

        out["level_str"] = out["level_str"].lower()
        return out

    def format(self, record):
        """Formats the log record as a JSON formatted string
       (also removes new line characters so everything prints on a single line)

        Args:
            record (logging.LogRecord):  The record to extract from.

        Returns:
            The jsonified string representation of the record.
        """
        # Maintain the message as a dict if passed in as one
        if isinstance(record.msg, dict):
            record.message = record.msg
        else:
            record.message = record.getMessage()

        record.asctime = self.formatTime(record, self.datefmt)
        if record.exc_info:
            record.exc_text = self.formatException(record.exc_info)

        if record.stack_info:
            record.stack_info = self.formatStack(record.stack_info)

        log_record = self._extract_fields_from_record_as_dict(record)

        # Add indent to all log records
        log_record['num_indent'] = self._indent

        # If enabled, add thread id
        if g_thread_id_enabled:
            log_record['thread_id'] = get_ident()

        return json.dumps(log_record, sort_keys=True)

class AlogPrettyFormatter(AlogFormatterBase):
    """Log formatter that pretty-prints lines for easy visibility.
    """
    _INDENT = "  "
    _LEVEL_MAP = {
        "critical": "FATL",
        "fatal": "FATL",
        "error": "ERRR",
        "warning": "WARN",
        "info": "INFO",
        "trace": "TRCE",
        "debug": "DBUG",
        "debug1": "DBG1",
        "debug2": "DBG2",
        "debug3": "DBG3",
        "debug4": "DBG4",
    }

    def __init__(self, channel_len=5):
        AlogFormatterBase.__init__(self)
        self.channel_len = channel_len

    def _make_header(self, timestamp, channel, level, log_code):
        """Create the header for a log line with proper padding.
        """
        # Get the padded or truncated channel
        chan = channel
        if len(channel) > self.channel_len:
            chan = channel[:self.channel_len]

        elif len(channel) < self.channel_len:
            chan = channel + ' ' * (self.channel_len - len(channel))

        # Get the mapped level
        lvl = self._LEVEL_MAP.get(level.lower(), "UNKN")

        # If thread id enabled, add it
        header = "%s [%s:%s" % (timestamp, chan, lvl)
        if g_thread_id_enabled:
            header += ":%d" % get_ident()
        header += "]"

        # Add log code if present
        if log_code is not None:
            header += " %s" % log_code

        return header

    def format(self, record):
        """Formats the log record as pretty-printed lines of the format:

        timestamp [CHANL:LEVL] message
        """
        # Extract special values from the message if it's a dict
        metadata = None
        if isinstance(record.msg, dict):
            if 'message' in record.msg:
                record.message = record.msg.pop('message')
            if 'log_code' in record.msg:
                record.log_code = record.msg.pop('log_code')
            metadata = record.msg
        else:
            record.message = record.getMessage()

        # Add metadata if present
        if not hasattr(record, 'message'):
            record.message = ''
        if metadata is not None and len(metadata) > 0:
            if len(record.message) > 0:
                record.message += ' '
            record.message += json.dumps(metadata)

        level = record.levelname
        channel = record.name
        timestamp = self.formatTime(record, self.datefmt)
        log_code = record.log_code if hasattr(record, 'log_code') else None
        header = self._make_header(timestamp, channel, level, log_code)
        # Pretty format the message
        indent = self._INDENT*self._indent
        if isinstance(record.message, str):
            formatted = ['%s %s%s' % (header, indent, line) for line in record.message.split('\n')]
            formatted = '\n'.join(formatted)
        else:
            formatted = '%s %s%s' % (header, indent, str(record.message))
        return formatted

## Constants ###################################################################

# Global maps from name <-> level, pull these from logging packages for easy consistency
# pylint: disable=protected-access
g_alog_level_to_name = {level: name.lower() for level, name in logging._levelToName.items()}

# Extra custom log levels
g_alog_level_to_name.update({
    60: "off",
    15: "trace",
    9: "debug1",
    8: "debug2",
    7: "debug3",
    6: "debug4",
})

g_alog_name_to_level = {name: level for level, name in g_alog_level_to_name.items()}

# Global map of default formatters
g_alog_formatters = {
    "json": AlogJsonFormatter,
    "pretty": AlogPrettyFormatter,
}

scope_start_str = "BEGIN: "
scope_end_str = "END: "

## Implementation Details ######################################################

g_alog_formatter = None

def is_log_code(arg):
    return arg.startswith('<') and arg.endswith('>')

def _log_with_code_method_override(self, value, arg_one, *args, **kwargs):
    """This helper is used as an override to the native logging.Logger instance
    methods for each level. As such, it's first argument, self, is the logger
    instance (or the global root logger singleton) on which to call the method.
    Having this as the first argument allows it to override the native methods
    and support functionality like:

    ch = alog.use_channel('FOO')
    ch.debug('<FOO12345678I>', 'Logging is fun!')
    """

    # If no positional args, arg_one is message
    if len(args) == 0:
        self.log(value, arg_one, **kwargs)

    # If arg_one looks like a log code, use the first positional arg as message
    elif is_log_code(arg_one):
        self.log(value, {"log_code": arg_one, "message": args[0] % tuple(args[1:])}, **kwargs)

    # Otherwise, treat arg_one as the message
    else:
        self.log(value, arg_one, *args, **kwargs)

def _add_level_fn(name, value):
    logging.addLevelName(value, name.upper())

    log_using_self_func = lambda self, arg_one, *args, **kwargs: \
        _log_with_code_method_override(self, value, arg_one, *args, **kwargs)
    setattr(logging.Logger, name, log_using_self_func)

    log_using_logging_func = lambda arg_one, *args, **kwargs: \
        _log_with_code_method_override(logging, value, arg_one, *args, **kwargs)
    setattr(logging, name, log_using_logging_func)

def _setup_formatter(formatter):
    # If the formatter is a string, pull it from the defaults
    global g_alog_formatter
    if isinstance(formatter, str):
        # Get the formatter class
        fmt_class = g_alog_formatters.get(formatter, None)
        if fmt_class is None:
            raise ValueError("Invalid formatter: %s" % formatter)

        # Set up the formatter if different type
        if not isinstance(g_alog_formatter, fmt_class):
            g_alog_formatter = fmt_class()

    # If the formatter is a valid AlogFormatterBase, use it directly
    elif isinstance(formatter, AlogFormatterBase):
        g_alog_formatter = formatter

    # Otherwise, log a warning and use the pretty printer
    else:
        raise ValueError("Invalid formatter type: %s" % type(formatter))

def _parse_filters(filters):
    # Check to see if we've got a dictionary. If we do, keep the valid filter entries
    if isinstance(filters, dict):
        return _parse_dict_of_filters(filters)
    elif isinstance(filters, str):
        if len(filters):
            return _parse_str_of_filters(filters)
        else:
            return {}
    else:
        logging.warning("Invalid filter type [%s] was ignored!", type(filters).__name__)
        return {}

def _parse_dict_of_filters(filters):
    for entry, level_name in filters.items():
        if g_alog_name_to_level.get(level_name, None) is None:
            logging.warning("Invalid filter entry [%s]", entry)
            del filters[entry]
    return filters

def _parse_str_of_filters(filters):
    chan_map = {}
    for entry in filters.split(','):
        if len(entry):
            parts = entry.split(':')
            if len(parts) != 2:
                logging.warning("Invalid filter entry [%s]", entry)
            else:
                chan, level_name = parts
                level = g_alog_name_to_level.get(level_name, None)
                if level is None:
                    logging.warning("Invalid level [%s] for channel [%s]", level_name, chan)
                else:
                    chan_map[chan] = level_name
        else:
            logging.warning("Invalid filter entry [%s]", entry)
    return chan_map

## Core ########################################################################

def configure(default_level, filters="", formatter='pretty', thread_id=False):
    """Top-level configuration function for the alog module. This function configures
    the logging package to use the given default level and overwrites the levels
    for all filters as specified. It can also configure the formatter type.

    Args:
        default_level   str
            This is the level that will be enabled for a given channel when a
            specific level has not been set in the filters.
        filters         str/dict
            This is a mapping from channel name to level that allows levels to
            be set on a per-channel basis. If a string, it is formatted as
            "CHAN:info,FOO:debug". If a dict, it should map from channel string
            to level string.
        formatter       str ('pretty' or 'json')/AlogFormatterBase
            The formatter is either the string 'pretty' or 'json' to indicate
            one of the default formatting options or an instance of
            AlogFormatterBase for a custom formatter implementation
        thread_id       bool
            If true, include thread
    """
    # Set up the formatter if different type
    _setup_formatter(formatter)

    # Set up thread id logging
    global g_thread_id_enabled
    g_thread_id_enabled = thread_id

    # Remove any existing handlers
    formatters = [h for h in logging.root.handlers if isinstance(h.formatter, AlogFormatterBase)]
    for handler in formatters:
        logging.root.removeHandler(handler)

    # Add the formatter
    handler = logging.StreamHandler()
    handler.setFormatter(g_alog_formatter)
    logging.root.addHandler(handler)

    # Add custom low levels
    for level, name in g_alog_level_to_name.items():
        if name not in ["off", "notset"]:
            _add_level_fn(name, level)

    # Set default level
    default_level_val = g_alog_name_to_level.get(default_level, None)
    if default_level_val is not None:
        logging.root.setLevel(default_level_val)
        for handler in logging.root.handlers:
            handler.setLevel(default_level_val)
    else:
        logging.warning("Invalid default_level [%s]", default_level)

    # Add level filters by name
    # NOTE: All levels assumed valid after call to _parse_filters
    for chan, level_name in _parse_filters(filters).items():
        level = g_alog_name_to_level[level_name]
        handler = logging.StreamHandler()
        handler.setFormatter(g_alog_formatter)
        handler.setLevel(level)
        lgr = logging.getLogger(chan)
        lgr.setLevel(level)
        lgr.propagate = False
        lgr.addHandler(handler)

def use_channel(channel):
    """Interface wrapper for python alog implementation to keep consistency with
    other languages.
    """
    return logging.getLogger(channel)

## Scoped Loggers ##############################################################

# The whole point of this class is act on scope changes
# pylint: disable=too-few-public-methods
class _ScopedLogBase:
    """FIXME
    """
    def __init__(self, log_fn, format_str, *args):
        """FIXME
        """
        self.log_fn = log_fn
        self.format_str = format_str
        self.args = args

    def _start_scoped_log(self):
        """FIXME
        """
        self.log_fn(scope_start_str + str(self.format_str), *self.args)
        global g_alog_formatter
        g_alog_formatter.indent()

    def _end_scoped_log(self):
        """FIXME
        """
        global g_alog_formatter
        g_alog_formatter.deindent()
        self.log_fn(scope_end_str + str(self.format_str), *self.args)

# pylint: disable=too-few-public-methods
class ScopedLog(_ScopedLogBase):
    """FIXME
    """
    def __init__(self, log_fn, format_str="", *args):
        """FIXME
        """
        super().__init__(log_fn, format_str, *args)
        self._start_scoped_log()

    def __del__(self):
        """FIXME
        """
        self._end_scoped_log()

# pylint: disable=too-few-public-methods
class ContextLog(_ScopedLogBase):
    """FIXME
    """
    def __enter__(self):
        """FIXME
        """
        self._start_scoped_log()
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        """FIXME
        """
        self._end_scoped_log()

# pylint: disable=too-few-public-methods
class FunctionLog(ScopedLog):
    """Scoped log class that adds the function name to the BEGIN/END lines.
    """
    def __init__(self, log_fn, format_str="", *args):
        fn_name = traceback.format_stack()[-2].strip().split(',')[2].split(' ')[2].strip()
        format_str = "%s(" + format_str + ")"
        super().__init__(log_fn, format_str, fn_name, *args)

# for compatibility
class FnLog(FunctionLog):
    pass

def logged_function(func):
    pass

## Timers ######################################################################

class _TimedLogBase:
    def __init__(self, log_fn, format_str, *args):
        """FIXME
        """
        self.log_fn = log_fn
        self.format_str = format_str
        self.args = args
        self.start_time = 0

    def _start_timed_log(self):
        """FIXME
        """
        self.start_time = time.time()

    def _end_timed_log(self):
        """FIXME
        """
        duration = str(timedelta(seconds=time.time() - self.start_time))
        fmt = self.format_str + "%s"
        args = list(self.args) + [duration]
        self.log_fn(fmt, *args)

# pylint: disable=too-few-public-methods
class ScopedTimer(_TimedLogBase):
    """Scoped log class that starts a timer at construction and logs the time delta at destruction.
    """
    def __init__(self, log_fn, format_str="", *args):
        super().__init__(log_fn, format_str, *args)
        self._start_timed_log()

    def __del__(self):
        self._end_timed_log()

class ContextTimer(_TimedLogBase):
    def __enter__(self):
        """FIXME
        """
        self._start_timed_log()
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        """FIXME
        """
        self._end_timed_log()

def timed_function(func):
    pass

## Testing #####################################################################

@logged_function(chan.info, "function")
def demo_function(val):
    chan = logging.getLogger("FOO")
    fn_scope = FnLog(chan.info)
    chan.debug3("This is a test")
    chan.error({"test_json": True, "outer_message": "testing json logging"})
    # Test scoped logging
    with ContextLog(chan.info, "inner"):
        chan.info("I am scoped")
        chan.info({"test_json": True, "inner_message": "Log with "+str(val)+" val"})
    chan.info("Log outside inner scope")

if __name__ == '__main__':
    import sys
    import time
    default_level = sys.argv[1] if len(sys.argv) > 1 else "info"
    filters = sys.argv[2] if len(sys.argv) > 2 else ""
    formatter = sys.argv[3] if len(sys.argv) > 3 else "pretty"
    configure(default_level=default_level, filters=filters, formatter=formatter)

    logging.info("TEST info")
    demo_function("bar")
    use_channel("FOO").debug2("Debug2 line %d", 10)
    use_channel("BAR").debug4("""Large, deep debugging entry with multiple
lines of text!""")
    test_ch = use_channel("TEST")
    test_ch.info("<TST12345678I>", "This is a line with a log code")

    # Sample scoped timer
    with ContextTimer(test_ch.info, 'Finished timer context: ') as t:
        time.sleep(1)
        test_ch.info('Done with the scope')
