try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension
from distutils.sysconfig import get_python_inc
import os

include_dirs = [get_python_inc()]
library_dirs = ['/usr/local/lib']

setup(name='otamapy',
      version="0.1",
      description="otamapy is Python Interface for otama.",
      long_description=open('README.rst').read(),
      author='Hideo Hattori',
      author_email='hhatto.jp@gmail.com',
      url='https://github.com/hhatto/otamapy',
      license='GPLv3',
      platforms='Linux',
      packages=['otamapy'],
      ext_modules=[
          Extension('otama',
                    sources=['./otamapy/otama.c'],
                    include_dirs=include_dirs,
                    library_dirs=library_dirs,
                    libraries=['otama'],
                    #extra_compile_args=["-DDEBUG"],
                    )],
      classifiers=[
          'Development Status :: 3 - Alpha',
          'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
          'Operating System :: POSIX :: Linux',
          'Programming Language :: C',
          'Programming Language :: Python'],
      keywords="otama",
      zip_safe=True,
      )
