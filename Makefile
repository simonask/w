all:
	clang++ -std=c++11 -O0 -g -I/usr/local/include -I. -levent -levent_pthreads -lpq -o test `find . -name "*.cpp"`
