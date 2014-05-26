import sys
import re
import math

#
# The scheme for storing strings is as follows.
#
# The basic format for a string is a zero-terminated sequence of compacted
# 6-bit characters, where each character is an offset in the the relevant table
# of characters.
#
# As an extension of this format, strings may include the special characters
# SPECIAL_DEFINCLUDE6 and SPECIAL_DEFINCLUDE12. These specify that a string
# from an array MENU_DEFINES should be spliced in place of the special character.
# Each special character is followed either by one or two 6-bit values giving
# an index into the MENU_DEFINES array (little-endian format for 12-bit indices).
# The value of 12-bit indices should not exceed 255.
#
# The menu strings themselves are stored in variables named MENU_STRING_*.
#
# To decode a string, one reads 6-bit chars from MENU_STRING_* until the
# terminating zero is found. Each time a special char is read, the relevant
# entry in MENU_DEFINES is looked up. Strings in MENU_DEFINES may themselves
# contain special chars.
#
# There is another special character, SPECIAL_LONGONLY, which is placed on
# either side of substrings which can be edited out to generate a shorter
# version of the string.
#
# Strings can be placed in named groups. The script outputs #defines giving
# max short/long lengths for the strings in each group.
#

SPECIAL_SPACE = 63
SPECIAL_DEFINCLUDE6 = 62
SPECIAL_DEFINCLUDE12 = 61
SPECIAL_LONGONLY = 60

def map_char_to_12px_name(c):
    if re.match(r"[A-Za-z0-9]", c):
        return 'CHAR_12PX_' + c.upper() + '_CODE'
    elif c == '/':
        return 'CHAR_12PX_SLASH_CODE'
    elif c == '+':
        return 'CHAR_12PX_PLUS_CODE'
    elif c == '-':
        return 'CHAR_12PX_MINUS_CODE'
    elif c == '/':
        return 'CHAR_12PX_SLASH_CODE'
    elif c == ' ':
        return 'MENU_STRING_SPECIAL_SPACE'
    else:
        sys.stderr.write("Could not map char '%s' to 12px name\n" % c)
        sys.exit(1)

def compact_string(string):
    """'string' is a sequence of characters/integers/tuples of strings [gross]"""
    out = ""
    n_bytes = (len(string)*8/6) + 1
    bytes = [[] for x in xrange(n_bytes)]
    for i in xrange(len(string)+1):
        byte_i = i*6/8
        bit_offset = i*6%8
        c = None
        if i == len(string):
            c = '0'
        else:
            si = string[i]
            if type(si) == type('c'):
                c = map_char_to_12px_name(string[i])
            elif type(si) == type(0):
                assert si < 64
                c = str(si)
            elif type(si) == type(()):
                c = si[0]
            else:
                assert False
        val = '(uint8_t)(' + c + ' << ' + str(bit_offset) + ')'
        bytes[byte_i].append(val)
        if bit_offset > 2:
            val = '(' + c + ' >> ' + str((6 - (6 - (8 - bit_offset)))) + ')'
            bytes[byte_i+1].append(val)
    while len(bytes[-1]) == 0:
        bytes.pop()
    for byte in bytes:
        out += '|'.join(byte) + ','
    return out

def get_length(seq, defines, long=False):
    length = 0
    in_long = False
    for elem in seq:
        if elem['type'] == 'lit':
            if not (in_long and not long):
                length += len(elem['text'])
        elif elem['type'] == 'def':
            if not (in_long and not long):
                length += get_length(defines[elem['name']], defines, long)
        elif elem['type'] == 'longonly':
            in_long = not in_long
        else:
            assert False
    return length

def output_strings_table(fh, fc, strings, defines, groups_to_lists_of_string_names):
    def out(table):
        s = []
        for elem in table:
            if elem['type'] == 'lit':
                for c in elem['text']:
                    s.append(c)
            elif elem['type'] == 'longonly':
                s.append(('MENU_STRING_SPECIAL_LONGONLY',))
            elif elem['type'] == 'def':
                i = defkeys.index(elem['name'])
                assert i < 256
                if i < math.pow(2,6):
                    s.append(('MENU_STRING_SPECIAL_DEFINCLUDE6',))
                    s.append(i)
                else:
                    s.append(('MENU_STRING_SPECIAL_DEFINCLUDE12',))
                    # Make 12-bit index.
                    s.append(i & 0b111111)
                    s.append(i >> 6)
            else:
                assert False
        return compact_string(s)

    # Globals max lengths.
    short_lengths = [get_length(s, defines, long=False) for s in strings.itervalues()]
    long_lengths = [get_length(s, defines, long=True) for s in strings.itervalues()]
    fh.write('#define MENU_MAX_SHORT_STRING_LENGTH %i\n' % max(short_lengths))
    fh.write('#define MENU_MAX_LONG_STRING_LENGTH %i\n' % max(long_lengths))

    # Max lengths for each group.
    for g, lst in groups_to_lists_of_string_names.iteritems():
        short_lengths = [get_length(strings[n], defines, long=False) for n in lst]
        long_lengths = [get_length(strings[n], defines, long=True)  for n in lst]
        fh.write('#define MENU_GROUP_%s_MAX_SHORT_STRING_LENGTH %i\n' % (g, max(short_lengths)))
        fh.write('#define MENU_GROUP_%s_MAX_LONG_STRING_LENGTH %i\n' % (g, max(long_lengths)))

    defkeys = defines.keys()
    defkeys.sort()
    index = 0
    for k in defkeys:
        v = defines[k]
        fc.write('const uint8_t MENU_DEFINE_' + str(index) + '[] PROGMEM = { ' + out(v) + '};\n')
        index += 1

    fh.write('typedef const uint8_t *const_ptr_to_uint8_t;')
    fh.write('extern const const_ptr_to_uint8_t MENU_DEFINES[];\n')
    fc.write('const const_ptr_to_uint8_t MENU_DEFINES[] PROGMEM = {\n')
    index = 0
    for k in defkeys:
        fc.write('    MENU_DEFINE_' + str(index) + ',\n')
        index += 1
    fc.write('};\n\n')

    for k, v in strings.iteritems():
        fh.write('extern const uint8_t MENU_STRING_' + k + '[];\n')
        ll = get_length(v, defines, long=True)
        sl = get_length(v, defines, long=False)
        fh.write('#define MENU_STRING_' + k + '_LONG_LENGTH ' + str(ll) + '\n')
        fh.write('#define MENU_STRING_' + k + '_SHORT_LENGTH ' + str(sl) + '\n')
        fc.write('const uint8_t MENU_STRING_' + k + '[] PROGMEM = { ')
        fc.write(out(v))
        fc.write('};\n\n')

# TODO: Backslash escaping not quite properly handled yet.
blank_or_comment_regex = re.compile(r"^\s*(?:#.*)?$")
line_regex = re.compile(r"^\s*((?:[\w,]*:)?\w*)\s*(?:@(\w+))?\s*\{((?:(?:[^}\\])|(?:\\.))*)\}\s*(?:#.*)?$")
text_regex = re.compile(r"\]|\[|(?:(?:[^$\[\]]|(?:\\.))+)|(?:\$\w*)")
def process(fh, fc, contents):
    strings = { }
    defines = { }
    groups_to_lists_of_string_names = { }

    lines = re.split(r"\r?\n", contents)
    line_number = 1
    for l in lines:
        if re.match(blank_or_comment_regex, l):
            continue
        m = re.match(line_regex, l)
        if not m:
            sys.stderr.write("Parse error on line %i\n" % line_number)
            sys.exit(1)

        g1 = m.group(1)
        ns = g1.split(":")
        string_name = None
        group_names = [ ]
        if min(map(len, ns)) == 0 or len(ns) == 0 or len(ns) > 2:
            sys.stderr.write("Bad string name one line %i\n" % line_number)
            sys.exit(1)
        if len(ns) == 1:
            string_name = ns[0]
        else:
            string_name = ns[1]
            group_names = ns[0].split(",")
            if min(map(len, group_names)) == 0:
                sys.stderr.write("Bad group name on line %i\n" % line_number)
                sys.exit(1)

        def_name = m.group(2)
        string_text = m.group(3).strip()

        textelems = list(re.finditer(text_regex, string_text))

        seq = [ ]
        open_count = 0
        for m in textelems:
            if m.group(0)[0] == '$':
                seq.append(dict(type='def', name=m.group(0)[1:]))
            elif m.group(0) == '[':
                if open_count > 0:
                    sys.stderr.write("Nested '[' at line %i\n" % line_number)
                    sys.exit(1)
                else:
                    open_count = 1
                    seq.append(dict(type='longonly'))
            elif m.group(0) == ']':
                if open_count < 1:
                    sys.stderr.write("Closing ']' without openening '[' at line %i\n" % line_number)
                    sys.exit(1)
                open_count -= 1
                seq.append(dict(type='longonly'))
            else:
                seq.append(dict(type='lit', text=m.group(0)))

        if def_name is not None and len(def_name) > 0:
            defines[def_name] = seq
            strings[string_name] = [dict(type='def',name=def_name)]
        else:
            strings[string_name] = seq

        for g in group_names:
            if not groups_to_lists_of_string_names.has_key(g):
                groups_to_lists_of_string_names[g] = [ ]
            groups_to_lists_of_string_names[g].append(string_name)

        line_number += 1

    output_strings_table(fh, fc, strings, defines, groups_to_lists_of_string_names)

if __name__ == '__main__':
    fname = sys.argv[1]
    f = open(fname)
    contents = f.read()

    fh = open("menu_strings_table.h", "w")
    fc = open("menu_strings_table.c", "w")

    fc.write('#include <menus/menu_strings.h>\n\n')
    fc.write('#include <bitmaps/bitmaps.h>\n')

    fh.write('#include <readbyte.h>\n')
    fh.write('#ifndef MENU_STRINGS_TABLE_H\n')
    fh.write('#define MENU_STRINGS_TABLE_H\n\n')
    fh.write('#include <bitmaps/bitmaps.h>\n\n')
    fh.write('#define MENU_STRING_SPECIAL_SPACE %i\n' % SPECIAL_SPACE)
    fh.write('#define MENU_STRING_SPECIAL_DEFINCLUDE6 %i\n' % SPECIAL_DEFINCLUDE6)
    fh.write('#define MENU_STRING_SPECIAL_DEFINCLUDE12 %i\n' % SPECIAL_DEFINCLUDE12)
    fh.write('#define MENU_STRING_SPECIAL_LONGONLY %i\n\n' % SPECIAL_LONGONLY)

    process(fh, fc, contents)

    fh.write('\n#endif\n')

    fh.close()
    fc.close()
