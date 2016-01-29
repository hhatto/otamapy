import os
import json
from glob import glob
from otama import Otama


BASE_DIR = os.path.abspath(os.path.dirname(__file__))
DATA_DIR = os.path.join(BASE_DIR, 'data')
IMAGE_DIR = os.path.join(BASE_DIR, 'image')
TARGET_FILE = os.path.join(BASE_DIR, 'image/lena.jpg')
config = {'namespace': 'testnamespace',
          'driver': {'name': 'color', 'data_dir': DATA_DIR, 'color_weight': 0.2},
          'database': {'driver': 'sqlite3',
                       'name': os.path.join(DATA_DIR, 'store.sqlite3')}
          }

if not os.path.exists(DATA_DIR):
    os.mkdir(DATA_DIR)

db = Otama.open(config)
db.create_database()
files = glob(os.path.join(IMAGE_DIR, '*.jpg'))
files += glob(os.path.join(IMAGE_DIR, '*.png'))
kvs = {}
for filename in files:
    kvs[db.insert(filename)] = filename

db.pull()

with open(os.path.join(DATA_DIR, 'kvs.json'), 'w') as fp:
    json.dump(kvs, fp)

print(db.search(2, TARGET_FILE))
db.close()
