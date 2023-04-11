test: test.cpp
	g++ -o test $^ -std=c++2a -g -lpthread 

clean:
	rm -r test