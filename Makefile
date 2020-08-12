CPPFLAGS := -Wall -g -fPIC

objects := $(patsubst %.cpp, %.o, $(wildcard *.cpp))
Mbim2Sar: clean $(objects)
	$(CXX) -shared -fPIC -o $@.so $(objects) -lpthread

clean:
	rm -rf *.o Mbim2Sar