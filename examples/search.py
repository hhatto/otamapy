import os
import json
from otama import Otama

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
TARGET_FILE = os.path.join(BASE_DIR, 'image/lena.jpg')
CONFIG_FILE = os.path.join(BASE_DIR, 'test.conf')
DATA_DIR = os.path.join(BASE_DIR, 'data')

kvs = None
with open(os.path.join(DATA_DIR, 'kvs.json')) as fp:
    kvs = json.load(fp)

last_id = None
db = Otama(CONFIG_FILE)
for result in db.search(10, TARGET_FILE):
    key = result['id']
    print("sim=%.3f, file=%s" % (result['similarity'], kvs[key]))
    last_id = result['id']

print(db.exists(last_id))
print(db.exists('a' + last_id[1:-1] + 'b'))
