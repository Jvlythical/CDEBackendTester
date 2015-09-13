make clean 2&> /dev/null
make vm > /dev/null
./vm -t 100 apps/mutex.so
make clean 2&> /dev/null
