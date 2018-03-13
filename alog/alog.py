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

import json
import logging
import logging.config
from datetime import datetime
from os import environ

## Formatters ##################################################################

class AlogFormatterBase(logging.Formatter):
  """
  Base class with common functionality for alog formatters
  """

  def formatTime(self, record, datefmt=None):
    """
    A wrapper for the parent formatTime that returns UTC timezone time stamp in ISO format
    :param LogRecord record: Log record to pull the created date from
    :param str datefmt: Ignored
    :return: string representation of created datetime
    :rtype: str
    """
    return datetime.utcfromtimestamp(record.created).isoformat()

DEFAULT_APP_VERSION = "1.0-SNAPSHOT-local"
class AlogJsonFormatter(AlogFormatterBase):
  """
  Log formatter which prints messages a single-line json
  """

  _FIELDS_TO_PRINT = ['name', 'levelname', 'asctime', 'message', 'exc_text',
    'region-id', 'org-id', 'tran-id', 'watson-txn-id', 'channel']
  _app_version = environ.get("SERVICE_VERSION", DEFAULT_APP_VERSION)

  @staticmethod
  def _map_to_common_key_name(log_record_keyname):
    if log_record_keyname == 'levelname':
      return 'level'
    elif log_record_keyname == 'asctime':
      return 'timestamp'
    elif log_record_keyname == 'exc_text':
      return 'exception'
    elif log_record_keyname == 'name':
      return 'logger_name'
    else:
      return log_record_keyname

  def _extract_fields_from_record_as_dict(self, record):
    """
    Extracts the fields we want out of log record and puts them into an dict
      for easy jsonification.
    :param logging.LogRecord record: the log record object to extract from
    :return The relevant fields pulled out from the log record object and initialized
      into a dictionary
    :rtype dict
    """
    return {self._map_to_common_key_name(field_name): getattr(record, field_name) for field_name in
          self._FIELDS_TO_PRINT if hasattr(record, field_name)}

  def format(self, record):
    """
    Formats the log record as a JSON formatted string (also removes new line characters so everything
    prints on a single line)
    :param logging.LogRecord record: the record to extract from
    :return the jsonified string representation of the record
    :rtype str
    """
    record.message = record.getMessage().replace('\n', '\t')
    record.asctime = self.formatTime(record, self.datefmt)
    if record.exc_text:
      record.exc_text = record.exc_text.replace('\n', '\t')
    elif record.exc_info:
      record.exc_text = self.formatException(record.exc_info)
    if record.stack_info:
      record.stack_info = self.formatStack(record.stack_info)
    log_record = self._extract_fields_from_record_as_dict(record)
    # Add app-version field to all log records
    log_record['app-version'] = self._app_version
    return json.dumps(log_record, sort_keys=True)

class AlogPrettyFormatter(AlogFormatterBase):
  """
  Log formatter that pretty-prints lines for easy visibility
  """
  _CHANNEL_PRINT_LEN = 5
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

  def _make_header(self, timestamp, channel, level):
    """
    Create the header for a log line with proper padding
    """

    # Get the padded or truncated channel
    ch = channel
    if len(channel) > self._CHANNEL_PRINT_LEN:
      ch = channel[:self._CHANNEL_PRINT_LEN]
    elif len(channel) < self._CHANNEL_PRINT_LEN:
      ch = channel + ' ' * (self._CHANNEL_PRINT_LEN - len(channel))

    # Get the mapped level
    lvl = self._LEVEL_MAP.get(level.lower(), "UNKN")

    return "%s [%s:%s]" % (timestamp, ch, lvl)

  def format(self, record):
    """
    Formats the log record as pretty-printed lines of the format:

    timestamp [CHANL:LEVL] message
    """
    level = record.levelname
    channel = record.name
    timestamp = self.formatTime(record, self.datefmt)
    header = self._make_header(timestamp, channel, level)
    formatted = '\n'.join(['%s %s' % (header, line) for line in record.getMessage().split('\n')])
    return formatted

## Constants ###################################################################

g_alog_level_to_name = dict([(val, name.lower()) for (val, name) in logging._levelToName.items()] + [
  (60, "off"),
  (9, "debug1"),
  (8, "debug2"),
  (7, "debug3"),
  (6, "debug4"),
])
g_alog_name_to_level = dict([(n, l) for (l, n) in g_alog_level_to_name.items()])

## Core ########################################################################

g_alog_formatter = None

class _level_wrapper(object):
  def __init__(self, level):
    self.level = level
  def __call__(self, slf, msg, *args, **kwargs):
    slf.log(msg, *args, **kwargs)

def configure(default_level, filters="", formatter=AlogPrettyFormatter):

  # Set up the formatter if different type
  global g_alog_formatter
  if type(g_alog_formatter) != formatter:
    g_alog_formatter = formatter()

  # Set up root handler if not already set
  if not len(logging.root.handlers):
    handler = logging.StreamHandler()
    handler.setFormatter(g_alog_formatter)
    logging.root.addHandler(handler)
  root_handler = logging.root.handlers[0]

  # Add custom low levels
  for level, name in g_alog_level_to_name.items():
    if not hasattr(logging.Logger, name) and name != "off":
      logging.addLevelName(level, name.upper())
      setattr(logging.Logger, name, _level_wrapper(level))
      setattr(logging.Logger, name, _level_wrapper(level))
      # setattr(logging.Logger, name, lambda self, msg, *args, **kwargs: self.log(level, msg, *args, **kwargs))
      # setattr(logging, name, lambda msg, *args, **kwargs: logging.root.log(level, msg, *args, **kwargs))

  # Set default level
  default_level_val = g_alog_name_to_level.get(default_level, None)
  if default_level_val is not None:
    logging.root.setLevel(default_level_val)
    for handler in logging.root.handlers:
        handler.setLevel(default_level_val)
  else:
    logging.warning("Invalid default_level [%s]", default_level)

  # Add level filters by name
  if len(filters):
    for entry in filters.split(','):
      if len(entry):
        parts = entry.split(':')
        if len(parts) != 2:
          logging.warning("Invalid filter entry [%s]", entry)
        else:
          ch, level_name = parts
          level = g_alog_name_to_level.get(level_name, None)
          if level is None:
            logging.warning("Invalid level [%s] for channel [%s]", level_name, ch)
          else:
            handler = logging.StreamHandler()
            handler.setFormatter(g_alog_formatter)
            handler.setLevel(level)
            l = logging.getLogger(name)
            l.setLevel(level)
            l.propagate = False
            l.addHandler(handler)
      else:
        logging.warning("Invalid filter entry [%s]", entry)


## Testing #####################################################################

if __name__ == '__main__':
  import sys
  default_level = sys.argv[1] if len(sys.argv) > 1 else "info"
  filters = sys.argv[2] if len(sys.argv) > 2 else ""
  use_json = len(sys.argv) > 3
  formatter = AlogJsonFormatter if use_json else AlogPrettyFormatter
  configure(default_level=default_level, filters=filters, formatter=formatter)

  logging.info("TEST info")
  logging.getLogger("FOO").debug2("Debug2 line %d", 10)
