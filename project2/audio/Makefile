

# t1: t1.cpp
# 	g++ -Wall $< -o $@ -lsndfile


t2: t2.cpp
	g++ -Wall $< -o $@ -lsndfile


main: main.cpp t2.cpp
	g++ -Wall $< -o $@ -lsndfile

clean:
	rm -f t2 main
	rm -f error.bin inter_error.bin error_lossy.bin
	rm -f reconstructed.wav reconstructed_inter.wav reconstructed_lossy.wav
	
# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

