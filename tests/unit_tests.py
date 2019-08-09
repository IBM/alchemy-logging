# *****************************************************************
#
# Licensed Materials - Property of IBM
#
# (C) Copyright IBM Corp. 2018. All Rights Reserved.
#
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
#
# *****************************************************************

import os
import sys
import json
import unittest

# Put the local module at the beginning of the path in case there's an installed
# copy on the system
local_module = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', 'alog'))
sys.path = [local_module] + sys.path
import alog


class TestJsonCompatibility(unittest.TestCase):
    '''Ensures that printed messages are valid json format when json formatting is specified'''

    def test_merge_msg_json(self):
        '''Tests that dict messages are merged when using json format. May be too complicated...'''
        # will run from terminal -- don't know how to save output of logging to variable
        commands_to_run = "python3 -c \"import alog;"
        commands_to_run += " alog.configure(default_level=\'info\', filters=\'\',"
        commands_to_run += " formatter=\'json\');"
        commands_to_run += " test_channel = alog.use_channel(\'test_merge_msg_json\');"
        commands_to_run += " test_channel.info(dict({\'test_msg\':1}))\" &> json_merge_test.txt"

        # run in terminal
        os.system(commands_to_run)

        # read results and try to convert to json
        with open('json_merge_test.txt', 'r') as test_file:
            logged_output = test_file.read().strip()
        try:
            logged_output = json.loads(logged_output)
        except json.decoder.JSONDecodeError:
            raise AssertionError("test_merge_msg_json unit test: FAILED. Logged output is not " \
                                 + "valid json. The output is:\n{0}".format(logged_output))
        self.assertIsNotNone(logged_output)
        self.assertIsInstance(logged_output, dict)

        for key in logged_output.keys():
            # should have merged all dict's!
            self.assertNotIsInstance(logged_output[key], dict)
        # key should be present if the message was merged into top-level dict
        self.assertIn('test_msg', logged_output)
        # value should be the same
        self.assertEqual(logged_output['test_msg'], 1)
        # don't need test file
        if os.path.isfile('json_merge_test.txt'):
            os.remove('json_merge_test.txt')

    def test_empty_msg_json(self):
        '''Tests that logs are in json format with an empty message. May be too complicated...'''
        # will run from terminal -- don't know how to save output of logging to variable
        commands_to_run = "python3 -c \"import alog;"
        commands_to_run += " alog.configure(default_level=\'info\', filters=\'\',"
        commands_to_run += " formatter=\'json\');"
        commands_to_run += " test_channel = alog.use_channel(\'test_empty_msg_json\');"
        commands_to_run += " test_channel.info(\'\')\" &> json_alog_test.txt"

        # run in terminal
        os.system(commands_to_run)

        # read results and try to convert to json
        with open('json_alog_test.txt', 'r') as test_file:
            logged_output = test_file.read().strip()
        self.assertIsNotNone(logged_output)
        try:
            logged_output = json.loads(logged_output)
        except json.decoder.JSONDecodeError:
            raise AssertionError("test_empty_msg_json unit test: FAILED. Logged output is not " \
                                 + "valid json. The output is:\n{0}".format(logged_output))

        self.assertIsInstance(logged_output, dict)
        # don't need test file
        if os.path.isfile('json_alog_test.txt'):
            os.remove('json_alog_test.txt')

class TestCustomFormatter(unittest.TestCase):

    def test_pretty_with_args(self):
        '''Tests that a manually constructed AlogPrettyFormatter can be used'''
        alog.configure('info', '', formatter=alog.AlogPrettyFormatter(10))

if __name__ == "__main__":
    # has verbose output of tests; otherwise just says all passed or not
    unittest.main(verbosity=2)
