

t1: t1.cpp
	g++ -Wall $< -o $@ 


t2: t2.cpp
	g++ -Wall $< -o $@ 


t3: t3.cpp
	g++ -Wall $< -o $@ 

t4: t4.cpp
	g++ -Wall $< -o $@ 

clean:
	rm -f t1
	rm -f t2
	rm -f t3
	rm -f t4

# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

