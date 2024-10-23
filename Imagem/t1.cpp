#include <iostream>
#include <opencv2/opencv.hpp>
#include <cxxopts.hpp>
#include <cmath>  // For log10
#include <chrono> // For timers

using namespace cv;
using namespace std;
using namespace std::chrono;



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
    auto start = high_resolution_clock::now(); // Start timer
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

    auto end = high_resolution_clock::now(); // End timer
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Time taken to calculate the histogram " << duration.count() << " ms" << endl;

}

// Function to apply Gaussian blur with different kernel sizes
void applyGaussianBlur(const Mat& image, int size = 3)
{
    auto start = high_resolution_clock::now(); // Start timer
    Mat blurredImage;
    GaussianBlur(image, blurredImage, Size(size, size), 0);
    displayImagesSideBySide(image,blurredImage,"Original image/Gaussian Blur");
    auto end = high_resolution_clock::now(); // End timer
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Time taken to calculate gaussian blur: " << duration.count() << " ms" << endl;

}
void RGB_applyGaussianBlur(const Mat& image, int size = 3)
{
    auto start = high_resolution_clock::now(); // Start timer
    Mat blurredImage;
    GaussianBlur(image, blurredImage, Size(size, size), 0);
    displayImagesSideBySide(image,blurredImage,"Original image/Gaussian Blur");
    auto end = high_resolution_clock::now(); // End timer
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Time taken to calculate gaussian blur: " << duration.count() << " ms" << endl;

}

// Function to display the difference image
void displayDifferenceImage(const Mat& diffImage)
{
    Mat diffGray;
    //cvtColor(diffImage, diffGray, COLOR_BGR2GRAY);  // Convert to grayscale if needed

    imshow("Difference Image", diffGray);
}

// Function to compute the absolute difference between two images
Mat calculateAbsoluteDifference(const Mat& image1, const Mat& image2)
{

    
    Mat diff;
    absdiff(image1, image2, diff);
    return diff;
}


// Function to calculate the Mean Squared Error (MSE)
double computeMSE(const Mat& image1, const Mat& image2)
{   

    
    Mat diff;
    absdiff(image1, image2, diff);  // Get the difference image
    diff.convertTo(diff, CV_32F);   // Convert to float for precision

    // Square the differences and compute the mean
    diff = diff.mul(diff);          // Square each element
    Scalar sum = cv::sum(diff);     // Sum all the squared differences

    double mse = sum[0] / (double)(image1.total());


    return mse;
}

// Function to calculate the Peak Signal-to-Noise Ratio (PSNR)
double computePSNR(const Mat& image1, const Mat& image2)
{   

    double mse = computeMSE(image1, image2);
    if (mse == 0)
    {
        return std::numeric_limits<double>::infinity();  // Infinite PSNR means no difference
    }
    double psnr = 10.0 * log10((255 * 255) / mse);

    return psnr;
}


// Function to quantize a grayscale image with a specified number of levels
Mat quantizeImage(const Mat& gray_image, int num_levels)
{
    auto start = high_resolution_clock::now(); // Start timer
    Mat quantized_image = gray_image.clone(); // Clone the input image to preserve the original
    int step = 256 / num_levels;  // Calculate the step size based on the number of levels

    // Iterate over each pixel and apply quantization
    for (int i = 0; i < gray_image.rows; i++)
    {
        for (int j = 0; j < gray_image.cols; j++)
        {
            // Get the pixel value
            int pixel_value = gray_image.at<uchar>(i, j);

            // Quantize the pixel value
            int quantized_value = (pixel_value / step) * step;

            // Set the quantized value in the output image
            quantized_image.at<uchar>(i, j) = quantized_value;
        }
    }

    auto end = high_resolution_clock::now(); // End timer
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Time taken to quantize the image " << duration.count() << " ms" << endl;

    return quantized_image;
}


// Function to quantize an RGB image with a specified number of levels
Mat  RGB_quantizeImage(const Mat& rgb_image, int num_levels)
{
    auto start = high_resolution_clock::now(); // Start timer

    // Split the image into 3 channels (B, G, R)
    Mat bgr[3];
    split(rgb_image, bgr);

    // Apply quantization to each channel
    for (int k = 0; k < 3; k++) {
        int step = 256 / num_levels;  // Calculate the step size based on the number of levels

        for (int i = 0; i < bgr[k].rows; i++)
        {
            for (int j = 0; j < bgr[k].cols; j++)
            {
                int pixel_value = bgr[k].at<uchar>(i, j);

                // Quantize the pixel value
                int quantized_value = (pixel_value / step) * step;

                // Set the quantized value in the output channel
                bgr[k].at<uchar>(i, j) = quantized_value;
            }
        }
    }

    // Merge the quantized channels back into one image
    Mat quantized_image;
    merge(bgr, 3, quantized_image);

    auto end = high_resolution_clock::now(); // End timer
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Time taken to quantize the image " << duration.count() << " ms" << endl;

    return quantized_image;
}

// Function to calculate the Mean Squared Error (MSE) for RGB images
double  RGB_computeMSE(const Mat& image1, const Mat& image2)
{
    // Split the images into 3 channels (B, G, R)
    Mat bgr1[3], bgr2[3];
    split(image1, bgr1);
    split(image2, bgr2);

    double mse_total = 0.0;
    // Calculate MSE for each channel and sum it
    for (int k = 0; k < 3; k++)
    {
        Mat diff;
        absdiff(bgr1[k], bgr2[k], diff);  // Get the difference image for channel k
        diff.convertTo(diff, CV_32F);     // Convert to float for precision

        diff = diff.mul(diff);            // Square the differences
        Scalar sum = cv::sum(diff);       // Sum all the squared differences

        double mse_channel = sum[0] / (double)(bgr1[k].total());
        mse_total += mse_channel;         // Sum the MSE of each channel
    }

    // Average MSE across the three channels
    return mse_total / 3.0;
}

// Function to calculate the Peak Signal-to-Noise Ratio (PSNR) for RGB images
double RGB_computePSNR(const Mat& image1, const Mat& image2)
{
    double mse = computeMSE(image1, image2);
    if (mse == 0)
    {
        return std::numeric_limits<double>::infinity();  // Infinite PSNR means no difference
    }
    double psnr = 10.0 * log10((255 * 255) / mse);
    return psnr;
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
            ("gf")
            ("diff", "Compute absolute difference between two images")
            ("split", "Split the image into RGB channels")
            ("q", "Quantize the image to a specified number of levels", cxxopts::value<int>()->default_value("16")->implicit_value("16"))
            ("qrgb", "Quantize the RGB image to a specified number of levels", cxxopts::value<int>()->default_value("16")->implicit_value("16"))
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

        }else  if (result.count("i2"))
        {
            string image2_path = result["i2"].as<string>();
            Mat image2 = imread(image2_path, IMREAD_COLOR); // Read the second image

            if (!image2.data)
            {
                cout << "No image data for the second image." << endl;
                return -1;
            }

            // If 'diff' option is selected, calculate and display differences
            if (result.count("diff"))
            {

                Mat resizedImage2;
                // Resize the second image to match the size of the first image
                if (image.size() != image2.size())
                {
                    cout << "Resizing second image to match the first image size." << endl;
                    resize(image2, resizedImage2, image.size());
                }
                else
                {
                    resizedImage2 = image2;
                }

                Mat diffImage = calculateAbsoluteDifference(image, resizedImage2);
                imshow("Difference Image", diffImage);

                // Compute MSE and PSNR
                double mse = computeMSE(image, resizedImage2);
                double psnr = computePSNR(image, resizedImage2);

                // Output the results to the console
                cout << "MSE: " << mse << endl;
                cout << "PSNR: " << psnr << " dB" << endl;
            }
        }else if (result.count("split"))
        {
            // Split the image into RGB channels
            Mat bgr[3];
            split(image, bgr);

            // Display the individual channels
            imshow("Blue Channel", bgr[0]);
            imshow("Green Channel", bgr[1]);
            imshow("Red Channel", bgr[2]);
        
        }else if(result.count("q"))
        {
            int num_levels = result["q"].as<int>();
            Mat quantized_image = quantizeImage(gray_image, num_levels);
            
            // Display original and quantized images
            displayImagesSideBySide(gray_image, quantized_image, "Original vs Quantized");

            // display image difference
            Mat diff;
            absdiff(gray_image, quantized_image, diff);
            imshow("Difference Image", diff);

            // Compute and display MSE and PSNR
            double mse = computeMSE(gray_image, quantized_image);
            double psnr = computePSNR(gray_image, quantized_image);

            cout << "Quantization Levels: " << num_levels << endl;
            cout << "MSE: " << mse << endl;
            cout << "PSNR: " << psnr << " dB" << endl;
        }else if("qrgb")
        {
            int num_levels = result["qrgb"].as<int>();
            Mat quantized_image = RGB_quantizeImage(image, num_levels);
            
            // Display original and quantized images
            displayImagesSideBySide(image, quantized_image, "Original vs Quantized");

            // display image difference
            Mat diff;
            absdiff(image, quantized_image, diff);
            imshow("Difference Image", diff);

            // Compute and display MSE and PSNR
            double mse = RGB_computeMSE(image, quantized_image);
            double psnr = RGB_computePSNR(image, quantized_image);

            cout << "Quantization Levels: " << num_levels << endl;
            cout << "MSE: " << mse << endl;
            cout << "PSNR: " << psnr << " dB" << endl;
        }else{
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
