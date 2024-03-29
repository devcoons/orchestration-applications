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

//	char  * command_MM_CPU_BIG = "sudo appMM_local 30 30 30 1500 0x3 1 0";
//	char  * command_MM_CPU_LITTLE   = "sudo appMM_local 30 30 30 1500 0x3c 1 0";
//	char  * command_MM_FPGA_BIG  = "sudo appMM_HW_arm 30 30 30 1500 1 0x3 s 1 0";
//	char  * command_MM_FPGA_LITTLE = "sudo appMM_HW_arm 30 30 30 1500 1 0x3c s 1 0";
//	char  * command_STOP_GPPU = "sudo stop_gppu 0";
//	char  * command_NOGPPU_LITTLE = "sudo appMM_noGPPU 30 30 30 1500 0x3c 1 0";
//	char  * command_NOGPPU_BIG = "sudo appMM_noGPPU 30 30 30 1500 0x3 1 0";

	char  * command_MM_CPU_BIG = NULL;
	char  * command_MM_CPU_LITTLE   = NULL;
	
	char  * command_MM_FPGA_BIG  = NULL;
	char  * command_MM_FPGA_LITTLE = NULL;
	
	char  * command_NOGPPU_BIG = NULL;
	char  * command_NOGPPU_LITTLE = NULL;

	char  * command_STOP_GPPU = NULL;

	

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
		char * size=NULL, * jobs=NULL; 
		char * kernels = NULL;
		char * appName = NULL, * indPolicy = NULL;
		
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
					{"jobs", 		required_argument,  0,  'c' },
					{"size", 		required_argument,  0,  'b' },
					{"kernels", 	required_argument,  0,  'd' },
					{0,		 0,							0,	 0  }
				};

		while ( (opt = getopt_long(argc, argv, "b:c:d:a:g:l:h:p:n:r:o:e:", long_options, &long_index )) != -1 ) 
			switch (opt) 
			{
				case 'b':
					size = str_merge(6,size,optarg," ",optarg," ",optarg);

	
					
					break;
				case 'c':
					jobs = str_merge(2,jobs,optarg);
	
	break;
				case 'd':
					kernels = str_merge(2,kernels,optarg);
					break;
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
		
		command_STOP_GPPU = str_merge(2,command_STOP_GPPU,"sudo stop_gppu 0");

		command_MM_CPU_BIG 	= str_merge(6,command_MM_CPU_BIG,	"sudo appMM_local ",size," ",jobs," 0x3 1 0");
		command_MM_CPU_LITTLE 	= str_merge(6,command_MM_CPU_LITTLE ,	"sudo appMM_local ",size," ",jobs," 0x3c 1 0");
		command_MM_FPGA_BIG 	= str_merge(6,command_MM_FPGA_BIG,	"sudo appMM_HW_arm ",size," ",jobs," 1 0x3 s 1 0");
		command_MM_FPGA_LITTLE 	= str_merge(6,command_MM_FPGA_LITTLE ,	"sudo appMM_HW_arm ",size," ",jobs," 1 0x3c s 1 0");
		command_NOGPPU_BIG 	= str_merge(6,command_NOGPPU_BIG,	"sudo appMM_noGPPU ",size," ",jobs," 0x3 1 0");
		command_NOGPPU_LITTLE	= str_merge(6,command_NOGPPU_LITTLE ,	"sudo appMM_noGPPU ",size," ",jobs," 0x3c 1 0");

printf("%s\n\n",command_MM_FPGA_BIG);
			
		Orchestration::Client TConnector(getpid());

		TConnector.connect("127.0.0.1", orchPort);
		TConnector.setAppName(appName);
		
		if(strcmp(indPolicy,"Restricted")==0)
			TConnector.setPolicy(IndividualPolicyType::Restricted);
		else
			TConnector.setPolicy(IndividualPolicyType::Balanced);
		
		TConnector.setGoalMs(goal, lowestGoal, highestGoal);
		TConnector.setPriority(priority);
		printf("Selected Kernels: 2 x CPU - ");
		TConnector.registerImplementation(cpu1Impl, ARMA57);
		TConnector.registerImplementation(cpu2Impl, ARMA53);
		
		if(strcmp(kernels,"2CPU 2GPPU")==0)
		{
printf("2 x GPPU\n");
			TConnector.registerImplementation(fpga1Impl, FPGA);
			TConnector.registerImplementation(fpga2Impl, FPGA);
		}
		if(strcmp(kernels,"2CPU 2noGPPU")==0)
		{
printf("2 x noGPPU\n");	
			TConnector.registerImplementation(fpga3Impl, FPGA);
			TConnector.registerImplementation(fpga4Impl, FPGA);		
		}
		if(strcmp(kernels,"2CPU 2noGPPU 2GPPU")==0)
		{
printf("2 x noGPPU 2 x GPPU\n");
			TConnector.registerImplementation(fpga1Impl, FPGA);
			TConnector.registerImplementation(fpga2Impl, FPGA);		
			TConnector.registerImplementation(fpga3Impl, FPGA);
			TConnector.registerImplementation(fpga4Impl, FPGA);		
		}

		
		TConnector.setProfiling(profiling);
		
		::cout<<std::endl;
		
		for (int i = 0; i < execTimes; i++)
		{			
			if ( !(fpipe = (FILE*)popen(command_STOP_GPPU, "r")) )
			perror("Problems with pipe, FPGA_Kernel_1");
			pclose(fpipe);
			TConnector.execute(NULL);		

		}
		
		TConnector.disconnect();
		
		return 0;
	}
