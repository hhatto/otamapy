import os
import json
from otama import Otama

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
TARGET_FILE1 = os.path.join(BASE_DIR, 'image/lena.jpg')
TARGET_FILE2 = os.path.join(BASE_DIR, 'image/lena-affine.jpg')
CONFIG_FILE = os.path.join(BASE_DIR, 'test.conf')
DATA_DIR = os.path.join(BASE_DIR, 'data')


db = Otama({'driver': {'name': 'vlad_nodb'}})
print(db.similarity({'data': open(TARGET_FILE1, 'rb').read()},
                    {'data': open(TARGET_FILE2, 'rb').read()}))

fv = db.feature_raw({'file': TARGET_FILE1})
print(fv)
print(db.similarity({'raw': fv}, {'file': TARGET_FILE1}))
print(db.similarity({'raw': fv}, {'file': TARGET_FILE2}))
print(db.similarity({'raw': fv}, {'file': str(TARGET_FILE2)}))

fv.dispose()
db.close()
