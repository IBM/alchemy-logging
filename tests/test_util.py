import os
import pytest
import unittest

from util import correct_log_codes

def get_copy_file_name(filename):
    # Get the of an output file (if changes found) in copy mode
    return '{}_copy.py'.format(os.path.splitext(filename)[0])

def get_code_match(line):
    # Return search result for our main log code regex
    return correct_log_codes.LOG_PATTERN.search(line)

class TestLogCorrection(unittest.TestCase):
    '''Ensures that printed messages are valid json format when json formatting is specified'''
    fixtures_dir = os.path.join(os.path.dirname(__file__), 'fixtures')
    duplicate_file_dir = os.path.join(fixtures_dir, 'duplicate_replacement')
    duplicates_file = os.path.join(duplicate_file_dir, 'test_case_1.py')
    no_duplicates_file = os.path.join(duplicate_file_dir, 'test_case_2.py')

    placholders_file_dir = os.path.join(fixtures_dir, 'placeholders')
    prefix_file = os.path.join(placholders_file_dir, 'test_case_1.py')
    prefixless_file = os.path.join(placholders_file_dir, 'test_case_2.py')
    prefix = '<REP'

    @pytest.fixture(scope="session", autouse=True)
    def run_log_code_corrector(self, request):
        if not os.path.isdir(self.fixtures_dir):
            raise IOError('ERROR: Fixtures directory: [{}] does not exist'.format(self.fixtures_dir))
        # Run the log code correction script over the whole fixteres directory with copy on
        correct_log_codes.validate_or_correct_log_codes(
            root_dir=self.fixtures_dir,
            prefix=self.prefix,
            validate_only=False,
            copy=True)

    def test_log_code_regex_positive_cases(self):
        '''Example positive cases for main log code regex'''
        # Example valid log codes with different prefixes and levels
        self.assertTrue(get_code_match('<FOO12345678E>') is not None)
        self.assertTrue(get_code_match('<BAR12444478W>') is not None)
        self.assertTrue(get_code_match('<BAZ88845678F>') is not None)
        self.assertTrue(get_code_match('<BAT14245678I>') is not None)
        self.assertTrue(get_code_match('<LCT14288878D>') is not None)

    def test_log_code_regex_negative_cases(self):
        '''Example negative cases for main log code regex'''
        # Numbers only
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<12345678>') is None)
        # No prefix
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<12345678E>') is None)
        # Prefix doesn't have enough letters
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<LC12345678E>') is None)
        # Prefix has too many letters
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<LCTT12345678E>') is None)
        # Not enough digits
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<LCT1234678E>') is None)
        # Too many digits
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<LCT12346242478E>') is None)
        # Lowercase letters in prefix
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<lct12345678E>') is None)
        # Bad level
        self.assertTrue(correct_log_codes.LOG_PATTERN.search('<LCT12345678Q>') is None)

    def test_it_makes_a_file_if_duplicates_in_copy_mode(self):
        '''If we run in copy mode and actually find duplicates, we make a copy file with corrections'''
        res_file = get_copy_file_name(self.duplicates_file)
        self.assertTrue(os.path.isfile(res_file))

    def test_it_does_not_make_a_file_if_no_duplicates_in_copy_mode(self):
        '''If we run in copy mode do  not find duplicates, we don't make a copy file'''
        res_file = get_copy_file_name(self.no_duplicates_file)
        self.assertFalse(os.path.isfile(res_file))

    def test_it_fixes_duplicates(self):
        '''Tests that a file of only duplicated log codes will replace all duplicates'''
        res_file = get_copy_file_name(self.duplicates_file)
        # Get the original log code lineset (anything giving a hit for our log code regex)
        with open(self.duplicates_file) as dup_file:
            original_codes = [x.strip() for x in dup_file.readlines() if  get_code_match(x)]
            num_codes = len(original_codes)
            unique_original_codes = set(original_codes)
        # Get the output log code lineset
        with open(res_file) as out_file:
            out_lines = set([x.strip() for x in out_file.readlines() if get_code_match(x)])
        # Duplicate file should only have one log code
        self.assertEqual(len(unique_original_codes), 1)
        # But there should be lots of occurrences of that code
        self.assertTrue(num_codes > 1)
        # And after, we should have a separate code per line
        self.assertEqual(len(out_lines), num_codes)

    def test_it_preserves_whitespace(self):
        '''Tests that a duplicated log codes with whitespace will keep whitespace the same'''
        res_file = get_copy_file_name(self.duplicates_file)
        with open(self.duplicates_file) as dup_file:
            old_matches = [get_code_match(x) for x in dup_file]
        with open(res_file) as out_file:
            new_matches = [get_code_match(x) for x in out_file]
        at_least_one = False
        for old, new in zip(old_matches, new_matches):
            # Matches better align, or else something is very wrong
            self.assertTrue((old is None and new is None) or (old is not None and new is not None))
            # If we match, ensure that span boundaries align
            if old is not None:
                at_least_one = True
                old_start, old_end = old.span()
                new_start, new_end = new.span()
                self.assertEqual(old_start, new_start)
                self.assertEqual(old_end, new_end)
        # Make sure  we actually verified span boundaries at least once
        self.assertTrue(at_least_one)

    def test_it_fills_prefixed_placeholders(self):
        '''Tests that it fills a prefixed placeholders correctly'''
        res_file = get_copy_file_name(self.prefix_file)
        with open(self.prefix_file) as pref_file:
            old_lines = pref_file.readlines()
        with open(res_file) as out_file:
            new_lines = out_file.readlines()
            new_matches = [get_code_match(x) for x in new_lines]
        at_least_one = False
        for idx, new in enumerate(new_matches):
            # Only new will match on the overall code regex
            if new is not None:
                at_least_one = True
                code_start, _ = new.span()
                # Index into corresponding line number of match & check prefixes
                old_prefix = old_lines[idx][code_start: code_start + 4]
                new_prefix = new_lines[idx][code_start: code_start + 4]
                self.assertEqual(old_prefix, new_prefix)
                # Ensure that the prefix used was not the override prefix
                self.assertNotEqual(new_prefix, self.prefix)
        # Make sure  we actually verified prefixes at least once
        self.assertTrue(at_least_one)

    def test_it_fills_prefixless_placeholders(self):
        '''Tests that it fills prefixless placeholders with the provided prefix'''
        res_file = get_copy_file_name(self.prefixless_file)
        with open(self.prefix_file) as pref_file:
            old_lines = pref_file.readlines()
        with open(res_file) as out_file:
            new_lines = out_file.readlines()
            new_matches = [get_code_match(x) for x in new_lines]
        at_least_one = False
        for idx, new in enumerate(new_matches):
            # Only new will match on the overall code regex
            if new is not None:
                at_least_one = True
                code_start, _ = new.span()
                # Index into corresponding line number of match & check prefixes
                new_prefix = new_lines[idx][code_start: code_start + 4]
                # Ensure that the prefix used was the override
                self.assertEqual(new_prefix, self.prefix)
        # Make sure  we actually verified prefixes at least once
        self.assertTrue(at_least_one)
