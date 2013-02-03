from otama import Otama

config = {'namespace': 'testnamespace',
          'driver': {'name': 'color', 'data_dir': './data', 'color_weight': 0.2},
          'database': {'driver': 'sqlite3', 'name': './data/store.sqlite3'}
          }
o = Otama()
print o
o.open(config)
idb = Otama.open(config)
