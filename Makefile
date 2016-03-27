CFLAGS = -std=c++11
LIBRARY = -lboost_regex -lstdc++
PTH = -pthread
multidns-c : jsoncpp.cpp multidns.cpp
	cc -o multidns-c jsoncpp.cpp multidns.cpp $(LIBRARY) $(PTH) $(CFLAGS) 
install:
	cp multidns-c /usr/bin/multidns-c
clean:
	rm multidns-c
