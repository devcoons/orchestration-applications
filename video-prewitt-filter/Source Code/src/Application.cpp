#include "Prewitt.h"


		int main(int argc, char ** argv)
		{
			int long_index = 0, opt = 0;
			char * input_video = NULL, * output_video = NULL;

			int goal,lowestGoal,highestGoal,profiling,priority,orchPort,execTimes,threads=1;
			char *appName = NULL, *indPolicy = NULL;
		
			static struct option long_options[] = {
					{"app.name", 	required_argument,  0,  'a' },
					{"goal", 		required_argument,  0,  'g' },
					{"lowestgoal", 	required_argument,  0,  'l' },
					{"highestgoal", required_argument,  0,  'h' },
					{"profiling", 	required_argument,  0,  'p' },
					{"priority", 	required_argument,  0,  'n' },
					{"ind.policy", 	required_argument,  0,  'r' },
					{"orch.port", 	required_argument,  0,  'o' },
					{"input.video",	required_argument,  0,  'e' },
					{"output.video",required_argument,  0,  'v' },
					{"threads",		required_argument,  0,  'q' },
					{0,			0,					0,	0   }
				};

			while ( (opt = getopt_long(argc, argv, "a:g:l:h:p:n:r:o:e:v:q:", long_options, &long_index )) != -1 ) 
			switch (opt) 
			{
				case 'a':
					appName = str_merge(2,appName,optarg);
					break;
				case 'g' : 
					goal=atoi(optarg);
					break;
				case 'l' : 
					lowestGoal=atoi(optarg);
					break;
				case 'h' : 
					highestGoal=atoi(optarg);
					break;
				case 'p' : 
					profiling=atoi(optarg);
					break;
				case 'n' : 
					priority=atoi(optarg);
					break;
				case 'r' : 
					indPolicy=str_merge(2,indPolicy,optarg);
					break;
				case 'o' : 
					orchPort=atoi(optarg);
					break;
				case 'e' : 
					input_video = str_merge(2,input_video,optarg);
				case 'v' : 
					output_video = str_merge(2,output_video,optarg);
					break;
				case 'q' : 
					threads=atoi(optarg);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
			}
			
			if(input_video==NULL || output_video==NULL || threads<=0 || goal <0 || profiling<0)
			{
				print_usage();
				exit(EXIT_FAILURE);		
			}
			
			cout<<"Loading Input Video"<<endl;
			cv::String a(input_video);
			cv::VideoCapture input_cap(a);
			
			if (!input_cap.isOpened())
			{
				std::cerr << "ERROR: Could not open video " <<input_video << std::endl;
				return 1;
			}
		  
			cout<<"Input Video Details"<<endl;
			cout<<"  - Frames :"<<input_cap.get(CV_CAP_PROP_FRAME_COUNT)<<endl;	
			cout<<"  - FPS    :"<<input_cap.get(CV_CAP_PROP_FPS)<<endl;	
			cout<<"  - Width  :"<<input_cap.get(CV_CAP_PROP_FRAME_WIDTH)<<endl;	
			cout<<"  - Height :"<<input_cap.get(CV_CAP_PROP_FRAME_HEIGHT)<<endl;	
		  
			
			Size S = Size((int) input_cap.get(CV_CAP_PROP_FRAME_WIDTH), (int) input_cap.get(CV_CAP_PROP_FRAME_HEIGHT));
			
			cout<<"Setting Output Video"<<endl;
			VideoWriter output_cap(output_video,  input_cap.get(CV_CAP_PROP_FOURCC), input_cap.get(CV_CAP_PROP_FPS), S,true);
					   
			if(!output_cap.isOpened())
			{
				std::cerr << "ERROR: Could not open video " << output_video << std::endl;
				return 1;
			}	

			
			ImplementationKernel cpu1Impl = [&](void * arg) -> void 
			{
				CPU_Kernel_1(arg);
			};
			ImplementationKernel cpu2Impl = [&](void * arg) -> void 
			{
				CPU_Kernel_2(arg);
			};
			
			Orchestration::Client TConnector(getpid());
		
			TConnector.connect("127.0.0.1", orchPort);
			TConnector.setAppName(appName);
		
			if(strcmp(indPolicy,"Restricted")==0)
				TConnector.setPolicy(IndividualPolicyType::Restricted);
			else
				TConnector.setPolicy(IndividualPolicyType::Balanced);
		
			TConnector.setGoalMs(goal, lowestGoal, highestGoal);
			TConnector.setPriority(priority);
			
			TConnector.registerImplementation(cpu1Impl, ARMA57);
			TConnector.registerImplementation(cpu2Impl, ARMA53);
			
			TConnector.setProfiling(profiling);

			Mat current_frame;//,gray_frame;
			int current_frame_pos=0;
			kernel_arguments_t kernel_args;
			
			kernel_args.max_width 		= (int)input_cap.get(CV_CAP_PROP_FRAME_WIDTH);
			kernel_args.max_height 		= (int)input_cap.get(CV_CAP_PROP_FRAME_HEIGHT);
			kernel_args.threads[0] 		= threads;
			kernel_args.caffinity[0] 	= 3;
			kernel_args.threads[1] 		= threads;
			kernel_args.caffinity[1] 	= 60;	
			kernel_args.current_frame 	= & current_frame;			
			
			
			cout<<"Execute Kernels"<<endl;

			while(1)
			{
	
				if(!input_cap.read(current_frame))break;
				
				TConnector.execute(&kernel_args);	
				output_cap.write(current_frame);
			}
			TConnector.disconnect();
			return 0;
		}

		void print_usage()
		{
			printf("Usage : -n$(app_name) -t$(throughput) [-p$(profiling)] -i$(input_video) -o$(output_video) -b$(threads)\n");
		}