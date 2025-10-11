verify: build
	sudo ldconfig -v 2>/dev/null | grep tracelib
	python -c "import tracelibpy"

build: activate
	cd build
	../cmake-4.1.1-linux-x86_64/bin/cmake -DINCLUDE_PYTHON_BINDINGS=ON -DINCLUDE_EXAMPLE_PROGRAMS=ON
	make -j
	sudo make install
	sudo ldconfig

activate:
	. ../tracelib-venv/bin/activate

deactivate:
	deactivate
