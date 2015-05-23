import os
import urllib.request
import urllib.parse
import json

API_KEY = os.environ['OCTOPART_API_KEY']
API_BASE_URL = """http://octopart.com/api/v3/"""

def get_resistor():
    url = API_BASE_URL + 'parts/search?' + urllib.parse.urlencode([
        ('apikey', API_KEY),
        ('start', 0),
        ('limit', 10),
        ('filter[fields][category_uids][]', 'cd01000bfc2916c6'),
        ('filter[value]', '100k')
    ])
    print(url)
    data = urllib.request.urlopen(url).read()
    j = json.loads(data.decode('utf-8'))
    for r in j['results']:
        print(r['item']['octopart_url'])

get_resistor()
