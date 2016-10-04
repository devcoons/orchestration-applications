	#include <iostream>
	#include <string.h>
	#include <string>
	#include <unistd.h>
	#include <getopt.h>
	#include <time.h>
	#include <pthread.h>
	#include <float.h>

	extern "C"
	{
		#include <extend.h>
	}
	
	using namespace std;
	
	pthread_t * TID;
	
	void print_usage()
	{
		printf("Missing Arguments!!\n");
	}

	void * Thread(void * arg)
	{
		time_t t;
		
		srand((unsigned) time(&t));
		
		int a=0;
		
		int b=0;
		
		while(1)
		{
			b = 1;
			for (a = 0; a < 15000000; a++)
				b = b + 1;
			usleep((rand() % 50) * 8000);
		}		
	}
	
	
	int main(int argc, char *argv[])
	{
		int long_index = 0, opt = 0;
		
		int threads=0;
		char *appName = NULL, *indPolicy = NULL;


		static struct option long_options[] = {
					{"threads", 	required_argument,  0,  't' },
					{0,			0,					0,	0   }
				};

		while ( (opt = getopt_long(argc, argv, "t:", long_options, &long_index )) != -1 ) 
			switch (opt) 
			{
				case 't':
					threads = atoi(optarg);
					break;
		
				default:
					print_usage();
					exit(EXIT_FAILURE);
			}
		
		if( (TID = (pthread_t *)malloc(threads * sizeof(pthread_t))) == NULL )
			
			return -1;
int i;
		for(i=0;i<threads;i++)
			
			pthread_create(&TID[i], NULL, Thread,NULL);
			
			while(1)
			{
				usleep(1000);
			}
		
		return 0;
	}