import os
import json
from otama import Otama

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
TARGET_FILE1 = os.path.join(BASE_DIR, 'image/lena.jpg')
TARGET_FILE2 = os.path.join(BASE_DIR, 'image/lena-affine.jpg')
CONFIG_FILE = os.path.join(BASE_DIR, 'test.conf')
DATA_DIR = os.path.join(BASE_DIR, 'data')

kvs = None
with open(os.path.join(DATA_DIR, 'kvs.json')) as fp:
    kvs = json.load(fp)

db = Otama({'driver': {'name': 'vlad_nodb'}})
print db.similarity({'data': open(TARGET_FILE1).read()},
                    {'data': open(TARGET_FILE2).read()})

fv = db.feature_raw({'file': TARGET_FILE1})
print fv
print db.similarity({'raw': fv},
                    {'file': TARGET_FILE1})
print db.similarity({'raw': fv},
                    {'file': TARGET_FILE2})

db.close()
