all: test test_ThreadPool

test: test.cpp
	g++  -g -o test $^ -std=c++2a -lpthread 

test_ThreadPool: test_threadpool.cpp
	g++ -g -o test_ThreadPool test_threadpool.cpp -std=c++2a -lpthread

clean:
	rm -r test test_ThreadPool