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

	
	void print_usage()
	{
		printf("Missing Arguments!!\n");
	}

	FILE *fpipe;
	char  * command_MM_CPU_BIG = "sudo appMM_local 30 30 30 1500 0x3 1 0";
	char  * command_MM_CPU_LITTLE   = "sudo appMM_local 30 30 30 1500 0x3c 1 0";
	char  * command_MM_FPGA_BIG  = "sudo appMM_HW_arm 30 30 30 1500 1 0x3 s 1 0";
	char  * command_MM_FPGA_LITTLE = "sudo appMM_HW_arm 30 30 30 1500 1 0x3c s 1 0";
	char  * command_STOP_GPPU = "sudo stop_gppu 0";
	char  * command_NOGPPU_LITTLE = "sudo appMM_noGPPU 30 30 30 1500 0x3c 1 0";
	char  * command_NOGPPU_BIG = "sudo appMM_noGPPU 30 30 30 1500 0x3 1 0";

void CPUKernel1(void * arg)
{
	set_affinity(3, pthread_self());
		cout << "Kernel [1]" << endl;
		if ( !(fpipe = (FILE*)popen(command_MM_CPU_BIG, "r")) )
			perror("Problems with pipe, CPU_Kernel_1");
		pclose(fpipe);
}

void CPUKernel2(void * arg)
{
		cout << "Kernel [2]" << endl;
		if ( !(fpipe = (FILE*)popen(command_MM_CPU_LITTLE, "r")) )
			perror("Problems with pipe, CPU_Kernel_2");
		pclose(fpipe);
}
	void FPGAKernel1(void * arg)
	{
		cout << "Kernel [3]" << endl;
		if ( !(fpipe = (FILE*)popen(command_MM_FPGA_BIG, "r")) )
			perror("Problems with pipe, FPGA_Kernel_0");
		pclose(fpipe);
	}

	void FPGAKernel2(void * arg)
	{
		cout << "Kernel [4]" << endl;
		if ( !(fpipe = (FILE*)popen(command_MM_FPGA_LITTLE, "r")) )
			perror("Problems with pipe, FPGA_Kernel_1");
		pclose(fpipe);
	}
	
	void FPGAKernel3(void * arg)
	{

		cout << "Kernel [5]" << endl;
		if ( !(fpipe = (FILE*)popen(command_NOGPPU_BIG, "r")) )
			perror("Problems with pipe, FPGA_Kernel_1");
		pclose(fpipe);
	}
	
	void FPGAKernel4(void * arg)
	{

		cout << "Kernel [6]" << endl;
		if ( !(fpipe = (FILE*)popen(command_NOGPPU_LITTLE, "r")) )
			perror("Problems with pipe, FPGA_Kernel_1");
		pclose(fpipe);
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
		
		ImplementationKernel fpga1Impl = [&](void * arg) -> void {
			FPGAKernel1(arg);
		};
		
		ImplementationKernel fpga2Impl = [&](void * arg) -> void {
			FPGAKernel2(arg);
		};
		
		ImplementationKernel fpga3Impl = [&](void * arg) -> void {
			FPGAKernel3(arg);
		};
		
		ImplementationKernel fpga4Impl = [&](void * arg) -> void {
			FPGAKernel4(arg);
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
		
	//	TConnector.registerImplementation(cpu1Impl, ARMA57);
	//	TConnector.registerImplementation(cpu2Impl, ARMA53);
		
		TConnector.registerImplementation(fpga1Impl, FPGA);
		TConnector.registerImplementation(fpga2Impl, FPGA);
		
		TConnector.registerImplementation(fpga3Impl, PCI);
		TConnector.registerImplementation(fpga4Impl, PCI);
		
		TConnector.setProfiling(profiling);
		
		::cout<<std::endl;
		
		for (int i = 0; i < execTimes; i++)
		{			if ( !(fpipe = (FILE*)popen(command_STOP_GPPU, "r")) )
			perror("Problems with pipe, FPGA_Kernel_1");
			pclose(fpipe);
			TConnector.execute(NULL);		

		}
		
		TConnector.disconnect();
		
		return 0;
	}