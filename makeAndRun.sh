set -e
#cmake . -DRUN_IN_PLACE=TRUE
make -j16
./bin/luanti --worldname aa --go
