BitStreamTest:	BitStreamTest.cpp BitStream.h Golomb.h
	g++ -Wall -O2 BitStreamTest.cpp -o BitStreamTest

clean:
	rm -f BitStreamTest
	rm -f encoder
	rm -f output.bin
	rm -f decoder

coder:	coder.cpp BitStream.h
	g++ -Wall -O2 coder.cpp -o coder

golomb: golomb_tester.cpp Golomb.h
	g++ -o golomb_tester golomb_tester.cpp
