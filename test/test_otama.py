import os
import shutil
import unittest
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO
import otama
from otama import Otama

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
DATA_DIR = os.path.join(BASE_DIR, 'data')
CONFIG_FILE = os.path.join(BASE_DIR, 'test.conf')
IMAGE_DIR = os.path.join(BASE_DIR, '../example/image')
CONFIG = {
    'namespace': 'testnamespace',
    'driver': {'name': 'color', 'data_dir': DATA_DIR, 'color_weight': 0.2},
    'database': {'driver': 'sqlite3',
                 'name': os.path.join(DATA_DIR, 'store.db')}}
INVOKE_CONFIG = {
    'namespace': 'testnamespace',
    'driver': {'name': 'sim', 'data_dir': DATA_DIR, 'load_fv': "false",
               'hit_threshold': 2},
    'database': {'driver': 'sqlite3',
                 'name': os.path.join(DATA_DIR, 'store.db')}}


class TestOtama(unittest.TestCase):

    def setUp(self):
        if not os.path.exists(DATA_DIR):
            os.mkdir(DATA_DIR)
        self.otama = Otama()
        self.db = self.otama.open(CONFIG)

    def tearDown(self):
        shutil.rmtree(DATA_DIR)

    def test_open(self):
        o = Otama()
        db = o.open(CONFIG)
        self.assertEqual(type(db), Otama)
        self.assertEqual(True, os.path.exists(DATA_DIR))

    def test_close(self):
        self.assertEqual(None, self.db.close())

    def test_create_database(self):
        self.assertEqual(None, self.db.create_database())

    def test_drop_database(self):
        self.assertEqual(None, self.db.drop_database())

    def test_drop_index(self):
        self.assertEqual(None, self.db.drop_index())

    def test_vacuum_index(self):
        self.assertEqual(None, self.db.vacuum_index())

    def test_has_libotama_version_string(self):
        self.assertEqual(str, type(otama.__libotama_version__))


class TestOtamaWithLevelDB(unittest.TestCase):

    def setUp(self):
        if not os.path.exists(DATA_DIR):
            os.mkdir(DATA_DIR)
        self.otama = Otama()
        self.db = self.otama.open(INVOKE_CONFIG)

    def tearDown(self):
        shutil.rmtree(DATA_DIR)

    def test_invoke(self):
        # FIXME
        self.assertEqual(None, self.db.invoke('update_idf', 0))
