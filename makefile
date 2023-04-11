test: test.cpp
	g++ -o test $^ -g -lpthread

clean:
	rm -r test