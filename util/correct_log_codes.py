import argparse
from collections import defaultdict
from glob import glob
import os
from random import randint
import re
import sys

# Things that are good well formatted logs should match the following regex
LOG_PATTERN = re.compile("<[A-Z]{3}[0-9]{8}[FEWID]>")
# In general log codes look like this - if things match this & don't match the
# regex above, they are probably malformed log codes. We don't do anything about
# these, we just complain about them to let people know they're there
GENERAL_PATTERN = re.compile("<[a-zA-Z]+[0-9]+[a-zA-Z]>")
# Placeholder log codes written by devs to be substituted - assumes prefixes
# are part of the log code - we'll fill digits in for these automatically
PLACEHOLDER_PATTERN = re.compile("<[A-Z]{3}X{3}[FEWID]>")
# If we are feeling extra lazy,  we can just write <XXX[LEVEL]> and specific a
# --prefix to fill with a valid log code. If no prefix is provided, we will warn
# about these, but not do anything, just to make sure we don't make any invalid
# assumptions about where log codes live.
GENERIC_PLACEHOLDER_PATTERN = re.compile("<X{3}[FEWID]>")

def parse_args():
    '''Parses arguments and adjust them as needed. Ensure that if we're running in validate mode
    that a prefix is set if one isn't provided.'''
    parser = argparse.ArgumentParser(description='Finds and replaces duplicate log codes.')

    parser.add_argument('-d', '--dir', type=str, required=True,
        help='Root directory of project to parse for log codes.')
    parser.add_argument('-v', '--validate-only', action='store_true', default=False, 
        help='If enabled, throws an error if duplicates are found. Otherwise, replaces them.')
    parser.add_argument('--prefix', type=str, required=False,
        help='3 letter prefix to be used for replace placeholders matching r\'<X{3}[FEWID]>.\'')
    parser.add_argument('-c', '--copy', action='store_true', default=False,
        help='Write output of path/to/file.py to path/to/file_copy.py instead of correcting in place.')
    args = parser.parse_args()
    # If we're in validate mode, make sure we have a prefix so that we log problem placeholders
    if args.validate_only and args.prefix is None:
        print('[VALIDATION MODE WITH NO PREFIX SPECIFIED - USING PLACEHOLDER PREFIX: [FOO]]')
        args.prefix = 'FOO'
    # Otherwise, ensure that our prefix is valid if we gave one
    if args.prefix is not None and len(args.prefix) != 3:
        raise ValueError('ERROR: Prefix [{}] must be of length 3'.format(args.prefix))
    elif args.prefix is not None:
        args.prefix = args.prefix.upper()
    # Ensure that a project directory is specified
    if not os.path.isdir(args.dir):
        raise IOError('ERROR: Project directory [{}] does not exist!'.format(args.dir))
    else:
        args.dir = os.path.abspath(args.dir)
        print('Project root: {}'.format(args.dir))
    return args

def get_log_code_map(project_dir):
    '''Create dictionary mapping discovered log codes from the parent search directory to the
    files in which they are found. Each log code will be mapped to a list of tuples containing
    the line number of discovery, the Regex match object, and the file of discovery. Anything
    that ends up with more than one entry is a duplicated log code.

    Args:
        project_dir str:
            Root of the project directory whose log codes we are validating or correcting.

    Returns:
        list: contains 3 collection.defaultdict objects, each of which map types of log codes
            to occurrences of code like objects matching on different patterns.
    '''    
    # Mantain one dictionary where we keep all valid log codes that we find, where we substitute
    # if we discover any codes with cardinality > 1
    code_map = defaultdict(list)
    # Another dictionary for placeholder codes, where we have prefixes and can easily substitute
    sub_code_map = defaultdict(list)
    # And one more of generic prefixless matches, where we only substitute if --prefix is set
    gen_sub_code_map = defaultdict(list)

    # Recursively grab all of the python files from the root directory
    python_files = glob("{}/**/*.py".format(project_dir), recursive=True)

    # If this file is on our search path, skip it, because we don't care about it
    this_file_path = os.path.abspath(__file__)
    if this_file_path in python_files:
        python_files.remove(this_file_path)

    # Parse one file at a time & pull out all of the things that look like log codes,
    # also saving the whitespace indentation & line number that they were discovered on
    for py_file in python_files:
        get_log_codes(py_file, code_map, sub_code_map, gen_sub_code_map)
    return code_map, sub_code_map, gen_sub_code_map

def get_log_codes(py_file, code_map, sub_code_map, gen_sub_code_map):
    '''Given a specific python file & a dictionary with all of the log codes discovered so far,
    open the file and pull out all of its matches on the LOG_PATTERN regex, keeping these for
    later to check for collisions.

    Args:
        py_file str: 
            Path to the file containing Python code to be parsed for log codes & log code
            lookalikes. Duplicates will later be substituted.
        code_map collections.defaultdict:
            Dictionary mapping log codes to lists of occurrences. A list of length > 1 indicates
            that the log code is a duplicate.
        sub_code_map collections.defaultdict:
            Dictionary mapping log codes to lists of occurrences. The codes are placeholders with
            prefixes and levels, and we will substitute all of them unless we're running in validate
            mode.
        gen_sub_code_map collections.defaultdict:
            Dictionary mapping log codes to lists of generic placeholders without prefixes. We will
            try to replace anything in here if we have a --prefix, otherwise we'll just warn about
            them.
    '''
    with open(py_file, 'r', encoding='utf-8') as p_file:
        for line_no, line in enumerate(p_file):
            general_res = GENERAL_PATTERN.search(line)
            # Consider our patterns, doing the most specific first. We being by  checking to see
            # if it's a well formatted code - if so, add to the sub_code_map. These are only
            # updated if collisions are discovered
            has_updated = update_dict_on_match(py_file, line_no, line, LOG_PATTERN, code_map)
            # If it's not a well-formatted log code, check if it's a placeholder log code with
            # a provided prefix. We can always all matches since we know the prefix and level
            if not has_updated:
                has_updated = update_dict_on_match(py_file,
                    line_no,
                    line,
                    PLACEHOLDER_PATTERN,
                    sub_code_map)
            # If it's not a well-formatted log code or a placeholder with a prefix, check if it's
            # a generic log code placeholder with no prefix. If it is, then we might be able to
            # substitute, but only if a --prefix is passed
            if not has_updated:
                has_updated = update_dict_on_match(py_file,
                    line_no,
                    line,
                    GENERIC_PLACEHOLDER_PATTERN,
                    gen_sub_code_map)
            # Otherwise, check if this looks like a log code, but is just a bit different.
            # Print a note, but don't take any action on it. This will alert on things
            # like the following, but we aren't smart enough to fix them
            # <OR12345678E> [not enough letters]
            # <COR1314E> [not enough digits]
            # <COR12345678F> [unrecognized suffix letter]
            if not has_updated and general_res is not None:
                print('WARN: possible malformatted code [{}] found in [{}] on line [{}]'.format(
                    general_res.group(),
                    py_file, 
                    line_no))

def update_dict_on_match(py_file, line_no, line, pattern, update_dict):
    '''Given a line from a file, the context in which it was parsed from, a regular expression,
    and a code dictionary, add an entry for potential substitution if a match is discovered.

    Args:
        py_file str:
            Path to file in which this line was pulled from.
        line_no int:
            Number of the line being considered.
        line str:
            Line in which we're searching for a regular expression match.
        pattern re.Pattern:
            Regular expression to be matched.
        update_dict collections.defaultdict:
            Dictionary to be updated on pattern match.

    Returns:
        bool: True if an update was made, indicating we should stop trying to match less specific
        regular expressions.
    '''
    search_res = pattern.search(line)
    if search_res is not None:
        matched_code = search_res.group()
        match_info = line_no, search_res, py_file, pattern
        update_dict[matched_code].append(match_info)
        return True

def get_substitution_map(code_map, sub_code_map, gen_sub_code_map, prefix):
    '''Given a dictionary mapping log codes to lists of discovery information, create a
    substitution dictionary mapping filenames to lists of replacement line numbers, the original
    corresponding Regex match object, and a replacement log code.

    Args:
        code_map collections.defaultdict:
            Dictionary mapping log codes to lists of occurrences. A list of length > 1 indicates
            that the log code is a duplicate.
        sub_code_map collections.defaultdict:
            Dictionary mapping log codes to lists of occurrences. The codes are placeholders with
            prefixes and levels, and we will substitute all of them unless we're running in
            validate mode.
        gen_sub_code_map collections.defaultdict:
            Dictionary mapping log codes to lists of generic placeholders without prefixes. We will
            try to replace anything in here if we have a --prefix, otherwise we'll just warn about
            them.
        prefix str:
            3 letter code to be substituted into placeholders of the form <X{3}[FEWID]>. If a
            prefix is not specified, this script will warn about these, but not do anything.
    Returns:
        collections.defaultdict: maps filenames to corresponding substitution information.
    '''
    substitution_map = defaultdict(list)
    gen_codes = set()
    # Update duplicates discovered in the code map [well formatted log codes]
    update_sub_map_with_matches(substitution_map, code_map, gen_codes, True, None)
    # Update all general matches
    update_sub_map_with_matches(substitution_map, sub_code_map, gen_codes, False, None)
    if prefix is not None:
        update_sub_map_with_matches(substitution_map, gen_sub_code_map, gen_codes, False, prefix)
    elif gen_sub_code_map:
        for generic_sub_code, match_infos in gen_sub_code_map.items():
            for (line_no, search_res, py_file, _) in match_infos:
                print('WARN: No prefix specified, but discovered prefixless placeholder [{}] in [{}] on line [{}]'.format(
                    search_res.group(),
                    py_file,
                    line_no
                ))
    return substitution_map

def update_sub_map_with_matches(substitution_map, code_map, gen_codes, duplicates_only, prefix):
    '''Consider a dictionary of regex matches and pull the associated substitution information
    out of it (e.g., generated log code).

    Args:
        substitution_map collections.defaultdict:
            Dictionary mapping files to substitution info.
        code_map collections.defaultdict:
            Dictionary mapping log codes to discovery information.
        gen_codes set:
            Set of generated log codes that have already been used.
        duplcates_only bool:
            If true, only adds duplicate matches to substitutions
        prefix str | None:
            prefix string of length 3 or None. Used for replacing generic placeholder prefixes on
            newly generated log codes.
    '''
    for log_code, occurrences in code_map.items():
        # For every occurrence of the given code that isn't the first one
        replace_matches = occurrences[1:] if duplicates_only else occurrences
        for (line_no, search_res, py_file, pattern) in replace_matches:
            # Get a new log code with the same prefix/suffix, but unique digits
            new_code = generate_random_log_code(log_code, code_map, gen_codes, prefix)
            # Save the generated code, just so that don't generate this one again by accident
            gen_codes.add(new_code)
            # And save the information needed to make the swap into a substitution map
            sub_info = line_no, search_res, new_code, pattern
            substitution_map[py_file].append(sub_info)

def generate_random_log_code(duplicate_code, code_map, generated_codes, sub_prefix):
    '''Given a duplicate log code and the code map, whose keys are the log codes in .py files,
    generate a new log code that hasn't been used anywhere else. We'll use this to replace the
    pass duplicate code.

    Args:
        duplicate_code str:
            Log code to be replaced.
        code_map collections.defaultdict:
            Dictionary mapping log codes to lists of occurrences.
        generated_codes set:
            Set of codes that have already been picked as replacements.
        sub_prefix str | None:
            prefix string of length 3 or None. Used for replacing generic placeholder prefixes on
            newly generated log codes.
    Returns:
        str: replacement log code
    '''
    # Given <LGO12345678W> -> prefix: "<LGO", suffix: "W>", then replace digits
    prefix = duplicate_code[:4] if sub_prefix is None else '<{}'.format(sub_prefix)
    suffix = duplicate_code[-2:]
    is_unique = False
    while not is_unique:
        # Generate a string of 8 digits, combine into a log code, and check if its alerady used
        gen_digits = gen_digits = '{:08d}'.format(randint(0, 10**8))
        gen_code = prefix + gen_digits + suffix
        is_unique = gen_code not in code_map.keys() and gen_code not in generated_codes
    return gen_code

def replace_duplicates(substitution_map, copy):
    '''Given a substitution map, load each of the files needing a replacement sequentially. For
    each file, load the lines as a list, replace the span matched by the log code Regex with the
    generated duplicate, and rewrite the file.

    Args:
        substitution_map collections.defaultdict:
            Dictionary mapping files to substitution info.
        copy bool:
            Indicates whether or path/to/file.py should be corrected to path/to/file_copy.py or
            not if corrections are needed. This is mostly used for testing.
    '''
    # Consider every file in the substitution map
    for sub_file in substitution_map.keys():
        out_file = sub_file if not copy else '{}_copy.py'.format(os.path.splitext(sub_file)[0])
        # For each one, get all lines in the file
        with open(sub_file, 'r', encoding='utf-8') as p_file:
            file_lines = p_file.readlines()
        # Then replace all substitution lines based on the matched regex span
        for (line_no, search_res, new_code, _) in substitution_map[sub_file]:
            # Use the regex match information & loaded file lines to replace the code
            start, stop = search_res.span()
            repl_line = file_lines[line_no][:start] + new_code + file_lines[line_no][stop:]
            file_lines[line_no] = repl_line
            print('Substituting: {} -> {}  in [{}]'.format(search_res.group(), new_code, out_file))
        # Now that we've substituted each of the lines that had duplicates, let's rewrite the file
        with open(out_file, 'w', encoding='utf-8') as p_file:
            p_file.writelines(file_lines)

def show_duplicate_log_codes_and_exit(substitution_map):
    '''In some situtations (e.g., in Jenkins), we only want to show the duplicates and explode
    if any were found. This function just shows where the duplicates are and what the substitutions
    would have been, but doesn't actually do anything about it.

    Args:
        substitution_map collections.defaultdict:
            Dictionary mapping files to substitution info.
    '''
    get_descriptor = lambda p: 'Duplicate' if p == LOG_PATTERN else 'Placeholder'
    for py_file, duplicate_info in substitution_map.items():
        for (line_no, search_res, new_code, pattern) in duplicate_info:
            descriptor = get_descriptor(pattern)
            print('{} log code [{}] at: [{}:{}]. Proposed new code: [{}]'.format(
                descriptor,
                search_res.group(),
                py_file,
                line_no,
                new_code))
    print('ERROR: failed due to prefixless substitution matches, or duplicated log codes')
    sys.exit(1)

if __name__ == '__main__':
    print('[VALIDATING UNIQUE LOG CODES]')
    args = parse_args()
    code_map, sub_code_map, gen_sub_code_map = get_log_code_map(args.dir)
    substitution_map = get_substitution_map(code_map, sub_code_map, gen_sub_code_map, args.prefix)
    if args.validate_only and substitution_map:
        show_duplicate_log_codes_and_exit(substitution_map)
    replace_duplicates(substitution_map, args.copy)
    print('[LOG CODES VALIDATED]')
