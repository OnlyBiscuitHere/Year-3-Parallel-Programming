/*
* CMP3752M Parallel Programming Assignment
* FUN18673753 Benjamin Fung
* The step by step of how the code runs:
	* The initial code follows a simple route of simple reading the text file in using stringstream.
	* However, the data being used is found in the final column relating to the temperature in each location of Lincoln.
	* As, the temperature values are to a 1 decimal point, the values can be times 10 so that it can be an integer.
	* Whilst holding the percision still.
	* Following this, the OpenCL implementation can be found as compatible devices appear in the command line through a simple menu.
	* Then, the Kernel script is added to the main code along with some debugging to ensure that there are not any errors in the kernel.
	* The function to read, split and store the temperature data is called.
	* Finally, queues are made to push the values for the number of elements, the elements themselves and batch sizes.
	* These batch sizes being roughly made up of 8000 elements in each kernel as there are 1.8 million values in the larger dataset divded into 256 kernels
	* In addition to setting the values for the upper and lower bound values for the histograms being used.
	* The setup and running kernel is done by setting the arguments and then queued to the kernel.
	* The thread that has the completed result of the histograms is read and then passed to another function
	* This function then calculates the statistics of the temperature data and converts them back to the 1 decimal point percision.
	* The results are placed in a table for easier reading

* In the Kernel:
	* After everything is queued up and passed to the kernel, the kernel makes use of parallel techniques like:
		* Reduction addition
		* Global memory
		* Local memory
	* Whereby, the temperature data, the amount of data, the histogram output and the lower boundary value are passed to the kernel.
	* This information is outputted to the console to inform the user.
	* To which the kernel sets up the histograms and make sures that they are populated with 0s
	* It then iterates over the elements in each kernel, gets the index of each temperature value, ignores any values outside of the boundaries set.
	* Then waits for all the kernels to sync.
	* Following this, the first thread then adds all the histograms together resulting in a completed histogram.
	* This histogram is returned to the main.cpp file and can have statistical analysis performed on the values.

* In the end, the user can select whether they want to view the completed histogram in the simple menu or just view the statistics which are:
* The min, max, mean, standard deviation, median, 25th and 75th percentile.
*/
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include "include/Utils.h"

// Reading in data
std::vector<int> read_data_from_file(std::string& input)
{
	std::ifstream input_file(input);
	std::vector<int> data;
	for (;;)
	{
		std::string str;
		std::getline(input_file, str);
		// Checks for empty line
		if (str.size() == 0)
		{
			break;
		}
		std::string junk, temp;
		std::stringstream buffer;
		buffer << str;
		buffer >> junk >> junk >> junk >> junk >> junk >> temp;
		// Because the temperature has one decimal place it means that we can times 10 so that it can be an integer
		int int_temp = (int)(10.0 * std::stod(temp.c_str()));
		// Fill vector with temperature data
		data.push_back(int_temp);
	}
	return data;
}
// Get the OpenCL platform
int getPlatform()
{
	vector<cl::Platform> platform;
	cl::Platform::get(&platform);
	vector<cl::Device> devices;
	cl::string name;
	int platform_id = 1000;
	std::cout << "Select a platform: | Press the corresponding number and enter" << std::endl;
	for (size_t i = 0; i < platform.size(); i++)
	{
		platform[i].getInfo(CL_PLATFORM_NAME, &name);
		std::cout << i + 1 << ". " << name << std::endl;
	}
	while (platform_id == 1000)
	{
		std::cout << std::endl;
		try
		{
			int temp;
			std::cin >> temp;
			if (temp <= platform.size() && temp > 0)
			{
				platform_id = temp - 1;
				return platform_id;
			}
		}
		catch (const std::exception&)
		{
			std::cout << "An error occured" << std::endl;
		}
	}
}
// Get the OpenCL Device
int getDevice(int id)
{
	vector<cl::Platform> platform;
	cl::Platform::get(&platform);
	vector<cl::Device> device;
	platform[id].getDevices(CL_DEVICE_TYPE_ALL, &device);
	int device_id = 1000;
	std::cout << "Please select a OpenCL device: | Select the corresponding number and press enter" << std::endl;
	for (size_t i = 0; i < device.size(); i++)
	{
		std::cout << i + 1 << ". " << device[i].getInfo<CL_DEVICE_NAME>() << std::endl;
	}
	while (device_id == 1000)
	{
		std::cout << std::endl;
		try
		{
			int temp;
			std::cin >> temp;
			if (temp <= device.size() && temp > 0)
			{
				device_id = temp - 1;
				return device_id;
			}
		}
		catch (const std::exception&)
		{
			std::cout << "An error occured" << std::endl;
		}
	}
}
// Printing the available devices to the command line
void availableDevices()
{
	vector<cl::Platform> platforms;
	vector<cl::Device> devices;
	cl::Platform::get(&platforms);
	cl::string names;
	if (platforms.size() == 0)
		std::cout << "There are no OpenCL platforms found" << std::endl;
	else
		std::cout << platforms.size() << " OpenCL Platoforms found" << std::endl;
	for (size_t i = 0; i < platforms.size(); i++)
	{
		devices.clear();
		platforms[i].getInfo(CL_PLATFORM_NAME, &names);
		std::cout << "Devices available: " << names << std::endl;
		platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);
		if (devices.size() == 0)
		{
			std::cout << "There are no OpenCL platforms found" << i << std::endl;
			break;
		}
		for (size_t j = 0; j < devices.size(); j++)
		{
			std::cout << "OpenCL compatible devices: " << devices[j].getInfo<CL_DEVICE_NAME>() << std::endl;
		}
		std::cout << std::endl;
	}
}
// Calculate the minimum value
float calcMin(std::vector<int> histogram_output, int hist_value_min, int hist_size)
{
	int min = hist_value_min;
	for (int i = 0; i < hist_size; ++i)
	{
		if (histogram_output[i] > 0)
		{
			min = hist_value_min + i;
			break;
		}
	}
	float fmin = 0.1 * (float)min;
	return fmin;
}
// Calculate the maximum value
float calcMax(std::vector<int> histogram_output, int hist_value_min, int hist_value_max, int hist_size)
{
	int max = hist_value_max;
	for (int i = hist_size; i-- > 0;)
	{
		if (histogram_output[i] > 0)
		{
			max = hist_value_min + i;
			break;
		}
	}
	float fmax = 0.1 * (float)max;
	return fmax;
}
// Output the Min, max , mean, standard deviation, median, 25th percentile and 75th percentile
void printSummary(std::vector<int> histogram_output, int hist_value_min, int hist_value_max, int hist_size)
{
	// Calculate the minimum, maximum, mean, standard deviation
	float fmin = calcMin(histogram_output, hist_value_min, hist_size);
	float fmax = calcMax(histogram_output, hist_value_min, hist_value_max, hist_size);
	int sum = 0;
	unsigned long long squaresum = 0;
	int count = 0;
	for (int i = 0; i < hist_size; ++i)
	{
		int value = hist_value_min + i;
		int n = histogram_output[i];
		int x = value * n;
		int xx = value * value * n;
		sum += x;
		squaresum += xx;
		count += n;
	}
	float fsum = 0.1 * (float)sum;
	float fsquaresum = 0.01 * (float)squaresum;
	float fmean = fsum / (float)count;
	float fvariance = fsquaresum / (float)count - (fmean * fmean);
	float fstandarddev = std::sqrt(fvariance);
	int count_median = count / 2;
	int count_25_percentile = count / 4;
	int count_75_percentile = (3 * count) / 4;
	float fmedian = -1000.0;
	float f25percentile = -1000.0;
	float f75percentile = -1000.0;
	//Calculate the Median value, 25th and 75th percentile
	count = 0;
	for (int i = 0; i < histogram_output.size(); ++i)
	{
		int value = hist_value_min + i;
		int n = histogram_output[i];
		count += n;
		if (count > count_25_percentile)
		{
			if (f25percentile == -1000.0)
			{
				f25percentile = 0.1 * (float)value;
			}
		}
		if (count > count_median)
		{
			if (fmedian == -1000.0)
			{
				fmedian = 0.1 * (float)value;
			}
		}
		if (count > count_75_percentile)
		{
			if (f75percentile == -1000.0)
			{
				f75percentile = 0.1 * (float)value;
			}
		}
	}
	// print the results
	std::cout << "| Min: " << fmin;
	std::cout << " | Max: " << fmax;
	std::cout << " | Mean: " << fmean;
	std::cout << " | Standard deviation: " << fstandarddev << " | " << std::endl;
	std::cout << "| Median: " << fmedian;
	std::cout << " | 25th percentile: " << f25percentile;
	std::cout << " | 75th percentile: " << f75percentile;
	std::cout << " | " << std::endl;
}
void printHist(std::vector<int> histogram_output, int hist_size)
{
	std::cout << "Values in histogram: " << std::endl;
	for (int i = 0; i < hist_size; ++i)
	{
		std::cout << "i=" << i << " " << histogram_output[i] << "  " << std::endl;
	}
	std::cout << std::endl;
}
int main(int argc, char** argv)
{
	// Show the available OpenCL compatible devices and allows the user to select them
	availableDevices();
	int platform_id = getPlatform();
	int device_id = getDevice(platform_id);
	try
	{
		cl::Context context = GetContext(platform_id, device_id);
		cl::Device device = context.getInfo<CL_CONTEXT_DEVICES>()[0];
		std::cout << "Runinng on " << GetPlatformName(platform_id) << ", " << GetDeviceName(platform_id, device_id) << std::endl;
		cl::CommandQueue queue(context, device);
		queue.finish();
		cl::Program::Sources sources;
		AddSources(sources, "kernels/kernel.cl");
		cl::Program program(context, sources);
		// Build and debug Kernel Code
		try
		{
			program.build();
		}
		catch (const cl::Error& err)
		{
			std::cout << "There was an error building the OpenCL program" << std::endl;
			std::cout << "Build Status:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Options:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Log:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			throw err;
		}
		// Reading in the data
		std::string input_filename;
		std::cout << "Select the full file or the smaller file | Enter 1 for the larger dataset | Enter 2 for the smaller dataset" << std::endl;
		int set;
		std::cin >> set;
		if (set == 1)
		{
			input_filename = "./temp_lincolnshire_datasets/temp_lincolnshire.txt";
			std::cout << "This file might take a few minutes reading in " << std::endl;
		}
		else
			input_filename = "./temp_lincolnshire_datasets/temp_lincolnshire_short.txt";
		std::vector<int> input_data;
		std::cout << "Reading data in..." << std::endl;
		input_data = read_data_from_file(input_filename);
		// 1.8 M (In the large dataset) / 256 kernels = approx 8k elements per kernel
		const int BATCH_SIZE = 8192;
		std::cout << "BATCH_SIZE: " << BATCH_SIZE << std::endl;
		// Setting a minimum number of batches
		int batch_count = (input_data.size() + BATCH_SIZE - 1) / BATCH_SIZE;
		std::cout << "batch_count: " << batch_count << std::endl;
		// Cl buffer to store the input - same for all kernels
		cl::Buffer clbuffer_input_data(context, CL_MEM_READ_ONLY, input_data.size() * sizeof(int));
		queue.enqueueWriteBuffer(clbuffer_input_data, CL_TRUE, 0, input_data.size() * sizeof(int), &(input_data[0]));
		// Cl buffer to store the input size - same for all kernels
		int input_data_size = input_data.size();
		cl::Buffer clbuffer_input_data_size(context, CL_MEM_READ_ONLY, sizeof(int));
		queue.enqueueWriteBuffer(clbuffer_input_data_size, CL_TRUE, 0, sizeof(int), &input_data_size);
		// Cl buffer to store the batch size - same for all kernels
		cl::Buffer clbuffer_batch_size(context, CL_MEM_READ_ONLY, sizeof(int));
		queue.enqueueWriteBuffer(clbuffer_batch_size, CL_TRUE, 0, sizeof(int), &BATCH_SIZE);
		// Launch the OpenCL kernel to count the number of occurances of each element and put them in a histogram
		int hist_value_min = -500; // -50 degrees * 10
		int hist_value_max = +500; // +50 degrees * 10
		int hist_size = (hist_value_max - hist_value_min + 1); // Number of Values
		// Vector to store the output from minimum kernel
		std::vector<int> histogram_output;
		histogram_output.resize(hist_size * batch_count, 0);
		// Setup buffers
		// Histogram kernel output
		cl::Buffer clbuffer_histogram_output(context, CL_MEM_WRITE_ONLY, hist_size * batch_count * sizeof(int));
		// Histogram size
		cl::Buffer clbuffer_histogram_size(context, CL_MEM_WRITE_ONLY, sizeof(int));
		queue.enqueueWriteBuffer(clbuffer_histogram_size, CL_TRUE, 0, sizeof(int), &hist_size);
		// Lower boundary value
		cl::Buffer clbuffer_histogram_lowerbound(context, CL_MEM_WRITE_ONLY, sizeof(int));
		queue.enqueueWriteBuffer(clbuffer_histogram_lowerbound, CL_TRUE, 0, sizeof(int), &hist_value_min);
		// Setup and run kernel
		cl::Kernel histogramer_kernel = cl::Kernel(program, "histogramer_kernel");
		histogramer_kernel.setArg(0, clbuffer_input_data);
		histogramer_kernel.setArg(1, clbuffer_input_data_size);
		histogramer_kernel.setArg(2, clbuffer_batch_size);
		histogramer_kernel.setArg(3, clbuffer_histogram_output);
		histogramer_kernel.setArg(4, clbuffer_histogram_size);
		histogramer_kernel.setArg(5, clbuffer_histogram_lowerbound);
		queue.enqueueNDRangeKernel(histogramer_kernel, cl::NullRange, cl::NDRange(batch_count), cl::NullRange);
		queue.finish();
		// Read the 0th Kernel which has the completed histogram
		queue.enqueueReadBuffer(clbuffer_histogram_output, CL_TRUE, 0, sizeof(int) * hist_size, &(histogram_output[0]));
		std::cout << "Would you like to see the completed histogram? | Enter 1 for yes | Enter 2 to go straight to the summary" << std::endl;
		int option;
		std::cin >> option;
		if (option == 1)
			// Return the histogram with all the values
			printHist(histogram_output, hist_size);
		else
			// Summary of the min, max, mean, standard deviation, median, 25th and 75th percentile
			printSummary(histogram_output, hist_value_min, hist_value_max, hist_size);
	}
	catch (cl::Error err)
	{
		std::cerr << "ERROR: " << err.what() << ", " << getErrorString(err.err()) << std::endl;
	}
	return  0;
}