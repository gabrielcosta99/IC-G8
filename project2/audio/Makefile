

t1: t1.cpp
	g++ -Wall $< -o $@ -lsndfile

test_t1: test_t1.cpp
	g++ -Wall $< -o $@ -lsndfile


t2: t2.cpp
	g++ -Wall $< -o $@ -lsndfile

t2_test: t2_test.cpp
	g++ -Wall $< -o $@ -lsndfile


test: test.cpp
	g++ -Wall $< -o $@ -lsndfile

# t3: t3.cpp
# 	g++ -Wall $< -o $@ -lsndfile

# t4: t4.cpp
# 	g++ -Wall $< -o $@ -lsndfile

# t5: t5.cpp
# 	g++ -Wall $< -o $@ -lsndfile

clean:
	rm -f t1 t2 test t2_test
	rm -f error.bin inter_error.bin error_lossy.bin
	rm -f reconstructed.wav reconstructed_inter.wav reconstructed_lossy.wav
	
# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

