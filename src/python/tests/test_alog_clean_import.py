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

# Standard
import logging
import io
import re

# Local
import alog

def test_log_before_configure():
    """Make sure that no exception is thrown if a high-order log function is
    called before calling configure()
    """
    ch = alog.use_channel("TEST")
    ch.debug2("No throw")


def test_default_log_configuration_works():
    '''We need to run this in a process that has a clean set of imports so that
    we can evaluate the import behavior. Using importlib.reload in the main
    process does not actually rerun the import-time code in alog.
    '''
    # Set up some standard logging with custom formatting
    log_stream = io.StringIO()
    stream_handler = logging.StreamHandler(stream=log_stream)
    fmt = '%(asctime)s [%(levelname)s] %(message)s'
    logging.basicConfig(
        level=logging.INFO,
        format=fmt,
        handlers=[stream_handler],
    )

    # NOTE: This really sucks, but it seems to be impossible to get pytest to
    #   not pre-configure the logging package, so we have to override the config
    #   for the child logger explicitly. This is NOT necessary when running the
    #   content of this test in a plain vanilla python session, but since pytest
    #   does some configuration of its own before the test starts, the above
    #   basicConfig call doesn't override that configuration.
    logger = logging.getLogger('mychan')
    logger.setLevel(logging.INFO)
    logger.addHandler(stream_handler)
    stream_handler.formatter = logging.Formatter(fmt)

    # Make sure debug2 works
    logger.debug2('Yep')
    assert log_stream.getvalue() == ''

    # Log to a standard enabled level and make sure the formatting is right
    # 2021-11-18 15:16:07,468 [INFO] test123
    logger.info('message')
    log_lines = list(filter(lambda x: x, log_stream.getvalue().split('\n')))
    assert len(log_lines) == 1
    assert re.match(r'\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d,\d\d\d \[INFO\] message', log_lines[0])
