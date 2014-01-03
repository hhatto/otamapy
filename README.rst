otamapy
=======

.. image:: https://travis-ci.org/hhatto/otamapy.png?branch=master
    :target: https://travis-ci.org/hhatto/otamapy
    :alt: Build status

About
-----
otamapy is Python Interface for otama_ (otama is CBIR.).

.. _otama: https://github.com/nagadomi/otama
.. _nv: https://github.com/nagadomi/nv
.. _eiio: https://github.com/nagadomi/eiio


Installation
------------
from pip::

    $ pip install --upgrade otamapy

from easy_install::

    $ easy_install -ZU otamapy


Requirements
------------
* Python2.7+ and Python3.3+
* otama library (otama_, nv_, eiio_)

Usage
-----

config file (example.conf)

.. code-block:: text

    {
        'namespace': 'testnamespace',
        'driver': {'name': 'color', 'data_dir': './data', 'color_weight': 0.2},
        'database': {'driver': 'sqlite3', 'name': './data/store.sqlite3'}
    }

store to database, and search from database.

.. code-block:: python

    # store_and_search.py
    from otama import Otama
    db = Otama.open('example.conf')

    kvs = {}
    db.create_table()
    for filename in ('foo.jpg', 'bar.jpg'):
        kvs[db.insert(filename)] = filename

    for result in db.search(10, 'foo.jpg')
        key = result['id']
        print("sim=%.3f, file=%s" % (result['similarity'], kvs[key]))

.. code-block:: text

    $ python store_and_search.py
    sim=1.000, file=foo.jpg
    sim=0.969, file=bar.jpg

see examples_ .

.. _examples: https://github.com/hhatto/otamapy/tree/master/examples

Links
-----
* PyPI_
* GitHub_

.. _PyPI: http://pypi.python.org/pypi/otamapy/
.. _GitHub: https://github.com/hhatto/otamapy

