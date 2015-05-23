import os
import urllib.request
import urllib.parse
import re
import json
import sys

API_KEY = os.environ['OCTOPART_API_KEY']
API_BASE_URL = """http://octopart.com/api/v3/"""

def parse_value(v):
    if type(v) == type(0):
        return v
    elif type(v) == type(1.0):
        return v
    elif type(v) == type(""):
        m = re.match(r"^(\d+)([.kupmM]?)(\d*)([kupmM]?)", v)
        if not m:
            return False
        if m.group(2) != '.' and m.group(2) != '' and m.group(4) != '':
            return False
        multiplier = m.group(2) if m.group(2) != '' and m.group(2) != '.' else m.group(4)
        mul = 1.0
        if multiplier == 'M':
            mul = 1000000.0
        elif multiplier == 'k':
            mul = 1000.0
        elif multiplier == 'm':
            mul = 0.001
        elif multiplier == 'u':
            mul = 0.000001
        elif multiplier == 'p':
            mul = 0.000000000001
        n = m.group(1)
        if m.group(2) != '':
            n += '.'
        n += m.group(3)
        n = float(n) * mul
        return n
    else:
        assert False

ex= """'https://octopart.com/search?filter[fields][category_uids][]=5c6a91606d4187ad&filter[fields][specs.resistance.value][]=100000&filter[fields][specs.case_package.value][]=0402'"""
def get_resistor(opts, by='price'):
    url = API_BASE_URL + 'parts/search?'

    urlopts = [
        ('apikey', API_KEY),
        ('start', 0),
        ('limit', 1),
        ('filter[fields][category_uids][]', 'cd01000bfc2916c6')
    ]

    if by is None:
        pass
    elif by == 'price':
        urlopts.append(('sortby', 'avg_price asc'))
    else:
        assert False

    if opts.get('value') is not None:
        urlopts.append(('filter[fields][specs.resistance.value][]', parse_value(opts['value'])))
    if opts.get('package') is not None:
        urlopts.append(('filter[fields][specs.case_package.value][]', opts['package']))

    url += urllib.parse.urlencode(urlopts)
    print(url)
    data = urllib.request.urlopen(url).read()
    j = json.loads(data.decode('utf-8'))
    for r in j['results']:
        print(r['item']['octopart_url'])

get_resistor(dict(value="10k", package="0402"))
