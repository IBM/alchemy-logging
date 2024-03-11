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
"""Protocols to assist IDEs with code completion for the additional methods which
Alchemy Logger overrides on the logging.Logger instances.
"""

import sys
from logging import Filter, LogRecord
from typing import (
    TYPE_CHECKING,
    Callable,
    ClassVar,
    List,
    Mapping,
    Optional,
    Set,
    Tuple,
    Union,
)

from .alog import _Level

if sys.version_info < (3, 8):
    from typing_extensions import Protocol
else:
    from typing import Protocol

if TYPE_CHECKING:
    from logging import (
        Handler,
        Logger,
        Manager,
        RootLogger,
        _ArgsType,
        _ExcInfoType,
        _SysExcInfoType,
    )

    from typing_extensions import Self


class _SupportsFilter(Protocol):
    def filter(self, __record: LogRecord) -> bool: ...


_FilterType = Union[Filter, Callable[[LogRecord], bool], _SupportsFilter]


class FilterProtocol(Protocol):
    """
    A base class for loggers and handlers which allows them to share
    common code.
    """

    filters: List[_FilterType]

    def __init__(self) -> None:
        """
        Initialize the list of filters to be an empty list.
        """

    def addFilter(self, filter: _FilterType) -> None:  # pylint: disable=invalid-name
        """
        Add the specified filter to this handler.
        """

    def removeFilter(self, filter: _FilterType) -> None:  # pylint: disable=invalid-name
        """
        Remove the specified filter from this handler.
        """

    def filter(self, record: LogRecord) -> bool:  # type: ignore
        """
        Determine if a record is loggable by consulting all the filters.

        The default is to allow the record to be logged; any filter can veto
        this and the record is then dropped. Returns a zero value if a record
        is to be dropped, else non-zero.

        .. versionchanged:: 3.2

           Allow filters to be just callables.
        """

    if sys.version_info >= (3, 12):

        def filter(self, record: LogRecord) -> Union[bool, LogRecord]:  # type: ignore pylint: disable=function-redefined
            """
            Determine if a record is loggable by consulting all the filters.

            The default is to allow the record to be logged; any filter can veto
            this by returning a false value.
            If a filter attached to a handler returns a log record instance,
            then that instance is used in place of the original log record in
            any further processing of the event by that handler.
            If a filter returns any other true value, the original log record
            is used in any further processing of the event by that handler.

            If none of the filters return false values, this method returns
            a log record.
            If any of the filters return a false value, this method returns
            a false value.

            .. versionchanged:: 3.2

               Allow filters to be just callables.

            .. versionchanged:: 3.12
               Allow filters to return a LogRecord instead of
               modifying it in place.
            """


class LoggerProtocol(FilterProtocol):
    """
    A Protocol reflecting the public methods, type hints and docstrings of the stdlib
    `logging.Logger`
    """

    name: str  # undocumented
    level: int  # undocumented
    parent: Optional["Logger"]  # undocumented
    propagate: bool
    handlers: List["Handler"]  # undocumented
    disabled: bool  # undocumented
    root: ClassVar["RootLogger"]  # undocumented
    manager: "Manager"  # undocumented

    def __init__(self, name: str, level: "_Level" = 0) -> None:
        """
        Initialize the logger with a name and an optional level.
        """

    def setLevel(self, level: "_Level") -> None:  # pylint: disable=invalid-name
        """
        Set the logging level of this logger.  level must be an int or a str.
        """

    def isEnabledFor(self, level: int) -> bool:  # type: ignore pylint: disable=invalid-name
        """
        Is this logger enabled for level 'level'?
        """

    def getEffectiveLevel(self) -> int:  # type: ignore pylint: disable=invalid-name
        """
        Get the effective level for this logger.

        Loop through this logger and its parents in the logger hierarchy,
        looking for a non-zero logging level. Return the first one found.
        """

    def getChild(self, suffix: str) -> "Self":  # type: ignore pylint: disable=invalid-name
        """
        Get a logger which is a descendant to this one.

        This is a convenience method, such that

        logging.getLogger('abc').getChild('def.ghi')

        is the same as

        logging.getLogger('abc.def.ghi')

        It's useful, for example, when the parent logger is named using
        __name__ rather than a literal string.
        """
        # see python/typing#980

    if sys.version_info >= (3, 12):

        def getChildren(self) -> Set["Logger"]:  # pylint: disable=invalid-name
            ...

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def debug(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'DEBUG'.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.debug("Houston, we have a %s", "thorny problem", exc_info=1)
        """

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def info(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'INFO'.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.info("Houston, we have a %s", "interesting problem", exc_info=1)
        """

    if sys.version_info >= (3, 12):
        info.__doc__ = """
        Log 'msg % args' with severity 'INFO'.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.info("Houston, we have a %s", "notable problem", exc_info=1)
        """

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def warning(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'WARNING'.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.warning("Houston, we have a %s", "bit of a problem", exc_info=1)
        """

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def warn(  # pylint: disable=invalid-name disable=missing-docstring
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None: ...

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def error(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'ERROR'.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.error("Houston, we have a %s", "major problem", exc_info=1)
        """

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def exception(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = True,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Convenience method for logging an ERROR with exception information.
        """

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def critical(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'CRITICAL'.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.critical("Houston, we have a %s", "major disaster", exc_info=1)
        """

    # Aligned with the typeshed type definition:
    # `*kwargs` replaced with: `exc_info`, `stack_info`, `stacklevel`, `extra`
    def log(
        self,
        level: int,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with the integer severity 'level'.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.log(level, "We have a %s", "mysterious problem", exc_info=1)
        """

    def _log(
        self,
        level: int,
        msg: object,
        args: "_ArgsType",
        exc_info: Optional["_ExcInfoType"] = None,
        extra: Optional[Mapping[str, object]] = None,
        stack_info: bool = False,
        stacklevel: int = 1,
    ) -> None:
        """
        Low-level logging routine which creates a LogRecord and then calls
        all the handlers of this logger to handle the record.
        """

    fatal = critical
    if sys.version_info >= (3, 10):

        def fatal(
            self,
            msg: object,
            *args: object,
            exc_info: "_ExcInfoType" = None,
            stack_info: bool = False,
            stacklevel: int = 1,
            extra: Optional[Mapping[str, object]] = None,
        ) -> None:
            """
            Don't use this method, use critical() instead.
            """

    def addHandler(self, hdlr: "Handler") -> None:  # pylint: disable=invalid-name
        """
        Add the specified handler to this logger.
        """

    def removeHandler(self, hdlr: "Handler") -> None:  # pylint: disable=invalid-name
        """
        Remove the specified handler from this logger.
        """

    def findCaller(  # pylint: disable=invalid-name
        self, stack_info: bool = False, stacklevel: int = 1
    ) -> Tuple[str, int, str, Optional[str]]:  # type: ignore
        """
        Find the stack frame of the caller so that we can note the source
        file name, line number and function name.
        """

    def handle(self, record: "LogRecord") -> None:
        """
        Call the handlers for the specified record.

        This method is used for unpickled records received from a socket, as
        well as those created locally. Logger-level filtering is applied.
        """

    def makeRecord(  # pylint: disable=invalid-name
        self,
        name: str,
        level: int,
        fn: str,
        lno: int,
        msg: object,
        args: "_ArgsType",
        exc_info: Optional["_SysExcInfoType"],
        func: Optional[str] = None,
        extra: Optional[Mapping[str, object]] = None,
        sinfo: Optional[str] = None,
    ) -> "LogRecord":  # type: ignore
        """
        A factory method which can be overridden in subclasses to create
        specialized LogRecords.
        """

    def hasHandlers(self) -> bool:  # type: ignore pylint: disable=invalid-name
        """
        See if this logger has any handlers configured.

        Loop through all handlers for this logger and its parents in the
        logger hierarchy. Return True if a handler was found, else False.
        Stop searching up the hierarchy whenever a logger with the "propagate"
        attribute set to zero is found - that will be the last logger which
        is checked for the existence of handlers.
        """

    def callHandlers(self, record: "LogRecord") -> None:  # pylint: disable=invalid-name
        """
        Pass a record to all relevant handlers.

        Loop through all handlers for this logger and its parents in the
        logger hierarchy. If no handler was found, output a one-off error
        message to sys.stderr. Stop searching up the hierarchy whenever a
        logger with the "propagate" attribute set to zero is found - that
        will be the last logger whose handlers are called.
        """


class ALogLoggerProtocol(LoggerProtocol):
    """
    A Protocol which describes the alchemy logging methods which it adds onto the
    stdlib `logging.Logger` instances.
    """

    def isEnabled(self, level: "_Level") -> bool:  # type: ignore pylint: disable=invalid-name
        """
        Is this logger enabled for level 'level'?

        Also supports referencing the level by name.
        """

    def trace(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'TRCE' (an alchemy Logging custom level).

        Used to log begin/end of functions for debugging code paths.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.trace("Houston, we have a %s", "major disaster", exc_info=1)
        """

    def debug1(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'DBG1' (an alchemy Logging custom level).

        High-level debugging statements.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.debug1("Houston, we have a %s", "thorny problem", exc_info=1)
        """

    def debug2(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'DBG2' (an alchemy Logging custom level).

        Mid-level debugging statements such as computed values.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.debug2("Houston, we have a %s", "thorny problem", exc_info=1)
        """

    def debug3(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'DBG3' (an alchemy Logging custom level).

        Low-level debugging statements such as computed values inside loops.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.debug3("Houston, we have a %s", "thorny problem", exc_info=1)
        """

    def debug4(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        """
        Log 'msg % args' with severity 'DBG4' (an alchemy Logging custom level).

        Ultra-low-level debugging statements such as data dumps and/or statements
        inside multiple nested loops.

        To pass exception information, use the keyword argument exc_info with
        a true value, e.g.

        logger.debug4("Houston, we have a %s", "thorny problem", exc_info=1)
        """
