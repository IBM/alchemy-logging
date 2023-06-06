################################################################################
# MIT License
#
# Copyright (c) 2021 IBM
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
################################################################################
'''ALog unit tests.
'''

# Standard
import inspect
import io
import json
import logging
import os
import pickle
import re
import sys
import threading
import time

# Third Party
import pytest

# Import the implementation details so that we can test them
import alog.alog as alog

## Helpers #####################################################################

test_code = "<TST93344011I>"

def pretty_level_to_name(pretty_level):
    for name, pretty_name in alog.AlogPrettyFormatter._LEVEL_MAP.items():
        if pretty_name == pretty_level:
            return pretty_name
    return None

def parse_pretty_line(line):
    timestamp_regex = r"([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{6})"
    rest_of_regex = r"\[([^:]*):([^\]:]*):?([0-9]*)\]( ?<[^\s]*>)? ([\s]*)([^\s].*)\n?"
    whole_regex = "^%s %s$" % (timestamp_regex, rest_of_regex)
    expr = re.compile(whole_regex)
    if isinstance(line, bytes):
        line = line.decode('utf-8')
    match = expr.match(line)
    assert match is not None, 'Failed to parse pretty line: [[%s]]' % line
    res = {
        'timestamp': match[1],
        'channel': match[2].strip(),
        'level': pretty_level_to_name(match[3]),
        'num_indent': len(match[6]) / len(alog.AlogPrettyFormatter._INDENT),
        'message': match[7],
    }
    if len(match[4]) > 0:
        res['thread_id'] = int(match[4])
    if match[5] is not None:
        res['log_code'] = match[5].strip()
    return res

def is_log_msg(msg):
    return not msg.startswith(alog.scope_start_str) and not msg.startswith(alog.scope_end_str)

class LogCaptureFormatter(alog.AlogFormatterBase):
    '''Helper that captures logs, then forwards them to a child
    '''

    def __init__(self, child_formatter):
        super().__init__()

        # Use the _setup_formatter to do all of the standard formatter setup,
        # then grab a handle to it
        alog._setup_formatter(child_formatter)
        self.formatter = alog.g_alog_formatter

        # We need to make sure that the inner formatter is sharing the same
        # indentation
        self.formatter._indent = self._indent

        # Keep track of the lines that have been captured
        self.captured = []

    def format(self, record):
        formatted = self.formatter.format(record)
        if isinstance(formatted, list):
            self.captured.extend(formatted)
        else:
            self.captured.extend(formatted.split('\n'))
        return formatted

    def get_json_records(self):
        return [json.loads(e) for e in self.captured]

    def get_pretty_records(self):
        return [parse_pretty_line(e) for e in self.captured]

## configure ###################################################################

def test_configure_default_level():
    '''Test that setting the default_level applies to multiple channels
    created both before and after the configure call
    '''
    ch1 = alog.use_channel('CH1')
    alog.configure(default_level='info')
    ch2 = alog.use_channel('CH2')
    assert ch1.isEnabled('info')
    assert not ch1.isEnabled('debug')
    assert ch2.isEnabled('info')
    assert not ch2.isEnabled('debug')

def test_configure_filters():
    '''Test that setting the level for a given channel via a filter after
    the channel has been created works
    '''
    ch1 = alog.use_channel('CH1')
    alog.configure(default_level='info', filters='CH1:error,CH2:debug')
    ch2 = alog.use_channel('CH2')
    assert not ch1.isEnabled('info')
    assert not ch1.isEnabled('debug')
    assert ch2.isEnabled('info')
    assert ch2.isEnabled('debug')

def test_configure_reconfigure():
    '''Test that re-invoking confgure changes the enablement as expected
    '''
    ch1 = alog.use_channel('CH1')
    alog.configure(default_level='info', filters='CH1:error')
    ch2 = alog.use_channel('CH2')

    assert not ch1.isEnabled('info')
    assert not ch1.isEnabled('debug')
    assert ch2.isEnabled('info')
    assert not ch2.isEnabled('debug')

    alog.configure(default_level='error', filters='CH1:info')

    assert ch1.isEnabled('info')
    assert not ch1.isEnabled('debug')
    assert not ch2.isEnabled('info')
    assert not ch2.isEnabled('debug')

def test_configure_configure_multi_formatter():
    '''Make sure that configure correctly removes all previously-configured
    handlers from the logging core.
    '''
    import logging
    logging.basicConfig()
    import alog
    alog.configure('info')
    assert len(logging.root.handlers) == 1

def test_configure_disable():
    '''Test that logging can be fully disabled
    '''
    capture_formatter = LogCaptureFormatter('pretty')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Make sure it is enabled now
    test_channel.info('test')
    assert len(capture_formatter.captured) == 1

    # Reconfigure to disable
    alog.configure(default_level='disable')
    test_channel.error('test')
    assert len(capture_formatter.captured) == 1

    # Reconfigure to re-enable
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel.error('test')
    assert len(capture_formatter.captured) == 2

def test_configure_custom_handler():
    '''Test that a custom handler can be given and that it works with both
    default levels and filtered levels
    '''

    stream = io.StringIO()
    alog.configure(
        default_level='info',
        filters='BAR:debug',
        handler_generator=lambda: logging.StreamHandler(stream))

    # Set up two channels (one filtered and one defaulted)
    foo = alog.use_channel('FOO')
    bar = alog.use_channel('BAR')

    # Log to the two channels to make sure that both behave as expected
    foo.info('Should show up')
    foo.debug('Should not show up')
    bar.info('Should show up')
    bar.debug('Should show up')

    all_lines = [l for l in stream.getvalue().split('\n') if l]
    assert len(all_lines) == 3

def test_configure_formatter_with_filename():
    '''Test that using logging to create a logger with a formatter that
    includes %(filename)s, %(lineno)s, and %(funcName)s does not point at
    alog.py.

    https://github.com/IBM/alchemy-logging/issues/152
    '''

    stream = io.StringIO()
    formatter = logging.Formatter("%(filename)s:%(lineno)s:%(funcName)s")
    handler = logging.StreamHandler(stream)
    handler.setLevel(logging.INFO)
    handler.setFormatter(formatter)
    logger = logging.getLogger("native-fname-test")
    logger.setLevel(logging.INFO)
    logger.addHandler(handler)

    # Grab this file, line, function
    frameinfo = inspect.getframeinfo(inspect.currentframe())
    logger.info("This is a test")
    expected_fname = os.path.basename(frameinfo.filename)
    expected_lineno = str(frameinfo.lineno + 1)
    expected_function = frameinfo.function

    logged_content = stream.getvalue()
    fname, lineno, func_name = logged_content.strip().split(":")
    assert fname == expected_fname
    assert lineno == expected_lineno
    assert func_name == expected_function

## json ########################################################################

def test_json_merge_msg_json():
    '''Tests that dict messages are merged when using json format'''

    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log a dict message
    test_channel.info(dict({'test_msg':1}))
    assert len(capture_formatter.captured) == 1
    logged_output = json.loads(capture_formatter.captured[0])
    assert isinstance(logged_output, dict)

    for key in logged_output.keys():
        # should have merged all dict's!
        assert not isinstance(logged_output[key], dict)
    # key should be present if the message was merged into top-level dict
    assert 'test_msg' in logged_output
    # value should be the same
    assert logged_output['test_msg'] == 1

def test_json_empty_msg_json():
    '''Tests that logs are in json format with an empty message'''

    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log an empty message
    test_channel.info('')

    # Validate that we get a dict back
    logged_output = capture_formatter.get_json_records()
    assert len(logged_output) == 1
    assert isinstance(logged_output[0], dict)

def test_json_json_level_formatting():
    '''Make sure that the key used for the level is 'level' and that the value is lowercase'''

    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log an empty message
    test_channel.info('test')

    # Validate that we get a dict back
    logged_output = capture_formatter.get_json_records()
    assert len(logged_output) == 1
    assert isinstance(logged_output[0], dict)
    assert 'level' in logged_output[0]
    assert logged_output[0]['level'].lower() == logged_output[0]['level']

def test_json_with_functions():
    '''Make sure that function arguments are formatted correctly with json'''

    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log a message with a function
    test_channel.info('Logging a function %s', is_log_msg)

    # Validate that we get a dict back
    logged_output = capture_formatter.get_json_records()
    assert len(logged_output) == 1
    assert isinstance(logged_output[0], dict)
    assert 'level' in logged_output[0]
    assert logged_output[0]['level'].lower() == logged_output[0]['level']

def test_log_code_json_function():
    '''Test that logging with a log code and the json formatter with a function works as expected.
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log with a code and some functions
    test_channel.info(test_code, "Logging 2 functions, %s and %s", is_log_msg, pretty_level_to_name)
    logged_output = capture_formatter.get_json_records()

    # Make sure the code and message came through as fields
    assert len(logged_output) == 1
    record = logged_output[0]
    assert 'log_code' in record
    assert record['log_code'] == test_code
    assert 'message' in record
    assert record['message'] == f"Logging 2 functions, {is_log_msg} and {pretty_level_to_name}"

## Custom Formatter ############################################################

def test_custom_formatter_pretty_with_args():
    '''Tests that a manually constructed AlogPrettyFormatter can be used'''
    alog.configure('info', '', formatter=alog.AlogPrettyFormatter(10))

## Thread Id ###################################################################

def test_thread_id_json():
    '''Test that the thread id is given with json formatting'''

    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', thread_id=True, formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log a sample message
    test_channel.info('This is a test')

    # Capture the output and make sure the thread id is present
    logged_output = capture_formatter.get_json_records()
    assert len(logged_output) == 1
    assert 'thread_id' in logged_output[0]

def test_thread_id_pretty():
    '''Test that the thread id is given with pretty formatting'''

    # Configure for log capture
    capture_formatter = LogCaptureFormatter('pretty')
    alog.configure(default_level='info', thread_id=True, formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log a sample message
    test_channel.info('This is a test')

    # Capture the output and make sure the thread id is present
    assert len(capture_formatter.captured), 1
    logged_output = capture_formatter.get_pretty_records()[0]
    assert 'thread_id' in logged_output

## Log Code ####################################################################

def test_log_code_dict():
    '''Test that logging a dict with a log code and message adds the code to
    the header as expected
    '''

    # Configure for log capture
    capture_formatter = LogCaptureFormatter('pretty')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log with a code
    test_channel.info({'log_code': test_code, 'message': 'This is a test'})
    test_channel.info({'log_code': '<>', 'message': 'https://url.com/a%20b'})
    logged_output = capture_formatter.get_pretty_records()

    # Make sure the code and message came through as fields
    assert len(logged_output) == 2
    record = logged_output[0]
    assert 'log_code' in record
    assert record['log_code'] == test_code
    assert 'message' in record
    assert record['message'] == 'This is a test'

    # Make sure the percent encoding in the message was preserved
    assert 'https://url.com/a%20b' == logged_output[1]['message']

def test_log_code_arg():
    '''Test that logging with the first argument as a log code adds the code
    to the header correctly
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('pretty')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log with a code
    test_channel.info(test_code, 'This is a test')
    logged_output = capture_formatter.get_pretty_records()

    # Make sure the code and message came through as fields
    assert len(logged_output) == 1
    record = logged_output[0]
    assert 'log_code' in record
    assert record['log_code'] == test_code
    assert 'message' in record
    assert record['message'] == 'This is a test'

def test_log_code_with_formatting():
    '''Test that logging with a log code and formatting arguments to the message.
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('pretty')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log with a code and a lazy formatting arg
    test_channel.info(test_code, 'This is a test %d', 1)
    logged_output = capture_formatter.get_pretty_records()

    # Make sure the code and message came through as fields
    assert len(logged_output) == 1
    record = logged_output[0]
    assert 'log_code' in record
    assert record['log_code'] == test_code
    assert 'message' in record
    assert record['message'] == 'This is a test 1'

def test_log_code_native_logging():
    '''Test that logging with the native logger works, despite overridden functions.
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('pretty')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    import logging
    logging.info('This is a test %d', 1)
    logged_output = capture_formatter.get_pretty_records()

    # Make sure the output was handled by the pretty formatter
    assert len(logged_output) == 1
    record = logged_output[0]
    assert 'log_code' not in record
    assert 'message' in record
    assert record['message'] == 'This is a test 1'

def test_log_code_json():
    '''Test that logging with a log code and the json formatter works as expected.
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log with a code
    test_channel.info({'log_code': test_code, 'message': 'This is a test'})
    logged_output = capture_formatter.get_json_records()

    # Make sure the code and message came through as fields
    assert len(logged_output) == 1
    record = logged_output[0]
    assert 'log_code' in record
    assert record['log_code'] == test_code
    assert 'message' in record
    assert record['message'] == 'This is a test'

def test_log_code_formatting_error_with_and_without_log_code(capsys):
    '''Test that a Logging Error is sent to stderr for invalid log interpolation
    with and without a log code argument
    '''
    alog.configure('info', formatter='pretty')
    test_channel = alog.use_channel('TEST')
    test_channel.info("<ABC12345I>", "HELLO %s", "WORLD", "FOO")
    test_channel.info("<ABC12345I>", "HELLO %s %s", "WORLD")
    test_channel.info("HELLO %s", "WORLD", "FOO")
    test_channel.info("HELLO %s %s", "WORLD")
    captured = str(capsys.readouterr().err)
    assert captured.count("--- Logging error ---") == 4


## Scoped Loggers ##############################################################

def test_scoped_logger_context_managed_scoping():
    '''Test that deindent happens when with statement goes out of scope.'''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log with a context timer
    with alog.ContextLog(test_channel.info, 'inner'):
       test_channel.info(test_code, 'This should be scoped')
    test_channel.info(test_code, 'This should not be scoped')
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 4
    # Parse out the two messages we explicitly logged. Only the first should be indented
    in_scope_log, out_scope_log = [line for line in logged_output if is_log_msg(line['message'])]
    assert in_scope_log['num_indent'] >= 1
    assert out_scope_log['num_indent'] == 0

def test_scoped_logger_direct_scoping():
    '''Test to make sure that log scoping works correctly by just calling the initializer
    and the finalizer directly.'''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    inner_scope = alog.ScopedLog(test_channel.info, 'inner')
    test_channel.info(test_code, 'This should be scoped')
    del inner_scope
    test_channel.info(test_code, 'This should not be scoped')
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 4
    # Parse out the two messages we explicitly logged. Only the first should be indented
    in_scope_log, out_scope_log = [line for line in logged_output if is_log_msg(line['message'])]
    assert in_scope_log['num_indent'] >= 1
    assert out_scope_log['num_indent'] == 0

def test_scoped_logger_direct_function_logger():
    '''Test to make sure that scoped function logger works.
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    def test():
        _ = alog.FunctionLog(test_channel.info, 'inner')
        test_channel.info(test_code, 'This should be scoped')
    test()
    test_channel.info(test_code, 'This should not be scoped')
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 4
    # Parse out the two messages we explicitly logged. Only the first should be indented
    in_scope_log, out_scope_log = [line for line in logged_output if is_log_msg(line['message'])]
    assert in_scope_log['num_indent'] >= 1
    assert out_scope_log['num_indent'] == 0

def test_scoped_logger_decorated_function_logger():
    '''Test to make sure that function logger works with decorators.
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    @alog.logged_function(test_channel.info, 'inner')
    def test():
        test_channel.info(test_code, 'This should be scoped')
    test()
    test_channel.info(test_code, 'This should not be scoped')
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 4
    # Parse out the two messages we explicitly logged. Only the first should be indented
    in_scope_log, out_scope_log = [line for line in logged_output if is_log_msg(line['message'])]
    assert in_scope_log['num_indent'] >= 1
    assert out_scope_log['num_indent'] == 0

def test_scoped_logger_disabled_scope_indentation():
    '''Test to make sure that scoped indentation is only applied if a given
    scope is enabled
    '''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Create a scope on a disabled level, but log on an enabled level inside
    with alog.ContextLog(test_channel.debug):
        test_channel.info('See me')

    logged_output = capture_formatter.get_json_records()
    assert len(logged_output) == 1

    # Make sure there was no identation on the record
    record = logged_output[0]
    assert record['num_indent'] == 0

def test_scoped_logger_context_additional_metadata():
    '''Test if additional metadata is passed and logged correctly
    and works only within scope with context logger'''
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    # Log with a context timer
    with alog.ContextLog(test_channel.info, 'inner', context_id="temp-id-1"):
       test_channel.info(test_code, 'This should be scoped')
    test_channel.info(test_code, 'This should not be scoped')
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 4
    # Parse out the two messages we explicitly logged. Only the first should be indented
    in_scope_log, out_scope_log = [line for line in logged_output if is_log_msg(line['message'])]
    assert "context_id" in in_scope_log and in_scope_log["context_id"] == "temp-id-1"
    assert "context_id" not in out_scope_log

## Timed Loggers ###############################################################

def test_timed_logger_context_managed_timer():
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    with alog.ContextTimer(test_channel.info, 'timed: '):
       test_channel.info(test_code, 'Test message.')
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 2

    # Parse out the two messages we explicitly logged. Only the first should be indented
    test_log, timed_log = [line for line in logged_output if is_log_msg(line['message'])]

    # ensure timer outputs a timedelta
    timed_message = timed_log['message']
    assert timed_message.startswith('timed: 0:')
    assert re.match(r'^timed: [0-9]:[0-9][0-9]:[0-9][0-9]\.[0-9]+$', timed_message)

def test_timed_logger_scoped_timer():
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    def test():
        _ = alog.ScopedTimer(test_channel.info, 'timed: ')
        test_channel.info(test_code, 'Test message.')
    test()
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 2

    # Parse out the two messages we explicitly logged. Only the first should be indented
    test_log, timed_log = [line for line in logged_output if is_log_msg(line['message'])]

    # ensure timer outputs a timedelta
    timed_message = timed_log['message']
    assert timed_message.startswith('timed: 0:')
    assert re.match(r'^timed: [0-9]:[0-9][0-9]:[0-9][0-9]\.[0-9]+$', timed_message)

def test_timed_logger_decorated_timer():
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    @alog.timed_function(test_channel.info, 'timed: ')
    def test():
        test_channel.info(test_code, 'Test message.')
    test()
    logged_output = capture_formatter.get_json_records()

    # Checks to see if a log message is a scope messsage (starts with BEGIN/END) or a "normal" log
    assert len(logged_output) == 2

    # Parse out the two messages we explicitly logged. Only the first should be indented
    test_log, timed_log = [line for line in logged_output if is_log_msg(line['message'])]

    # ensure timer outputs a timedelta
    timed_message = timed_log['message']
    assert timed_message.startswith('timed: 0:')
    assert re.match(r'^timed: [0-9]:[0-9][0-9]:[0-9][0-9]\.[0-9]+$', timed_message)

## IsEnabled ###################################################################

def test_is_enabled_for_true():
    '''Tests when a level is enabled, it returns true'''
    alog.configure('info')
    ch = alog.use_channel('TEST')
    assert ch.isEnabled('info')
    assert ch.isEnabled('warning')

def test_is_enabled_for_false():
    '''Tests when a level is disabled, it returns true'''
    alog.configure('info')
    ch = alog.use_channel('TEST')
    assert not ch.isEnabled('trace')
    assert not ch.isEnabled('debug2')

def test_is_enabled_for_off():
    '''Tests when a channel is fully off, it always returns false'''
    alog.configure('off')
    ch = alog.use_channel('TEST')
    assert not ch.isEnabled('error')
    assert not ch.isEnabled('trace')
    assert not ch.isEnabled('debug2')

def test_is_enabled_for_filters():
    '''Tests that different channels on different levels respond correctly
    '''
    alog.configure('warning', 'MAIN:debug')
    ch1 = alog.use_channel('TEST')
    ch2 = alog.use_channel('MAIN')

    assert ch1.isEnabled('error')
    assert ch2.isEnabled('error')

    assert not ch1.isEnabled('info')
    assert ch2.isEnabled('info')

    assert not ch1.isEnabled('debug2')
    assert not ch2.isEnabled('debug2')

def test_is_enabled_for_numeric_values():
    '''Tests that isEnabled works with the numeric level values'''
    alog.configure('info')
    ch = alog.use_channel('TEST')
    assert not ch.isEnabled(alog.g_alog_name_to_level['trace'])
    assert not ch.isEnabled(alog.g_alog_name_to_level['debug2'])

## Threading ###################################################################

def test_thread_local_indent():
    '''Make sure that indent counts are kept on a per-thread basis'''
    capture_formatter = LogCaptureFormatter(alog.AlogJsonFormatter())
    alog.configure('info', thread_id=True, formatter=capture_formatter)

    # Make a small function that does some logging with indentation and some
    # small sleeps in between to encourage thread swapping
    ch = alog.use_channel('TEST')
    def doit():
        ch.info('Indent 0')
        with alog.ContextLog(ch.info, 'scope 1'):
            ch.info('Indent 1')
            time.sleep(0.001)
            with alog.ContextLog(ch.info, 'scope 2'):
                ch.info('Indent 2')
                time.sleep(0.001)
            ch.info('Indent 1 (number two)')
            time.sleep(0.001)
        ch.info('Indent 0 (number two)')

    # Create two threads that each execute it
    th1 = threading.Thread(target=doit)
    th2 = threading.Thread(target=doit)

    # Run them in parallel
    th1.start()
    th2.start()
    th1.join()
    th2.join()

    # Make sure that the lines were captured correctly
    entries = capture_formatter.captured
    assert len(entries) == 18

    # Sort the lines by thread ID
    entries_by_thread = {}
    for entry in entries:
        entry = json.loads(entry)
        entries_by_thread.setdefault(entry['thread_id'], []).append(entry)
    thread_entries = list(entries_by_thread.values())
    assert len(thread_entries) == 2

    # Make sure that the sequence of indentations for each thread lines up
    thread0_indents = [e['num_indent'] for e in thread_entries[0]]
    thread1_indents = [e['num_indent'] for e in thread_entries[1]]
    per_thread_indents = list(zip(thread0_indents, thread1_indents))
    assert all([a == b for a, b in per_thread_indents])

    # Make sure all expected indentation levels are present
    assert 0 in thread0_indents
    assert 1 in thread0_indents
    assert 2 in thread0_indents

## ExcInfo #####################################################################

def test_exc_info_json():
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('json')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    try:
        raise ValueError('throw it')
    except ValueError:
        test_channel.info('caught it!', exc_info=True)
        test_channel.info('nothing to see here', exc_info=False)
    logged_output = capture_formatter.get_json_records()

    # Make sure a two lines logged with 'exception' populated in the first
    # but not the second
    assert len(logged_output) == 2
    assert logged_output[0]['exception'] is not None
    assert logged_output[1]['exception'] is None

def test_exc_info_pretty():
    # Configure for log capture
    capture_formatter = LogCaptureFormatter('pretty')
    alog.configure(default_level='info', formatter=capture_formatter)
    test_channel = alog.use_channel('TEST')

    try:
        raise ValueError('throw it')
    except ValueError:
        test_channel.info('caught it!', exc_info=True)
        test_channel.info('nothing to see here', exc_info=False)
    logged_output = capture_formatter.get_pretty_records()

    # Six lines: One message, four stack trace, one message
    assert len(logged_output) == 6

## Pickling ####################################################################


def test_pickle_custom_formatter():
    '''Make sure an instance of AlogPrettyFormatter can be pickled. This is
    important for sending log configuration between processes when using
    multiprocessing and 'spawn'.
    '''
    fmt = alog.AlogPrettyFormatter(12)
    dumped = pickle.dumps(fmt)
    fmt2 = pickle.loads(dumped)
    assert fmt.channel_len == fmt2.channel_len

## Lazy Interpolation ##########################################################

class StringifyCapturer:
    '''Small dummy class that will keep track of if its __str__ method are
    called
    '''
    def __init__(self):
        self.called_str = False

    def __str__(self):
        self.called_str = True
        return 'StringifyCapturer'

def test_lazy_interp_enabled():
    '''Test that a log line made on an enabled channel/level does call
    __str__
    '''

    alog.configure(default_level='info')
    test_channel = alog.use_channel('TEST')

    capturer = StringifyCapturer()
    test_channel.info('%s', capturer)
    assert capturer.called_str

def test_lazy_interp_disabled_without_log_code():
    '''Test that a log line made on a disabled channel/level without a log
    code does not call __str__
    '''

    alog.configure(default_level='info')
    test_channel = alog.use_channel('TEST')

    capturer = StringifyCapturer()
    test_channel.debug('%s', capturer)
    assert not capturer.called_str

def test_lazy_interp_disabled_with_log_code():
    '''Test that a log line made on a disabled channel/level with a log
    code does not call __str__
    '''

    alog.configure(default_level='info')
    test_channel = alog.use_channel('TEST')

    capturer = StringifyCapturer()
    test_channel.debug('<FOO12345123D>', '%s', capturer)
    assert not capturer.called_str

def test_lazy_interp_disabled_by_filter_with_log_code():
    '''Test that a log line made on a channel/level that is disabled by a
    filter does not call __str__
    '''

    alog.configure(default_level='info', filters='TEST:error')
    test_channel = alog.use_channel('TEST')

    capturer = StringifyCapturer()
    test_channel.info('<FOO52345123D>', '%s', capturer)
    assert not capturer.called_str

## One Off Bugs ################################################################

def test_bug_uvicorn_access_log(capsys):
    '''This test emulates the behavior of uvicorn's access logger which makes
    assumptions about the shape of the arguments passed through.

    REF: https://github.com/encode/uvicorn/blob/0.17.6/uvicorn/logging.py#L97
    BUG: https://github.com/IBM/alchemy-logging/issues/183
    '''
    class UvicornFormatter:
        @staticmethod
        def format(record):
            # This should raise if interpolation is done ahead of time
            (arg_one, arg_two) = record.args
            return "made it"

    logger = logging.getLogger("fake.uvicorn.access")
    handler = logging.StreamHandler()
    handler.setFormatter(UvicornFormatter())
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)
    logger.info("One %s, Two %s", 1, 2)
    captured = str(capsys.readouterr().err)
    assert "--- Logging error ---" not in captured
