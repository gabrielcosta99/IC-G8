

t1: t1.cpp
	g++ -Wall $< -o $@ -lsndfile

test_t1: test_t1.cpp
	g++ -Wall $< -o $@ -lsndfile


# t2: t2.cpp
# 	g++ -Wall $< -o $@ -lsndfile


# t3: t3.cpp
# 	g++ -Wall $< -o $@ -lsndfile

# t4: t4.cpp
# 	g++ -Wall $< -o $@ -lsndfile

# t5: t5.cpp
# 	g++ -Wall $< -o $@ -lsndfile

clean:
	rm -f t1
	rm -f error.bin
	rm -f inter_error.bin
	rm -f reconstructed.wav
	rm -f reconstructed_inter.wav
	
# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

