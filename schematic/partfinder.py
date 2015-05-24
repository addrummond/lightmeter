import os
import urllib.request
import urllib.parse
import re
import json
import sys
import pprint

CURRENCY = 'USD'
LIMIT = 100

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

def basic_urlopts():
    return [
        ('apikey', API_KEY),
        ('start', 0),
        ('limit', LIMIT)
    ]

def generic_urlopts(opts, searchopts):
    if opts is None:
        opts = { }
    if searchopts is None:
        searchopts = { }

    urlopts = []

    if searchopts.get('by') is None or searchopts.get('by') == 'price':
        urlopts.append(('sortby', 'avg_price asc'))
    else:
        assert False

    if searchopts.get('seller'):
        urlopts.append(('filter[fields][offers.seller.name][]', searchopts['seller']))

    # In stock only.
    urlopts.append(('filter[fields][avg_avail][]', '[1 TO *]'))
    # ROHS compliant.
    urlopts.append(('filter[fields][specs.rohs_status.value][]', 'Compliant'))

    return urlopts

def pull_seller_info(seller, offers):
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
        product_url = o.get('product_url')

    if sku is None or prices is None or product_url is None:
        return [ ]
    else:
        return [
            dict(prices=prices, sku=sku, product_url=product_url)
        ]

def find_best_price(seller, q, results):
    if q is None:
        q = 1
    # Find lowest price for selected seller in selected quantity.
    i = 0
    prs = [ ]
    for r in results:
        assert jsk(r, 'item', 'offers') is not None
        info = pull_seller_info(seller, r['item']['offers'])
        if len(info) == 0:
            continue
        for pr in info[0]['prices']:
            pr[1] = float(pr[1])
        prcs = list(sorted(info[0]['prices'], key=lambda x: -x[0]))
        for num,prc in prcs:
            if num <= q:
                prs.append((r, prc))
        i += 1
    if len(prs) == 0:
        return [ ]
    prs.sort(key=lambda x: x[1])
    return [prs[0][0]]

def best_results(searchopts, results):
    if searchopts.get('by') == 'price':
        return find_best_price(searchopts['seller'], searchopts.get('bulk'), results)
    else:
        return results

def from_best(seller, best, **extras):
    for r in best:
        info = pull_seller_info(seller, r['item']['offers'])
        if len(info) == 0:
            continue
        d = dict(
            prices=info[0]['prices'],
            sku=info[0]['sku'],
            octopart_url=jsk(r, 'item', 'octopart_url'),
            product_url=info[0]['product_url']
        )
        for k, v in extras.items():
            d[k] = v
        return [d]
    return [ ]

def check_searchopts(searchopts):
    assert type(searchopts.get('seller')) == type('')

def get_mfg_part(opts, searchopts):
    assert type(opts.get('mfg_part_num')) == type('')
    check_searchopts(searchopts)

    partnum = opts['mfg_part_num']

    url = API_BASE_URL + 'parts/search?'
    urlopts = (basic_urlopts() +
               [('filter[fields][mpn][]', partnum)] +
               generic_urlopts(opts, searchopts))
    url += urllib.parse.urlencode(urlopts)
    data = urllib.request.urlopen(url).read()
    j = json.loads(data.decode('utf-8'))

    best = best_results(searchopts, j['results'])
    return from_best(
        searchopts['seller'],
        best,
        kind='component_by_mfg_part_num'
    )

def get_rescapind(kind, opts, searchopts):
    assert kind == 'resistor' or kind == 'capacitor' or kind == 'inductor'
    check_searchopts(searchopts)

    ance = dict(resistor='resistance', capacitor='capacitance', inductor='inductance')
    uids = dict(resistor='cd01000bfc2916c6', capacitor='f8883582c9a8234f', inductor='bf4e72448e766489')

    url = API_BASE_URL + 'parts/search?'
    urlopts = (basic_urlopts() +
               [('filter[fields][category_uids][]', uids[kind])] +
               generic_urlopts(opts, searchopts))

    if opts.get('value') is not None:
        urlopts.append(('filter[fields][specs.%s.value][]' % ance[kind], convert_value(opts['value'])))
    if opts.get('package') is not None:
        urlopts.append(('filter[fields][specs.case_package.value][]', opts['package']))
    if opts.get('tolerance') is not None:
        urlopts.append(('filter[fields][specs.%s_tolerance.value][]' % ance[kind], '±' + opts['tolerance']))

    url += urllib.parse.urlencode(urlopts)
    data = urllib.request.urlopen(url).read()
    j = json.loads(data.decode('utf-8'))

    if j.get('results') is None or len(j['results']) == 0:
        return [ ]

    best = best_results(searchopts, j['results'])
    return from_best(
        searchopts['seller'],
        best,
        kind=kind,
        value=opts.get('value'),
    )

def get_resistor(opts, searchopts=None):
    return get_rescapind('resistor', opts, searchopts)

def get_capacitor(opts, searchopts):
    return get_rescapind('capacitor', opts, searchopts)

def get_inductor(opts, searchopts):
    return get_rescapind('inductor', opts, searchopts)

#print(get_resistor(dict(value="10k", package="0402"), dict(by='price', bulk=1000, seller='Digi-Key')))
print(get_mfg_part(dict(mfg_part_num="MMA8653FCR1"), dict(seller="Digi-Key", by='price', bulk=1000)))
