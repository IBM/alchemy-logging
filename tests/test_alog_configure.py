#*****************************************************************#
# (C) Copyright IBM Corporation 2020.                             #
#                                                                 #
# The source code for this program is not published or otherwise  #
# divested of its trade secrets, irrespective of what has been    #
# deposited with the U.S. Copyright Office.                       #
#*****************************************************************#
'''ALog configuration unit tests.
'''

import unittest

# Import the implementation details so that we can test them
import alog.alog as alog

class TestConfigure(unittest.TestCase):
    '''Ensure that the configure() function works as expected'''

    def test_configure_multi_formatter(self):
        '''Make sure that configure correctly removes all previously-configured
        handlers from the logging core.
        '''
        import logging
        logging.basicConfig()
        import alog
        alog.configure('info')
        self.assertEqual(len(logging.root.handlers), 1)
