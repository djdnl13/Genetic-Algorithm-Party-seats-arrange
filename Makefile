all: main.cpp
	$(CXX) --std=c++11 main.cpp -o main -lutil -L/usr/local/lib -lboost_iostreams -lboost_system -lboost_filesystem -Wno-cpp

clean:
	rm main
