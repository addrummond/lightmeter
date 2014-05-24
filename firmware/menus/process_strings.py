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
# The table of strings is then organized as follows. MENU_DEFINES is a list
# of pointers to 6-bit compacted strings. Each string is then an array of
# uint8_t named MENU_STRING_*. There are two special characters,
# SPECIAL_DEFINCLUDE6 and SPECIAL_DEFINCLUDE12, which specify that a string from
# MENU_DEFINES should be spliced into the string. This special character is
# followed by either one or two two 6-bit values giving a
# 6 or 12-bit index into MENU_DEFINES (little-endian order for 12-bit indices).
# The value of 12-bit indices should not exceed 255.
#

SPECIAL_SPACE = 63
SPECIAL_DEFINCLUDE6 = 62
SPECIAL_DEFINCLUDE12 = 61

def map_char_to_12px_name(c):
    if re.match(r"[A-Za-z0-9]", c):
        return 'CHAR_12PX_' + c.upper() + '_O'
    elif c == '/':
        return 'CHAR_12PX_SLASH_O'
    elif c == '+':
        return 'CHAR_12PX_PLUS_O'
    elif c == '-':
        return 'CHAR_12PX_MINUS_O'
    elif c == '/':
        return 'CHAR_12PX_SLASH_O'
    elif c == ' ':
        return str(SPECIAL_SPACE)
    else:
        sys.stderr.write("Could not map char '%s' to 12px name\n" % c)
        sys.exit(1)

def compact_string(string):
    """'string' is a sequence of characters/integers"""
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
            else:
                assert False
        val = '(' + c + ' << ' + str(bit_offset) + ')'
        bytes[byte_i].append(val)
        if bit_offset > 2:
            val = '(' + c + ' >> ' + str(6 - (8 - bit_offset)) + ')'
            bytes[byte_i+1].append(val)
    while len(bytes[-1]) == 0:
        bytes.pop()
    for byte in bytes:
        out += '|'.join(byte) + ','
    return out

def output_strings_table(fh, fc, strings, defines):
    defkeys = defines.keys()
    defkeys.sort()
    index = 0
    for k in defkeys:
        v = defines[k]
        fc.write('const uint8_t MENU_DEFINE_' + str(index) + '[] PROGMEM = { ' + compact_string(v) + '};\n')
        index += 1

    fh.write('extern const uint8_t *MENU_DEFINES[];\n')
    fc.write('const uint8_t *MENU_DEFINES[] PROGMEM = {\n')
    index = 0
    for k in defkeys:
        fc.write('    &MENU_DEFINE_' + str(index) + ',\n')
        index += 1
    fc.write('};\n\n')

    for k, v in strings.iteritems():
        fh.write('const uint8_t MENU_STRING_' + k + '[];\n')
        fc.write('const uint8_t MENU_STRING_' + k + '[] PROGMEM = { ')
        s = []
        for elem in v:
            if elem['type'] == 'lit':
                for c in elem['text']:
                    s.append(c)
            elif elem['type'] == 'def':
                i = defkeys.index(elem['name'])
                assert i < 256
                if i < math.pow(2,6):
                    s.append(SPECIAL_DEFINCLUDE6)
                    s.append(i)
                else:
                    s.append(SPECIAL_DEFINCLUDE12)
                    # Make 12-bit index.
                    s.append(i & 0b111111)
                    s.append(i >> 6)
            else:
                assert False
        fc.write(compact_string(s))
        fc.write('};\n\n')

blank_or_comment_regex = re.compile(r"^\s*(?:#.*)?$")
line_regex = re.compile(r"^\s*(\w*)\s*(?:@(\w+))?\s*\{((?:(?:[^}\\])|(?:\\.))*)\}\s*(?:#.*)?$")
text_regex = re.compile(r"((?:[^$]+)|(?:\$\w*))")
def process(fh, fc, contents):
    strings = { }
    defines = { }

    lines = re.split(r"\r?\n", contents)
    line_number = 0
    for l in lines:
        if re.match(blank_or_comment_regex, l):
            continue
        m = re.match(line_regex, l)
        if not m:
            sys.stderr.write("Parse error on line %i" % line_number)
            sys.exit(1)

        string_name = m.group(1)
        def_name = m.group(2)
        string_text = m.group(3).strip()

        textelems = list(re.finditer(text_regex, string_text))
        aggregate_text = ""
        for m in textelems:
            if m.group(0)[0] == '$':
                if not defines.has_key(m.group(0)[1:]):
                    sys.stderr.write("Could not find define '%s'" % m.group(0)[1:])
                    sys.exit(1)
                aggregate_text += defines[m.group(0)[1:]]
            else:
                aggregate_text += m.group(0)

        strings[string_name] = []
        if def_name is None or len(def_name) == 0:
            for m in textelems:
                if m.group(0)[0] == '$':
                    strings[string_name].append(dict(type='def', name=m.group(0)[1:]))
                else:
                    strings[string_name].append(dict(type='lit', text=m.group(0)))
        else:
            strings[string_name].append(dict(type='def', name=def_name))

        if def_name is not None and len(def_name) > 0:
            defines[def_name] = aggregate_text

        line_number += 1

    output_strings_table(fh, fc, strings, defines)

if __name__ == '__main__':
    fname = sys.argv[1]
    f = open(fname)
    contents = f.read()

    fh = open("menu_strings.h", "w")
    fc = open("menu_strings.c", "w")

    fc.write('#include <menu_strings.h>\n\n')

    fh.write('#ifndef MENU_STRINGS_H\n')
    fh.write('#define MENU_STRINGS_H\n\n')
    fh.write('#include <bitmaps.h>\n\n')

    process(fh, fc, contents)

    fh.write('\n#endif\n')

    fh.close()
    fc.close()
