t1: t1.cpp
	g++ -Wall $< -o $@ 

# in this exercise, we need to install boost_locale library, using "sudo apt-get install libboost-all-dev"
t2: t2.cpp
	g++ -Wall $< -o $@ -lboost_locale

t3: t3.cpp
	g++ -Wall $< -o $@  -I/usr/include/python3.10 -lpython3.10 -lboost_locale

t4: t4.cpp
	g++ -Wall $< -o $@ -I/usr/include/python3.10 -lpython3.10 -lboost_locale

clean:
	rm -f t1
	rm -f t2
	rm -f t3
	rm -f t4

# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

