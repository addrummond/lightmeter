import os
import urllib.request
import urllib.parse
import re
import json
import sys
import pprint

CURRENCY = 'USD'

API_KEY = os.environ['OCTOPART_API_KEY']
API_BASE_URL = """http://octopart.com/api/v3/"""

def jsk(d, *ks):
    current = d
    for k in ks[:-1]:
        v = current.get(k)
        if v is None:
            return None
        current = v
    return current.get(ks[-1])

def convert_value(v):
    if type(v) == type(0):
        return v
    elif type(v) == type(1.0):
        return v
    elif type(v) == type(""):
        m = re.match(r"^(\d+)([.kunpmM]?)(\d*)([kunpmM]?)", v)
        if not m:
            return False
        if m.group(2) != '.' and m.group(2) != '' and m.group(4) != '':
            return False
        mul = 0
        multiplier = m.group(2) if m.group(2) != '' and m.group(2) != '.' else m.group(4)
        if multiplier == 'M':
            mul = 6
        elif multiplier == 'k':
            mul = 3
        elif multiplier == 'm':
            mul = -3
        elif multiplier == 'u':
            mul = -6
        elif multiplier == 'n':
            mul = -9
        elif multiplier == 'p':
            mul = -12
        n = m.group(1)
        if m.group(2) != '':
            n += '.'
        n += m.group(3)
        if mul != 0:
            n += 'e' + str(mul)
        return n
    else:
        assert False

def append_generic_urlopts(opts, searchopts, urlopts):
    if opts is None:
        opts = { }
    if searchopts is None:
        searchopts = { }

    urlopts = []
    if opts.get('rohs') is True:
        urlopts.append(('filter[fields][specs.rohs_status.value][]', 'Compiant'))
    elif opts.get('rohs') is not None and opts.get('rohs') is not False:
        assert False

    if searchopts.get('by') is None or searchopts.get('by') == 'price':
        urlopts.append(('sortby', 'avg_price asc'))
    else:
        assert False

    if searchopts.get('seller'):
        urlopts.append(('filter[fields][offer.seller.name][]', searchopts['seller']))

def get_seller_info(seller, offers):
    prices = [ ]
    sku = None
    for o in offers:
        if o['seller']['name'] != seller:
            continue
        assert o.get('seller') is not None
        assert o['seller'].get('name') is not None
        assert type(o.get('prices')) == type({ })
        assert type(o['prices'].get(CURRENCY)) == type([])
        prices = o['prices'][CURRENCY]
        sku = o.get('sku')

    if sku is None or prices is None:
        return [ ]
    else:
        return [
            dict(prices=prices, sku=sku)
        ]

def get_rescapind(kind, opts, searchopts):
    if kind != 'resistor' and kind != 'capacitor' and kind != 'inductor':
        assert False
    if type(searchopts.get('seller')) != type(''):
        assert False

    url = API_BASE_URL + 'parts/search?'

    ance = dict(resistor='resistance', capacitor='capacitance', inductor='inductance')
    uids = dict(resistor='cd01000bfc2916c6', capacitor='f8883582c9a8234f', inductor='bf4e72448e766489')

    urlopts = [
        ('apikey', API_KEY),
        ('start', 0),
        ('limit', 10),
        ('filter[fields][category_uids][]', uids[kind])
    ]
    append_generic_urlopts(opts, searchopts, urlopts)

    if opts.get('value') is not None:
        urlopts.append(('filter[fields][specs.%s.value][]' % ance[kind], convert_value(opts['value'])))
    if opts.get('package') is not None:
        urlopts.append(('filter[fields][specs.case_package.value][]', opts['package']))
    if opts.get('tolerance') is not None:
        urlopts.append(('filter[fields][specs.%s_tolerance.value][]' % ance[kind], '±' + opts['tolerance']))

    url += urllib.parse.urlencode(urlopts)
    data = urllib.request.urlopen(url).read()
    j = json.loads(data.decode('utf-8'))

    rets = []
    for r in j['results']:
       assert type(r.get('item')) == type({ })
       assert type(r['item'].get('offers')) == type([ ])
       info = get_seller_info(searchopts['seller'], r['item']['offers'])
       if len(info) == 0:
           continue

       d = dict(
           kind=kind,
           value=opts.get('value'),
           prices=info[0]['prices'],
           sku=info[0]['sku'],
           octopart_url=jsk(r, 'item', 'octopart_url')
       )
       rets.append(d)
       break
    return rets

def get_resistor(opts, searchopts=None):
    return get_rescapind('resistor', opts, searchopts)

def get_capacitor(opts, searchopts):
    return get_rescapind('capacitor', opts, searchopts)

def get_inductor(opts, searchopts):
    return get_rescapind('inductor', opts, searchopts)

print(get_capacitor(dict(value="10u", package="0402", rohs=True), dict(by='price', seller='Digi-Key')))
