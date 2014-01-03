import os
import sys
from invoke import run, task


@task
def all():
    """building a otamapy module"""
    run('python setup.py build')


@task
def test():
    """run unittest"""
    run('nosetests')


@task
def example():
    """run example code"""
    clean()
    run('pip install --upgrade .')
    os.chdir('examples')
    run('python store.py')
    run('python search.py')
    run('python remove_and_search.py')


@task
def pypireg():
    """regster to PyPI"""
    run('python setup.py register')
    run('python setup.py sdist upload')


@task
def install_libotama():
    """install to libotama"""
    if sys.platform == 'darwin':
        run('sh tools/install-libotama-for-macosx.sh')
    else:
        run('sh tools/install-libotama.sh')


@task
def clean():
    """clean development environment"""
    run('rm -rf build dist *.egg-info temp setup.cfg')
    run('rm -f */*.pyc *.pyc')
