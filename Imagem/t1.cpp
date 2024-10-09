#include <iostream>
#include <opencv2/opencv.hpp>
#include <cxxopts.hpp>

using namespace cv;
using namespace std;



// Function to display two images side by side in a single window
void displayImagesSideBySide(const Mat& image1, const Mat& image2, const string& windowName)
{
    // Ensure both images have the same number of channels
    Mat img1, img2;
    if (image1.channels() == 1)
    {
        cvtColor(image1, img1, COLOR_GRAY2BGR);  // Convert grayscale to BGR
    }
    else
    {
        img1 = image1;
    }

    if (image2.channels() == 1)
    {
        cvtColor(image2, img2, COLOR_GRAY2BGR);  // Convert grayscale to BGR
    }
    else
    {
        img2 = image2;
    }

    // Create a larger canvas to hold both images side by side
    int width = img1.cols + img2.cols;
    int height = max(img1.rows, img2.rows);
    Mat canvas(height, width, CV_8UC3, Scalar(255, 255, 255));

    // Copy each image into the appropriate place in the canvas
    img1.copyTo(canvas(Rect(0, 0, img1.cols, img1.rows)));
    img2.copyTo(canvas(Rect(img1.cols, 0, img2.cols, img2.rows)));

    // Display the combined image
    imshow(windowName, canvas);
}

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



    displayImagesSideBySide(gray_image,histImage,"Hist");

   //// Display the histogram
   //imshow("Grayscale Histogram", histImage);
   //// display the image
   //imshow("Grayscale image", gray_image);
}

// Function to apply Gaussian blur with different kernel sizes
void applyGaussianBlur(const Mat& image, int size = 3)
{
    Mat blurredImage;
    GaussianBlur(image, blurredImage, Size(size, size), 0);
    displayImagesSideBySide(image,blurredImage,"Original image/Gaussian Blur");
    //imshow("Original image", image);
    //imshow("Gaussian Blur", blurredImage);
}

// Function to display the difference image
void displayDifferenceImage(const Mat& diffImage)
{
    Mat diffGray;
    cvtColor(diffImage, diffGray, COLOR_BGR2GRAY);  // Convert to grayscale if needed

    // Scale the difference image for better visibility (optional)
    Mat diffEnhanced;
    normalize(diffGray, diffEnhanced, 0, 255, NORM_MINMAX);

    imshow("Difference Image (Enhanced)", diffEnhanced);
}

// Function to compute the absolute difference between two images
Mat calculateAbsoluteDifference(const Mat& image1, const Mat& image2)
{
    Mat diff;
    absdiff(image1, image2, diff);  // Calculate absolute difference between image1 and image2
    return diff;
}


int main(int argc, char** argv)
{
    try
    {
        cxxopts::Options options("DisplayImage", "A program to display images, apply Gaussian blur, and display histograms.");

        options
        .add_options()
            ("i,image", "Path to the image file", cxxopts::value<string>())
            ("i2,image2", "Path to the second image file (for comparison)", cxxopts::value<string>())
            ("gsh", "Display grayscale histogram")
            ("gf", "Apply Gaussian blur with specified kernel size", cxxopts::value<int>()->default_value("3")->implicit_value("3"))
            ("diff", "Compute absolute difference between two images and display MSE & PSNR")
            ("h,help", "Print usage");

        auto result = options.parse(argc, argv);

        if (result.count("help"))
        {
            cout << options.help() << endl;
            return 0;
        }

        if (!result.count("image"))
        {
            cout << "Please provide an image path using --image option." << endl;
            return -1;
        }

        string image_path = result["image"].as<string>();
        Mat image = imread(image_path, IMREAD_COLOR); // Read the file

        if (!image.data)
        {
            
            cout << "No image data." << endl;
            return -1;
        }

        // Convert image to gray
        Mat gray_image;
        cvtColor(image, gray_image, COLOR_BGR2GRAY);

        // Check for options
        if (result.count("gsh"))
        {
            drawGrayscaleHistogram(gray_image, 1024, 400, 2);
        }
        else if (result.count("gf"))
        {
            int kernel_size = result["gf"].as<int>();
            cout << "Applying Gaussian Blur with kernel size: " << kernel_size << endl;
            applyGaussianBlur(image, kernel_size);

        }else  if (result.count("image2"))
        {
            string image2_path = result["image2"].as<string>();
            Mat image2 = imread(image2_path, IMREAD_COLOR); // Read the second image

            if (!image2.data)
            {
                cout << "No image data for the second image." << endl;
                return -1;
            }

            // If 'diff' option is selected, calculate and display differences
            if (result.count("diff"))
            {
                Mat diffImage = calculateAbsoluteDifference(image, image2);
                displayDifferenceImage(diffImage);

                //double mse = computeMSE(image1, image2);
                //double psnr = computePSNR(image1, image2);
                //cout << "MSE: " << mse << endl;
                //cout << "PSNR: " << psnr << " dB" << endl;
            }
        }else {
            imshow("Original Image", image);
        }

        waitKey(0);
    }catch (const cxxopts::exceptions::exception& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
