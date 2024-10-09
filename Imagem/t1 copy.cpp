#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;


// Function to draw and display a grayscale histogram
void drawGrayscaleHistogram(const Mat& gray_image, int hist_w = 512, int hist_h = 400, int gap = 1)
{
    // Calculate the histogram of the grayscale image
    int histSize = 256;    // bin size
    float range[] = { 0, 256 }; // range of pixel values
    const float* histRange = { range };
    bool uniform = true, accumulate = false;
    Mat hist;
    calcHist(&gray_image, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

    // Create an image to display the histogram
    int bin_w = cvRound((double)hist_w / histSize);
    Mat histImage(hist_h + 50, hist_w + 50, CV_8UC1, Scalar(255, 255, 255));  // Add space for labels

    // Normalize the histogram to fit the height of the image
    normalize(hist, hist, 0, hist_h, NORM_MINMAX);

    // Draw the histogram with rectangles and separation between bars
    for (int i = 0; i < histSize; i++)
    {
        rectangle(histImage,
                  Point(bin_w * i + 25, hist_h + 25),
                  Point(bin_w * (i + 1) - gap + 25, hist_h - cvRound(hist.at<float>(i)) + 25),
                  Scalar(0, 0, 0),
                  FILLED);
    }

    // Draw X and Y axis
    line(histImage, Point(25, 25), Point(25, hist_h + 25), Scalar(0, 0, 0), 2);  // Y-axis
    line(histImage, Point(25, hist_h + 25), Point(hist_w + 25, hist_h + 25), Scalar(0, 0, 0), 2);  // X-axis

    // Label X-axis (0 to 255 for pixel intensities)
    for (int i = 0; i <= 255; i += 50)
    {
        putText(histImage, std::to_string(i), Point(bin_w * i + 25, hist_h + 45), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 0), 1);
    }

    // Label Y-axis (0 to max value)
    for (int i = 0; i <= hist_h; i += 50)
    {
        putText(histImage, std::to_string(i), Point(0, hist_h - i + 25), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 0), 1);
    }

    // Display the histogram
    imshow("Grayscale Histogram", histImage);
}

// Function to apply Gaussian blur with different kernel sizes
void applyGaussianBlur(const Mat& image, int size=3)
{
    Mat blurredImage;

    // Applying Gaussian blur with different kernel sizes
    GaussianBlur(image, blurredImage, Size(size, size), 0);
    imshow("Gaussian Blur", blurredImage);

    //GaussianBlur(image, blurredImage, Size(5, 5), 0);
    //imshow("Gaussian Blur 5x5", blurredImage);
//
    //GaussianBlur(image, blurredImage, Size(7, 7), 0);
    //imshow("Gaussian Blur 7x7", blurredImage);
//
    //GaussianBlur(image, blurredImage, Size(9, 9), 0);
    //imshow("Gaussian Blur 9x9", blurredImage);
}

int main(int argc, char** argv )
{
    if ( argc < 2 )
    {
        printf("usage: DisplayImage.out <Image_Path> [options]\n");
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

    switch (argv[2])
    {

        case "-GSH":
            // Draw and display the histogram
            drawGrayscaleHistogram(gray_image, 1024, 400, 2);

        case "-G":
            int matrixSize = 3;
            if ( argc < 4 )
            {
                printf("usage: DisplayImage.out <Image_Path> -G MATIRX_SIZE\n");
                printf("No MATIRX_SIZE provided using default %d\n",matrixSize);
            }else {
                matrixSize = argv[3]
            }
            //
            applyGaussianBlur(image,matrixSize);
        default:
            printf("No option selected showing image");
            imshow("Display Image", image);
    }



    // display the color channels and the gray image
    //imshow("Display Image", image);
    //imshow("Blue", bgr[0]);
    //imshow("Green", bgr[1]);
    //imshow("Red", bgr[2]);
    //imshow("Gray", gray_image); 


    waitKey(0);

    return 0;
}