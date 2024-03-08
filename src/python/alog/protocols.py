import sys
from typing import TYPE_CHECKING, ClassVar, Mapping, Optional, Protocol

if TYPE_CHECKING:
    from logging import (
        Handler,
        Logger,
        LogRecord,
        Manager,
        RootLogger,
        _ArgsType,
        _ExcInfoType,
        _Level,
        _SysExcInfoType,
    )

    from typing_extensions import Self


class LoggerProtocol(Protocol):
    name: str  # undocumented
    level: int  # undocumented
    parent: Optional["Logger"]  # undocumented
    propagate: bool
    handlers: list["Handler"]  # undocumented
    disabled: bool  # undocumented
    root: ClassVar["RootLogger"]  # undocumented
    manager: "Manager"  # undocumented

    def __init__(self, name: str, level: "_Level" = 0) -> None:
        ...

    def setLevel(self, level: "_Level") -> None:
        ...

    def isEnabledFor(self, level: int) -> bool:
        ...

    def getEffectiveLevel(self) -> int:
        ...

    def getChild(self, suffix: str) -> "Self":
        ...  # see python/typing#980

    if sys.version_info >= (3, 12):

        def getChildren(self) -> set["Logger"]:
            ...

    def debug(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def info(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def warning(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def warn(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def error(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def exception(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = True,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def critical(
        self,
        msg: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

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
        ...

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
        ...  # undocumented

    fatal = critical

    def addHandler(self, hdlr: "Handler") -> None:
        ...

    def removeHandler(self, hdlr: "Handler") -> None:
        ...

    def findCaller(
        self, stack_info: bool = False, stacklevel: int = 1
    ) -> tuple[str, int, str, Optional[str]]:
        ...

    def handle(self, record: "LogRecord") -> None:
        ...

    def makeRecord(
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
    ) -> "LogRecord":
        ...

    def hasHandlers(self) -> bool:
        ...

    def callHandlers(self, record: "LogRecord") -> None:
        ...  # undocumented


class ALogLoggerProtocol(LoggerProtocol):
    def is_enabled(self, level: "_Level") -> bool:
        ...

    def off(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def trace(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def debug1(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def debug2(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def debug3(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...

    def debug4(
        self,
        arg_one: object,
        *args: object,
        exc_info: "_ExcInfoType" = None,
        stack_info: bool = False,
        stacklevel: int = 1,
        extra: Optional[Mapping[str, object]] = None,
    ) -> None:
        ...
