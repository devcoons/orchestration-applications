#pragma once
#ifndef _PREWITT_
#define _PREWITT_
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>
#include <getopt.h>
	#include <iostream>
	#include <string.h>
	#include <string>
	#include <unistd.h>
	#include <getopt.h>
	
	#include <orchestration/orchestration.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video.hpp>

#include <pthread.h>
extern "C"
{
	#include <extend.h>
}

	using namespace std;
	using namespace cv;



	typedef struct thread_arguments_t
	{
		int max_width;
		int max_height;
		int start_width;
		int end_width;
		Mat * current_frame;
		Mat * gray_frame;
		int caffinity;
	}thread_arguments_t;
		
		
	typedef struct kernel_arguments_t
	{
		int max_width;
		int max_height;
		Mat * current_frame;
		int threads[2];
		int caffinity[2];
	}kernel_arguments_t;
		
		
	void print_usage();
	void * prewitt_thread(void *);	
	void thread_pool(Mat *,Mat *,int,int,int,int);
	void prewitt(Mat *,Mat *,int starting_width,int ending_width,int,int);
	void CPU_Kernel_1(void * arg);
	void CPU_Kernel_2(void * arg);
#endif