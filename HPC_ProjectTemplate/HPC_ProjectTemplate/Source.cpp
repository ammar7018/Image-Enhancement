#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <msclr\marshal_cppstd.h>
#include <ctime>
#include <mpi.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

// Global Variables
int ImageWidth = 4, ImageHeight = 4;
const int color_range = 256, new_scale = 255;

// I need the chunk size
int* local_image;

float local_PDF[color_range + 20];
float global_PDF[color_range + 20];
float CDF[color_range + 20];

int local_frequency[color_range];
int global_frequency[color_range];
int local_lookup[color_range + 20];
int final_lookup[color_range + 20];
int* UpdatedImage;

int* inputImage(int* w, int* h, System::String^ imagePath)
{
	int* input;
	int OriginalImageWidth, OriginalImageHeight;

	// Read Image into local arrayss
	System::Drawing::Bitmap BM(imagePath);
	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;

	ImageHeight = BM.Height;
	ImageWidth = BM.Width;

	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);
			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			// gray scale (RGB average)
			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3);
		}
	}
	return input;
}

void createImage(int* image, int width, int height, int index)
{

	System::Drawing::Bitmap MyNewImage(width, height);

	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			// cuttoff
			if (image[i * width + j] < 0)
				image[i * width + j] = 0;
			if (image[i * width + j] > 255)
				image[i * width + j] = 255;
			System::Drawing::Color c = System::Drawing::Color::FromArgb(
				image[i * MyNewImage.Width + j],
				image[i * MyNewImage.Width + j],
				image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "The Resulting Image is Saved " << index << endl;
}

void countFrequency(int* image,int *fullData,int chunk_size,int image_size, int rank, int world_size) {

	for (int i = 0; i < chunk_size; i++)
		local_frequency[image[i]]++;

	if (rank == 0) {
		
		int start = (world_size-1) * chunk_size;
		int end = start + chunk_size;
		//out of size of sub image 
		for (int i = end; i < image_size; i++) {
			local_frequency[fullData[i]]++;
		}
	}

	MPI_Reduce(&local_frequency, &global_frequency, color_range, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Bcast(&global_frequency, color_range, MPI_INT, 0, MPI_COMM_WORLD);
}

void calcCDF() {
	CDF[0] = global_PDF[0];
	for (int i = 1; i < color_range; i++)
		CDF[i] = CDF[i - 1] + global_PDF[i];
	MPI_Bcast(CDF, color_range, MPI_INT, 0, MPI_COMM_WORLD);
}

void calcPDF(int image_size, int rank, int world_size) {

	for (int i = 0; i < color_range; i++)
		global_PDF[i] += (float)global_frequency[i] / (float)image_size;

//	MPI_Reduce(local_PDF, global_PDF, color_range, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
//	MPI_Bcast(&global_PDF, color_range, MPI_FLOAT, 0, MPI_COMM_WORLD);
}

void scale(int rank, int world_size) {

	for (int i = 0; i < color_range; i++)
		final_lookup[i] = CDF[i] * new_scale;

	MPI_Bcast(final_lookup, color_range, MPI_INT, 0, MPI_COMM_WORLD);
}

void update_image(int* local_image, int image_size, int chunk_size, int world_size, int rank) {

	for (int i = 0; i < chunk_size; i++)
		local_image[i] = final_lookup[local_image[i]];

	MPI_Gather(local_image, chunk_size, MPI_INT, UpdatedImage, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
}

int main()
{
	int start_s, stop_s, TotalTime = 0;
	System::String^ imagePath;
	std::string img;
	img = "..//Data//Input//2N.png";
	imagePath = marshal_as<System::String^>(img);

	MPI_Init(NULL, NULL);
	int world_size, rank, *imageData=NULL;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//int arr[] = { 3,2,4,5,7,7,8,2,3,1,2,3,5,4,6,7 };


	if (rank == 0) {
		imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	}
		start_s = clock();
	
	MPI_Bcast(&ImageHeight, 1, MPI_INT, 0, MPI_COMM_WORLD);	
	MPI_Bcast(&ImageWidth, 1, MPI_INT, 0, MPI_COMM_WORLD);

		 
	UpdatedImage= new int[(ImageHeight * ImageWidth)];
	int image_size = (ImageHeight * ImageWidth);
	int chunk_size = (image_size) / world_size;
	int start = rank * chunk_size;
	int end = start + chunk_size;

	local_image = new int[chunk_size+5];
	MPI_Scatter(imageData, chunk_size, MPI_INT, local_image, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

	countFrequency(local_image, imageData, chunk_size,image_size, rank, world_size);

	if (rank == 0) {
		calcPDF(image_size, rank, world_size);
		calcCDF();
		scale(rank, world_size);
	}

	MPI_Bcast(CDF, color_range, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(final_lookup, color_range, MPI_INT, 0, MPI_COMM_WORLD);


	update_image(local_image,image_size, chunk_size, world_size, rank);

	if (rank == 0)
	{
		
		int start = (world_size - 1) * chunk_size;
		int end = start + chunk_size;
		//out of size of sub image 

		for (int i = end; i < image_size; i++) {
			UpdatedImage[i] = final_lookup[ imageData[i] ];
		}

	     stop_s = clock();

		createImage(UpdatedImage, ImageWidth, ImageHeight, 8);
		
	     TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	     std::cout << "time: " << TotalTime << endl;
	}


	MPI_Finalize();

	//free(imageData);
	
	return 0;

}