
all:
	python setup.py build

.PHONY: test

test:
	make clean;
	easy_install -ZU .
	cd examples && python store.py && python search.py && python remove_and_search.py

pypireg:
	python setup.py register
	python setup.py sdist upload

clean:
	rm -rf build dist *.egg-info temp setup.cfg
	rm -f */*.pyc
