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

## Formatters ##################################################################

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

    def _make_header(self, timestamp, channel, level):
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

        return "%s [%s:%s]" % (timestamp, chan, lvl)

    def format(self, record):
        """Formats the log record as pretty-printed lines of the format:

        timestamp [CHANL:LEVL] message
        """
        level = record.levelname
        channel = record.name
        timestamp = self.formatTime(record, self.datefmt)
        header = self._make_header(timestamp, channel, level)
        # Pretty format the message
        indent = self._INDENT*self._indent
        formatted = ['%s %s%s' % (header, indent, line) for line in record.getMessage().split('\n')]
        formatted = '\n'.join(formatted)
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

## Implementation Details ######################################################

g_alog_formatter = None

def _add_new_level(name, value):
    logging.addLevelName(value, name.upper())

    log_using_self_func = lambda self, msg, *args, **kwargs: self.log(value, msg, *args, **kwargs)
    setattr(logging.Logger, name, log_using_self_func)

    log_using_logging_func = lambda msg, *args, **kwargs: logging.log(value, msg, *args, **kwargs)
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
    chan_map = {}
    if len(filters):
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

def configure(default_level, filters="", formatter='pretty'):
    """Top-level configuration function for the alog module. This function configures
    the logging package to use the given default level and overwrites the levels
    for all filters as specified. It can also configure the formatter type.

    Args:
        default_level (str):    This is the level that will be enabled for a
                                given channel when a specific level has not been
                                set in the filters.
        filters (str/dict):     This is a mapping from channel name to level that
                                allows levels to be set on a per-channel basis.
                                If a string, it is formatted as "CHAN:info,FOO:debug".
                                If a dict, it should map from channel string to
                                level string.
    """
    # Set up the formatter if different type
    _setup_formatter(formatter)

    # Set up root handler if not already set
    if not len(logging.root.handlers):
        handler = logging.StreamHandler()
        handler.setFormatter(g_alog_formatter)
        logging.root.addHandler(handler)

    # Add custom low levels
    for level, name in g_alog_level_to_name.items():
        if not hasattr(logging.Logger, name) and name not in ["off", "notset"]:
            _add_new_level(name, level)

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

## Convenience Helpers #########################################################

# The whole point of this class is act on scope changes
# pylint: disable=too-few-public-methods
class ScopedLog(object):
    """Scoped logging class that adds BEGIN/END lines and indents lines logged in
    the scope.
    """
    def __init__(self, log_fn, format_str="", *args):
        self.log_fn = log_fn
        self.format_str = format_str
        self.args = args
        self.log_fn("BEGIN: " + str(self.format_str), *self.args)
        global g_alog_formatter
        g_alog_formatter.indent()
    def __del__(self):
        global g_alog_formatter
        g_alog_formatter.deindent()
        self.log_fn("END: " + str(self.format_str), *self.args)

# The whole point of this class is act on scope changes
# pylint: disable=too-few-public-methods
class FnLog(ScopedLog):
    """Scoped log class that adds the function name to the BEGIN/END lines.
    """
    def __init__(self, log_fn, format_str="", *args):
        fn_name = traceback.format_stack()[-2].strip().split(',')[2].split(' ')[2].strip()
        format_str = "%s(" + format_str + ")"
        ScopedLog.__init__(self, log_fn, format_str, fn_name, *args)

# The whole point of this class is act on scope changes
# pylint: disable=too-few-public-methods
class ScopedTimer(object):
    """Scoped log class that starts a timer at construction and logs the time delta
    at destruction.
    """
    def __init__(self, log_fn, format_str="", *args):
        self.log_fn = log_fn
        self.format_str = format_str
        self.args = args
        self.start_time = time.time()

    def __del__(self):
        dt = str(timedelta(seconds=time.time() - self.start_time))
        fmt = self.format_str + "%s"
        args = list(self.args) + [dt]
        self.log_fn(fmt, *args)

## Testing #####################################################################

def demo_function(val):
    chan = logging.getLogger("FOO")
    fn_scope = FnLog(chan.info)
    chan.debug3("This is a test")
    chan.error({"test_json": True, "outer_message": "testing json logging"})
    # Test scoped logging
    inner_scope = ScopedLog(chan.debug, "inner")
    chan.info("Log with %s val", val)
    chan.info({"test_json": True, "inner_message": "Log with "+str(val)+" val"})
    del inner_scope
    chan.info("Log outside inner scope")

if __name__ == '__main__':
    import sys
    default_level = sys.argv[1] if len(sys.argv) > 1 else "info"
    filters = sys.argv[2] if len(sys.argv) > 2 else ""
    formatter = sys.argv[3] if len(sys.argv) > 3 else "pretty"
    configure(default_level=default_level, filters=filters, formatter=formatter)

    logging.info("TEST info")
    demo_function("bar")
    use_channel("FOO").debug2("Debug2 line %d", 10)
    use_channel("BAR").debug4("""Large, deep debugging entry with multiple
lines of text!""")
