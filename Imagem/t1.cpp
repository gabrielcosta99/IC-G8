#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;



int main(int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    Mat image;
    image = imread( argv[1], IMREAD_COLOR ); // Read the file

    if ( !image.data )
    {
        printf("No image data \n");
        return -1;
    }
    //namedWindow("Display Image", WINDOW_AUTOSIZE );

    // split the image into its 3 channels
    Mat bgr[3];   //destination array
    split(image,bgr);//split source

    //convert image to gray
    Mat gray_image;
    cvtColor(image, gray_image, COLOR_BGR2GRAY);

    // calculate the histogram pixel values for gray image
    int histSize = 256;    // bin size
    float range[] = { 0, 256 }; // range of pixel values
    const float* histRange = { range };
    bool uniform = true, accumulate = false;
    Mat hist;
    calcHist(&gray_image, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);


    // Visualize the histogram to analyse the distribution of intensity values

    // draw the histograms
    int hist_w = 600, hist_h = 600;
    int bin_w = cvRound((double)hist_w / histSize);
    Mat histImage(hist_h, hist_w, CV_8UC1, Scalar(255, 255, 255));


    // normalize the histogram values to fit the image size
    //normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

    
    // display the histogram
    imshow("Histogram", histImage);

    


    // display the color channels and the gray image
    //imshow("Display Image", image);
    //imshow("Blue", bgr[0]);
    //imshow("Green", bgr[1]);
    //imshow("Red", bgr[2]);
    //imshow("Gray", gray_image); 


    waitKey(0);

    return 0;
}