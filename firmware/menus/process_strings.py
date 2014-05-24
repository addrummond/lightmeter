import sys
import re

#
# Strings for menus are stored as arrays of 1-byte indices into a string table.
# Each string in the table consists of a zero-terminated sequence of compacted
# 6-bit characters, where each character is an offset in the the relevant table
# of characters.
#

def map_char_to_12px_name(c):
    if re.match(r"[A-Za-z]", c):
        return 'CHAR_12PX_' + c.upper()
    elif c == '/':
        return 'CHAR_12PX_SLASH'
    elif c == ' ':
        return '63'
    else:
        sys.stderr.write("Could not map char '%s' to 12px name\n" % c)
        sys.exit(1)

def compact_string(string):
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
            c = map_char_to_12px_name(string[i]) + '_O'
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

def output_strings_table(f, strings, defines):
    f.write('const uint8_t MENU_STRING_TABLE[] PROGMEM = {\n')
    for v in defines.itervalues():
        f.write(compact_string(v))
    f.write('\n};\n')

blank_or_comment_regex = re.compile(r"^\s*(?:#.*)?$")
line_regex = re.compile(r"^\s*(\w*)\s*(?:@(\w+))?\s*\{((?:(?:[^}\\])|(?:\\.))*)\}\s*(?:#.*)?$")
text_regex = re.compile(r"((?:[^$]+)|(?:\$\w*))")
def process(contents):
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

        textelems = re.finditer(text_regex, string_text)
        aggregate_text = ""
        for m in textelems:
            if m.group(0)[0] == '$':
                if not defines.has_key(m.group(0)[1:]):
                    sys.stderr.write("Could not find define '%s'" % m.group(0)[1:])
                    sys.exit(1)
                aggregate_text += defines[m.group(0)[1:]]
            else:
                aggregate_text += m.group(0)

        strings[string_name] = aggregate_text
        if def_name is not None and len(def_name) > 0:
            defines[def_name] = aggregate_text

        line_number += 1

    output_strings_table(sys.stdout, strings, defines)

if __name__ == '__main__':
    fname = sys.argv[1]
    f = open(fname)
    contents = f.read()
    process(contents)
