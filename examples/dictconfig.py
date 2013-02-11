import os
import json
from glob import glob
from otama import Otama


BASE_DIR = os.path.abspath(os.path.dirname(__file__))
DATA_DIR = os.path.join(BASE_DIR, 'data')
CONFIG_FILE = os.path.join(BASE_DIR, 'test.conf')
IMAGE_DIR = os.path.join(BASE_DIR, 'image')
config = {'namespace': 'testnamespace',
          'driver': {'name': 'color', 'data_dir': './data', 'color_weight': 0.2},
          'database': {'driver': 'sqlite3', 'name': './data/store.sqlite3'}
          }

if not os.path.exists(DATA_DIR):
    os.mkdir(DATA_DIR)

db = Otama.open(config)
db.create_table()
files = glob(os.path.join(IMAGE_DIR, '*.jpg'))
files += glob(os.path.join(IMAGE_DIR, '*.png'))
kvs = {}
for filename in files:
    kvs[db.insert(filename)] = filename

db.pull()

with open(os.path.join(DATA_DIR, 'kvs.json'), 'w') as fp:
    json.dump(kvs, fp)

db.close()
