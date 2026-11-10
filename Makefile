.PHONY: test test-c test-py cov clean
test: test-c test-py
test-c:
	cmake -S . -B build -DFTOTA_COVERAGE=ON >/dev/null && cmake --build build && ctest --test-dir build --output-on-failure
test-py:
	python3 -m pytest -q
cov: test-c
	cd build && gcovr -r .. --filter ../core/src --print-summary
clean:
	rm -rf build .pytest_cache .hypothesis
