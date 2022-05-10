#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include<mpi.h>
#include <ctime>// include this header 
#pragma once

#define NUM_OF_PHOTOES 495
#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
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

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

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
			//i * OriginalImageWidth + j
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

std::string getImagePath(int index) 
{
	std::string img;
	if (index < 10)
	{
		img = "..//Data//Input//in00000" + to_string(index) + ".jpg";
	}
	else if (index < 100)
	{
		img = "..//Data//Input//in0000" + to_string(index) + ".jpg";
	}
	else if (index < 1000)
	{
		img = "..//Data//Input//in000" + to_string(index) + ".jpg";
	}
	return img;
}

int main()
{
	MPI_Init(NULL, NULL);
	int size, rank;
	int ImageWidth = 4, ImageHeight = 4;
	int start_s, stop_s, TotalTime = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0)
	{
		start_s = clock();
	}
	int** imagesMat = new int* [NUM_OF_PHOTOES / size];
	int imgCounter = 0;
	for (int i = (NUM_OF_PHOTOES * rank / size) + 1; i < (NUM_OF_PHOTOES * (rank + 1) / size) + 1; i++)
	{
		System::String^ imagePath;
		std::string img = getImagePath(i);
		imagePath = marshal_as<System::String^>(img);
		int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
		imagesMat[imgCounter] = new int[ImageHeight * ImageWidth];
		for (int j = 0; j < ImageHeight * ImageWidth; j++)
		{
			imagesMat[imgCounter][j] = imageData[j];
		}
		free(imageData);
		imgCounter++;
	}
	int* means = new int[ImageHeight * ImageWidth];
	int sum;
	for (int i = 0; i < ImageHeight * ImageWidth; i++)
	{
		sum = 0;
		for (int j = 0; j < NUM_OF_PHOTOES / size; j++)
		{
			sum += imagesMat[j][i];
		}
		means[i] = sum / (NUM_OF_PHOTOES / size);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	int* meanOfMeans = new int[ImageHeight * ImageWidth];
	MPI_Reduce(means, meanOfMeans, ImageHeight * ImageWidth, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		for (int i = 0; i < ImageHeight * ImageWidth; i++)
		{
			meanOfMeans[i] /= size;
		}
		createImage(meanOfMeans, ImageWidth, ImageHeight, 0);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(meanOfMeans, ImageHeight * ImageWidth, MPI_INT, 0, MPI_COMM_WORLD);
	imgCounter = 0;
	int TH = 75;
	for (int i = (NUM_OF_PHOTOES * rank / size) + 1; i < (NUM_OF_PHOTOES * (rank + 1) / size) + 1; i++)
	{
		int* subtraction = new int[ImageHeight * ImageWidth];
		for (int j = 0; j < ImageHeight * ImageWidth; j++)
		{
			subtraction[j] = abs(meanOfMeans[j] - imagesMat[imgCounter][j]) > TH ? 255 : 0;
		}
		createImage(subtraction, ImageWidth, ImageHeight, i);
		free(subtraction);
		imgCounter++;
	}
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
	{
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time: " << TotalTime << endl;
	}
	MPI_Finalize();
	return 0;
	//mpiexec HPC_ProjectTemplate.exe
}