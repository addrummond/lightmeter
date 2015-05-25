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

VAL_RE = r"(\d+)([.kunpmM]?)(\d*)([kunpmM]?)"

def convert_split_value(g1, g2, g3, g4):
    if g2 != '.' and g2 != '' and g4 != '':
        return False
    mul = 0
    multiplier = g2 if g2 != '' and g2 != '.' else g4
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
    n = g1
    if g2 != '':
        n += '.'
    n += g3
    if mul != 0:
        n += 'e' + str(mul)
    return n

def convert_value(v):
    if type(v) == type(0):
        return v
    elif type(v) == type(1.0):
        return v
    elif type(v) == type(""):
        m = re.match("^" + VAL_RE + "$", v)
        if not m:
            return False
        return convert_split_value(m.group(1), m.group(2), m.group(3), m.group(4))
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
    # Not at end of life.
    urlopts.append(('filter[fields][specs.lifecycle_status.value][]', 'Active'))

    return urlopts

def pull_seller_info(seller, offers):
    # Bit of a hack. We want a merged list of all prices, but potentially the
    # sku and product URL could be different for different price lists. We
    # assume that they're not.
    pricess = [ ]
    sku = None
    product_url = None
    for o in offers:
        if o['seller']['name'] != seller:
            continue
        assert o.get('seller') is not None
        assert o['seller'].get('name') is not None
        assert type(o.get('prices')) == type({ })
        assert type(o['prices'].get(CURRENCY)) == type([])
        pricess.append(o['prices'][CURRENCY])
        sku = o.get('sku')
        product_url = o.get('product_url')

    if sku is None or product_url is None:
        return [ ]
    else:
        prices = [ ]
        for l in pricess:
            prices += l
        return [dict(prices=prices, sku=sku, product_url=product_url)]

def find_best_price(seller, q, results):
    if q is None:
        q = 1
    # Find lowest price for selected seller in selected quantity.
    prs = [ ]
    for r in results:
        assert jsk(r, 'item', 'offers') is not None
        info = pull_seller_info(seller, r['item']['offers'])
        if len(info) == 0:
            continue
        for pr in info[0]['prices']:
            pr[1] = float(pr[1])
            prs.append((r, pr))
    prs.sort(key=lambda x: (x[1][1], x[1][0]))
    for r,pr in prs:
        num,_ = pr
        if num <= q:
            return [r]
    return [ ]

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
    assert type(opts.get('tolerance')) is type(None) or type(opts.get('tolerance')) is type(0) or type(opts.get('tolerance')) is type(0.0)
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
        urlopts.append(('filter[fields][specs.%s_tolerance.value][]' % ance[kind], '±' + opts['tolerance'] + '%'))
    if opts.get('max_temperature_coefficient') is not None:
        urlopts.append(('filter[fields][specs.temperature_coefficient.value][]', '[* TO ' + convert_value(opts['max_temperature_coefficient']) + ']'))

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

def parse_bom(input):
    parse = [ ]

    lines = re.split(r"\r?\n", input)
    num = 1
    for l in lines:
        # Strip any comment.
        m = re.match(r"^([^#]*)#?.*$", l)
        assert m
        l = m.group(1)

        if re.match("^\s*$", l):
            continue

        m = re.match(r"^\s*(\d+)\s+([RIC$])([\w-]*)(?:~(\d+\.?\d*)%)?\s+(" + VAL_RE + r")\s*$", l)
        if m:
            print(m.groups())
            quantity = int(m.group(1))
            kind = m.group(2)
            f = None
            if kind == 'R':
                f = get_resistor
            elif kind == 'I':
                f = get_inductor
            elif kind == 'C':
                f = get_capacitor
            package = None
            if m.group(3) is not None:
                package = m.group(3)
            tolerance = None
            if m.group(4) is not None:
                tolerance = float(m.group(4))
            parse.append((
                quantity,
                f,
                dict(
                    value=m.group(5),
                    tolerance=tolerance,
                    package=package
                )
            ))
        else:
            m = re.match(r"^\s*(\d+)\s+\$((?:[\w-]+)|(?:\{[^\}]+\}))", l)
            if not m:
                sys.stderr.write("Parse error on line %i.\n" % num)
                sys.exit(1)

            parse.append((int(m.group(1)), get_mfg_part, dict(mfg_part_num=m.group(2))))
    return parse

f = open("bom.partslist")
pprint.pprint(parse_bom(f.read()))
