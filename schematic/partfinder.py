import os
import urllib.request
import urllib.parse
import json

API_KEY = os.environ['OCTOPART_API_KEY']
API_BASE_URL = """http://octopart.com/api/v3/"""

ex= """'https://octopart.com/search?filter[fields][category_uids][]=5c6a91606d4187ad&filter[fields][specs.resistance.value][]=100000&filter[fields][specs.case_package.value][]=0402'"""
def get_resistor():
    url = API_BASE_URL + 'parts/search?' + urllib.parse.urlencode([
        ('apikey', API_KEY),
        ('start', 0),
        ('limit', 10),
        ('filter[fields][category_uids][]', 'cd01000bfc2916c6'),
        ('filter[fields][specs.case_package.value][]', '0402'),
        ('filter[fields][specs.resistance.value]', '10000')
    ])
    print(url)
    data = urllib.request.urlopen(url).read()
    j = json.loads(data.decode('utf-8'))
    for r in j['results']:
        print(r['item']['octopart_url'])

get_resistor()
