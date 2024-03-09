# Alog overrides methods on `logging.Logger`.
# Pytest alters the order of imports, resulting in `alog` overriding information prior to
# us being able to make a copy of it.
#

# conftest.py is read before individual tests (to allow for fixtures to be shared across files).
# We make use of this priority to give us a chance to capture the original method names,
# docstrings and method parameters of stdlib `logging.Logger` prior to the import of `alog`
# which overrides them
import inspect
from logging import Logger
from typing import List


def get_parameter_names(obj: object, method_name: str) -> List[str]:
    """
    Get the parameter names for an object's method
    """
    signature = inspect.signature(getattr(obj, method_name))
    return list(signature.parameters)


LOGGER_FUNCTIONS = {fn[0] for fn in inspect.getmembers(Logger, inspect.isfunction)}
LOGGER_DOCSTRINGS = {fn: inspect.getdoc(getattr(Logger, fn)) for fn in LOGGER_FUNCTIONS}
LOGGER_PARAMETERS = {fn: get_parameter_names(Logger, fn) for fn in LOGGER_FUNCTIONS}
