test: test.cpp
	g++  -g -o test $^ -std=c++2a -lpthread 

clean:
	rm -r test