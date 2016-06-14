#include "Prewitt.h"

	pthread_t * TID;		
	pthread_mutex_t LOCKTIME;
	pthread_mutex_t LOCKSTATE;	
	pthread_barrier_t BEGINBRR;
	pthread_barrier_t ENDBRR;

	void CPU_Kernel_2(void * _kernel_args)
	{
		set_affinity(60, pthread_self());
		cout << "\rKernel [2]\t" ;
		int i=0;
		Mat gray_frame;
		kernel_arguments_t * kernel_args = (kernel_arguments_t*)_kernel_args;
		thread_arguments_t args[kernel_args->threads[1]];
		pthread_barrier_init (&BEGINBRR, NULL, kernel_args->threads[1]+1);
		pthread_barrier_init (&ENDBRR,   NULL, kernel_args->threads[1]+1);
		cvtColor(*kernel_args->current_frame, gray_frame, cv::COLOR_BGR2GRAY);
		for(i=0;i<kernel_args->threads[1];i++)
		{
			args[i].max_width = kernel_args->max_width;
			args[i].max_height = kernel_args->max_height;
			args[i].start_width = (i == 0 ? 0 : args[i-1].end_width);
			args[i].end_width = (i+1) * (kernel_args->max_width/kernel_args->threads[1]);
			args[i].current_frame = kernel_args->current_frame;
			args[i].gray_frame = &gray_frame;
			args[i].caffinity = kernel_args->caffinity[1];
		}

		if( (TID = (pthread_t *)malloc(kernel_args->threads[1] * sizeof(pthread_t))) == NULL )
			return ;

		for(i=0;i<kernel_args->threads[1];i++)
			pthread_create(&TID[i], NULL, prewitt_thread,(void*)&args[i]);
		pthread_barrier_wait (&BEGINBRR);				
		pthread_barrier_wait (&ENDBRR);
			
		for(i=0;i<kernel_args->threads[1];i++)
			pthread_join(TID[i], NULL);
			
		pthread_barrier_destroy(&BEGINBRR);
		pthread_barrier_destroy(&ENDBRR);
		free(TID);
	}
		
	void CPU_Kernel_1(void * _kernel_args)
	{
		set_affinity(3, pthread_self());
		cout << "\rKernel [1]\t" ;
		int i=0;
		Mat gray_frame;
		kernel_arguments_t * kernel_args = (kernel_arguments_t*)_kernel_args;
		thread_arguments_t args[kernel_args->threads[0]];
			
		pthread_barrier_init (&BEGINBRR, NULL, kernel_args->threads[0]+1);
		pthread_barrier_init (&ENDBRR,   NULL, kernel_args->threads[0]+1);
		cvtColor(*kernel_args->current_frame, gray_frame, cv::COLOR_BGR2GRAY);
		for(i=0;i<kernel_args->threads[0];i++)
		{
			args[i].max_width = kernel_args->max_width;
			args[i].max_height = kernel_args->max_height;
			args[i].start_width = (i == 0 ? 0 : args[i-1].end_width);
			args[i].end_width = (i+1) * (kernel_args->max_width/kernel_args->threads[0]);
			args[i].current_frame = kernel_args->current_frame;
			args[i].gray_frame = &gray_frame;
			args[i].caffinity = kernel_args->caffinity[0];
		}

		if( (TID = (pthread_t *)malloc(kernel_args->threads[0] * sizeof(pthread_t))) == NULL )
			return ;

		for(i=0;i<kernel_args->threads[0];i++)
			pthread_create(&TID[i], NULL, prewitt_thread,(void*)&args[i]);

		pthread_barrier_wait (&BEGINBRR);				
		pthread_barrier_wait (&ENDBRR);
			
		for(i=0;i<kernel_args->threads[0];i++)
			pthread_join(TID[i], NULL);
			
		pthread_barrier_destroy(&BEGINBRR);
		pthread_barrier_destroy(&ENDBRR);
		free(TID);
	}
	
	
	void * prewitt_thread(void * arg)
	{	
		thread_arguments_t * args = (thread_arguments_t*)arg;
		set_affinity(args->caffinity, pthread_self());
		pthread_barrier_wait (&BEGINBRR);
		uint8_t result;
		float accum1 = 0, accum2 = 0;
		int i,j,row,col,row1,col1;	
		float kGx[3][3] = { { -1.0f, 0, 1.0f },{ -1.0f, 0, 1.0f },{ -1.0f, 0, 1.0f } };
		float kGy[3][3] = { { -1.0f, -1.0f, -1.0f },{ 0, 0, 0 },{ 1.0f, 1.0f, 1.0f } };
			for (row = 0; row <args->max_height; row++)
				for (col = args->start_width; col < args->end_width; col++) 
				{
					accum1 = 0;
					accum2 = 0;
					for (i = -1; i <= 1; i++)
						for (j = -1; j <= 1; j++) 
						{
							row1 = row + i >= 0 ? row + i : 0;
							col1 = col + j >= 0 ? col + j : 0;
							row1 = row + i >= args->max_height ? args->max_height - 1 : row1;
							col1 = col + j >= args->max_width  ? args->max_width  - 1 : col1;
							accum1 += (*(args->gray_frame)).at<uint8_t>(row1, col1) * kGy[1 + i][1 + j];
							accum2 += (*(args->gray_frame)).at<uint8_t>(row1, col1) * kGx[1 + i][1 + j];					
						}
						result = (uint8_t) sqrt(pow(accum1, 2) + pow(accum2, 2));
						args->current_frame->at<Vec3b>(row,col)[0] = result;
						args->current_frame->at<Vec3b>(row,col)[1] = result;
						args->current_frame->at<Vec3b>(row,col)[2] = result;					
				}	
		pthread_barrier_wait (&ENDBRR);
		return NULL;
	}