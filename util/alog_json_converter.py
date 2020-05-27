#!/usr/bin/env python

from datetime import datetime
import traceback
import sys
import json

import alog
import munch

log_level = sys.argv[1] if len(sys.argv) > 1 else 'debug4'
log_filters = sys.argv[2] if len(sys.argv) > 2 else ''
alog.configure(log_level, log_filters)

try:
    for line in sys.stdin:
        try:
            js = json.loads(line)

            # Only print it if enabled
            chan = alog.use_channel(js['channel'])
            if chan.isEnabledFor(alog.alog.g_alog_name_to_level[js['level_str']]):

                # Parse the timestamp
                timestamp = datetime.strptime(js['timestamp'], '%Y-%m-%dT%H:%M:%S.%f')

                # Create a munch to simulate a log record
                m = munch.DefaultMunch(None, js)

                # Add in the internal names of the fileds
                m.created = timestamp.timestamp()
                m.msg = js
                m.name = m.channel
                m.levelname = m.level_str

                # Set the formatter's indentation
                # NOTE: Handle per-thread indent implementation change
                if hasattr(alog.alog.AlogFormatterBase, 'ThreadLocalIndent'):
                    alog.alog.g_alog_formatter._indent.indent = m.num_indent
                else:
                    alog.alog.g_alog_formatter._indent = m.num_indent

                # Remove all entries that are not needed anymore
                for k in ['timestamp', 'channel', 'level_str', 'num_indent']:
                    del js[k]

                # Remove any keys whose value is None
                null_keys = []
                for k, v in js.items():
                    if v is None:
                        null_keys.append(k)
                for k in null_keys:
                    del js[k]

                # Log it using the log formatter
                print(alog.alog.g_alog_formatter.format(m))
                sys.stdout.flush()

        # Handle non-json lines gracefully
        except ValueError:
            print("(*) {}".format(line.strip()))

        # Handle bugs in the script
        except Exception:
            traceback.print_exc()

except KeyboardInterrupt:
    sys.stdout.flush()
    pass
