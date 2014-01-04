try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension
from distutils.sysconfig import get_python_inc
import os

include_dirs = [get_python_inc()]
library_dirs = ['/usr/local/lib']
exec(open('otama/_version.py').read())

setup(name='otamapy',
      version=__version__,
      description="otamapy is Python Interface for otama.",
      long_description=open('README.rst').read(),
      author='Hideo Hattori',
      author_email='hhatto.jp@gmail.com',
      url='https://github.com/hhatto/otamapy',
      license='GPLv3',
      platforms='Linux',
      packages=['otama'],
      ext_modules=[
          Extension('otama.otama',
                    sources=['./otama/otama.c'],
                    include_dirs=include_dirs,
                    library_dirs=library_dirs,
                    libraries=['otama'],
                    #extra_compile_args=["-DDEBUG"],
                    )],
      classifiers=[
          'Development Status :: 4 - Beta',
          'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
          'Operating System :: POSIX :: Linux',
          'Programming Language :: C',
          'Programming Language :: Python',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 3'],
      keywords="otama, CBIR",
      zip_safe=False,
      )
