all:
	clang++ -std=c++11 -O0 -g -I/usr/local/include -levent -levent_pthreads -o test *.cpp
