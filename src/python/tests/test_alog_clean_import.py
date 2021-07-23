# Import the implementation details so that we can test them
import alog

def test_log_before_configure():
    """Make sure that no exception is thrown if a high-order log function is
    called before calling configure()
    """
    ch = alog.use_channel("TEST")
    ch.debug2("No throw")
