sudo rm -rf test_cases*
./scripts/build.sh

python scripts/gen_test_dirs.py

./build/libArchStatic pack test_cases test_cases.tar.gz
./build/libArchStatic unpack test_cases.tar.gz test_cases_unpacked
./build/libArchStatic pack test_cases test_cases_unpacked.tar.gz

python scripts/verify_unpack.py test_cases test_cases_unpacked/test_cases
