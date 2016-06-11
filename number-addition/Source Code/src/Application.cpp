	#include <iostream>
	#include <string.h>
	#include <string>
	#include <unistd.h>
	#include <getopt.h>
	
	#include <orchestration/orchestration.h>
	
	extern "C"
	{
		#include <extend.h>
	}
	
	using namespace std;

	long long a = 0;
	long long b = 0;
	
	void print_usage()
	{
		printf("Missing Arguments!!\n");
	}

	void CPUKernel1(void * arg)
	{
		set_affinity(3, pthread_self());
		cout << "Kernel [1]" << endl;
		b = 1;
		for (a = 0; a < 20000000; a++)
			b = b + 1;
	}

	void CPUKernel2(void * arg)
	{
		set_affinity(60, pthread_self());
		cout << "Kernel [2]" << endl;
		b = 1;
		for (a = 0; a <20000000; a++)
			b = b + 1;
	}

	int main(int argc, char *argv[])
	{
		int long_index = 0, opt = 0;
		
		int goal,lowestGoal,highestGoal,profiling,priority,orchPort,execTimes;
		char *appName = NULL, *indPolicy = NULL;
		
		ImplementationKernel cpu1Impl = [&](void * arg) -> void {
			CPUKernel1(arg);
		};
		ImplementationKernel cpu2Impl = [&](void * arg) -> void {
			CPUKernel2(arg);
		};

		static struct option long_options[] = {
					{"app.name", 	required_argument,  0,  'a' },
					{"goal", 		required_argument,  0,  'g' },
					{"lowestgoal", 	required_argument,  0,  'l' },
					{"highestgoal", required_argument,  0,  'h' },
					{"profiling", 	required_argument,  0,  'p' },
					{"priority", 	required_argument,  0,  'n' },
					{"ind.policy", 	required_argument,  0,  'r' },
					{"orch.port", 	required_argument,  0,  'o' },
					{"exec.times", 	required_argument,  0,  'e' },
					{0,			0,					0,	0   }
				};

		while ( (opt = getopt_long(argc, argv, "a:g:l:h:p:n:r:o:e:", long_options, &long_index )) != -1 ) 
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
					execTimes=atoi(optarg);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
			}
		
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
		
		::cout<<std::endl;
		
		for (int i = 0; i < execTimes; i++)
		{
			TConnector.execute(NULL);
			::cout<<"\r"<<i<<"  Current ms:"<<TConnector.getCurrentMs()<<" Average ms:"<<TConnector.getAverageMs();
		}
		
		TConnector.disconnect();
		
		return 0;
	}