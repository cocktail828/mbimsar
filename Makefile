CPPFLAGS := -Wall -g -fPIC -DNDEBUG -std=c++11

objects := $(patsubst %.cpp, %.o, $(wildcard *.cpp))
Mbim2Sar: clean $(objects)
	$(CXX) -o $@ $(objects) -lpthread

clean:
	rm -rf *.o Mbim2Sar