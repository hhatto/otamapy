
all:
	python setup.py build

test:
	make clean;
	easy_install -ZU .
	cd examples && python store.py && python search.py

clean:
	rm -rf build dist *.egg-info temp
