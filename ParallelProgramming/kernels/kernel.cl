// The kernel for the main.cpp
kernel void histogramer_kernel(
	global const int *data_in,
	global int *data_size,
	global int *batch_size,
	global int *histogram_data,
	global int *histogram_size,
	global int *histogram_lowerbound
)
{
	// Variable initialisers
	int bsize = *batch_size; // Temperature data * 10
	int dsize = *data_size; // Number of vector elements
	int hsize = *histogram_size; // Histogram output for each kernel
	int hlow = *histogram_lowerbound; // The lowerbound of the histogram which is set -500
	int id = get_global_id(0); 
	if(id == 0)
	{
		printf("Kernel Info:");
		printf("bsize=%d, dsize=%d, hsize=%d, hlow=%d\n", bsize, dsize, hsize, hlow);
	}
	// Initialising the histograms 
	for(int i = 0; i < hsize; ++ i)
	{
		int histogram_index = id * hsize + i;
		histogram_data[histogram_index] = 0;
	}
	// Iterate over the number of elements in each kernel
	for(int i = 0; i < bsize; ++ i)
	{
		// Get the index of temperature data
		int index = id * bsize + i;
		if(index < dsize)
		{
			// Populate the histogram
			int temp = data_in[index];
			if(temp < hlow) continue; // Ignoring the values of out range
			if(temp > hlow + hsize) continue;
			int histogram_index = id * hsize + (temp - hlow);
			++ histogram_data[histogram_index]; // Assuming the range is correct the values between are added to histogram
		}
	}
	// Wait for sync
	barrier(CLK_GLOBAL_MEM_FENCE);
	if(id == 0)
	{
		for(int i = 0; i < hsize; ++ i)
		{
			int histogram_index = id * hsize + i;
			int value = histogram_data[histogram_index];
		}
	}
	// Adding the histograms but only in thread 0
	if(id == 0)
	{
		for(int i = 0; i < hsize; ++ i)
		{
			for(int k = 1; k < get_global_size(0); ++ k)
			{
				histogram_data[i] += histogram_data[k * hsize + i];
			}
		}
	}
	if(id == 0)
	{
		for(int i = 0; i < hsize; ++ i)
		{
			int histogram_index = id * hsize + i;
			int value = histogram_data[histogram_index];
		}
	}
	// The first histogram holds the complete histogram
}
