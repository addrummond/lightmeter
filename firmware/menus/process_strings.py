import sys
import re

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

    print strings
    print defines

if __name__ == '__main__':
    fname = sys.argv[1]
    f = open(fname)
    contents = f.read()
    process(contents)
