all: convertorApp testApp


convertorApp: convertor/ConvertorApp.cpp convertor/Convertor.hpp convertor/Convertor.cpp BlueBinXml.hpp 
	g++ -Wall -pedantic -O2 -std=c++20 -o convertor/convertor convertor/ConvertorApp.cpp convertor/Convertor.cpp


testApp: tests/Tests.cpp convertor/Convertor.hpp convertor/Convertor.cpp BlueBinXml.hpp 
	g++ -Wall -pedantic -O2 -std=c++20 -o tests/tests tests/Tests.cpp convertor/Convertor.cpp

test: testApp
	./tests/tests

clean:
	rm -f convertor/convertor
	rm -f tests/tests