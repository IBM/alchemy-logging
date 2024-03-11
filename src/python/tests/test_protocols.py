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
"""ALog unit tests to confirm that the protocol is aligned `logging.Logger`."""

# Standard
from typing import List
import importlib
import inspect
import sys

# Third Party
import pytest

# Local
from alog.protocols import LoggerProtocol


def get_parameter_names(obj: object, method_name: str) -> List[str]:
    """
    Get the parameter names for an object's method
    """
    signature = inspect.signature(getattr(obj, method_name))
    return list(signature.parameters)


def _populate_constants():
    """
    Idempotent function to populate the "plain vanilla" logging constants
    without the additions from alog
    """
    real_logging = sys.modules.pop("logging")
    local_logging = importlib.import_module("logging")
    LOGGER_FUNCTIONS = {
        fn[0] for fn in inspect.getmembers(local_logging.Logger, inspect.isfunction)
    }
    LOGGER_DOCSTRINGS = {
        fn: inspect.getdoc(getattr(local_logging.Logger, fn)) for fn in LOGGER_FUNCTIONS
    }
    LOGGER_PARAMETERS = {
        fn: get_parameter_names(local_logging.Logger, fn) for fn in LOGGER_FUNCTIONS
    }

    sys.modules["logging"] = real_logging

    return (LOGGER_FUNCTIONS, LOGGER_DOCSTRINGS, LOGGER_PARAMETERS)


LOGGER_FUNCTIONS, LOGGER_DOCSTRINGS, LOGGER_PARAMETERS = _populate_constants()


PROTOCOL_FUNCTIONS = {
    fn[0] for fn in inspect.getmembers(LoggerProtocol, inspect.isfunction)
}
PROTOCOL_DOCSTRINGS = {
    fn: inspect.getdoc(getattr(LoggerProtocol, fn)) for fn in LOGGER_FUNCTIONS
}
PROTOCOL_PARAMETERS = {
    fn: get_parameter_names(LoggerProtocol, fn) for fn in LOGGER_FUNCTIONS
}


def test_protocol_is_complete():
    """
    Test that all functions in stdlib `Logger` exists in the protocol `LoggerProtocol`
    """
    excluded_functions = {"__reduce__", "__repr__"}

    fn_not_in_protocol = LOGGER_FUNCTIONS.difference(PROTOCOL_FUNCTIONS)
    fn_not_in_protocol = fn_not_in_protocol.difference(excluded_functions)

    assert not fn_not_in_protocol, "All Logger functions should exist in the protocol"


def test_protocol_docstrings():
    """
    Test that all docstrings in `LoggerProtocol` matches those of the stdlib `Logger`
    """
    assert PROTOCOL_DOCSTRINGS == LOGGER_DOCSTRINGS


def test_protocol_function_signatures():
    """
    Test that all function parameter names in `LoggerProtocol` matches those of the
    stdlib `Logger`
    """
    # Adjust LOGGER_PARAMETERS for expected differences.
    # These are aligned with the typeshed type definition where `*kwargs` is replaced
    # with `exc_info`, `stack_info`, `stacklevel`, `extra`
    param_list = {
        "log",
        "debug",
        "info",
        "warn",
        "warning",
        "error",
        "critical",
        "fatal",
    }
    adjusted_logger_parameters = {}
    for param in LOGGER_PARAMETERS:
        params = [p for p in LOGGER_PARAMETERS[param]]
        if param in param_list:
            params.remove("kwargs")
            params.extend(["exc_info", "stack_info", "stacklevel", "extra"])
        elif param == "exception":
            params.remove("kwargs")
            params.extend(["stack_info", "stacklevel", "extra"])
        adjusted_logger_parameters[param] = params

    # Compare parameters against adjusted Logger parameters
    assert PROTOCOL_PARAMETERS == adjusted_logger_parameters


# Type annotations/hints aren't tested since those are included in the stdlib `Logger`
