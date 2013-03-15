import time
import os.path
from otama import Otama

BASEPATH = os.path.abspath(os.path.dirname(__file__))
print(BASEPATH)

while True:
    db = Otama(os.path.join(BASEPATH, 'test.conf'))
    db.search(5, os.path.join(BASEPATH, 'image/baboon.png'))
    #db.close()
    #time.sleep(0.1)
