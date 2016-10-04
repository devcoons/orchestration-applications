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
//	char  * command_MM_CPU_BIG = "sudo appSobel_local image_hd 5 0x3 1 0";
//	char  * command_MM_CPU_LITTLE   = "sudo appSobel_local image_hd 5 0x3c 1 0";
//	char  * command_MM_FPGA_BIG  = "sudo appSobel_HW_arm image_hd 5 2 0x3c 1 0";
//	char  * command_MM_FPGA_LITTLE = "sudo appSobel_HW_arm image_hd 5 2 0x3c 1 0";
//	char  * command_STOP_GPPU = "sudo stop_gppu 0";
//	char  * command_NOGPPU_LITTLE = "sudo appSobel_noGPPU image_hd 5 0x3c 1 0";
//	char  * command_NOGPPU_BIG = "sudo appSobel_noGPPU image_hd 5 0x3 1 0";

	char  * command_CPU_BIG = NULL;
	char  * command_CPU_LITTLE   = NULL;
	
	char  * command_GPPU_BIG  = NULL;
	char  * command_GPPU_LITTLE = NULL;
	
	char  * command_NOGPPU_BIG = NULL;
	char  * command_NOGPPU_LITTLE = NULL;

	char  * command_PCIe_BIG = NULL;
	char  * command_PCIe_LITTLE = NULL;

	char  * command_STOP_GPPU = NULL;


void CPUKernel1(void * arg)
{
	set_affinity(3, pthread_self());
		cout << "Kernel [1]" << endl;
		if ( !(fpipe = (FILE*)popen(command_CPU_BIG, "r")) )
			perror("Problems with pipe, CPU_Kernel_1");
		pclose(fpipe);
}

void CPUKernel2(void * arg)
{
		cout << "Kernel [2]" << endl;
		if ( !(fpipe = (FILE*)popen(command_CPU_LITTLE, "r")) )
			perror("Problems with pipe, CPU_Kernel_2");
		pclose(fpipe);
}
	void FPGAKernel1(void * arg)
	{
		cout << "Kernel [3]" << endl;
		if ( !(fpipe = (FILE*)popen(command_GPPU_BIG, "r")) )
			perror("Problems with pipe, FPGA_Kernel_0");
		pclose(fpipe);
	}

	void FPGAKernel2(void * arg)
	{
		cout << "Kernel [4]" << endl;
		if ( !(fpipe = (FILE*)popen(command_GPPU_LITTLE, "r")) )
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

	void PCIeKernel1(void * arg)
	{

		cout << "Kernel [7]" << endl;
		if ( !(fpipe = (FILE*)popen(command_PCIe_BIG, "r")) )
			perror("Problems with pipe, FPGA_Kernel_1");
		pclose(fpipe);
	}
	
	void PCIeKernel2(void * arg)
	{

		cout << "Kernel [8]" << endl;
		if ( !(fpipe = (FILE*)popen(command_PCIe_LITTLE, "r")) )
			perror("Problems with pipe, FPGA_Kernel_1");
		pclose(fpipe);
	}

	int main(int argc, char *argv[])
	{
		int long_index = 0, opt = 0;
		
		int goal,lowestGoal,highestGoal,profiling,priority,orchPort,execTimes;
		char *appName = NULL, *indPolicy = NULL;
		char * kernels = NULL;
		char * image = NULL, * times = NULL;

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
if ( !(fpipe = (FILE*)popen(command_STOP_GPPU, "r")) )
			perror("Could no close the GPPU driver");
			pclose(fpipe);
			FPGAKernel3(arg);
		};
		
		ImplementationKernel fpga4Impl = [&](void * arg) -> void {
if ( !(fpipe = (FILE*)popen(command_STOP_GPPU, "r")) )
			perror("Could no close the GPPU driver");
			pclose(fpipe);
			FPGAKernel4(arg);
		};
		ImplementationKernel pcie1Impl = [&](void * arg) -> void {
			PCIeKernel1(arg);
		};
		
		ImplementationKernel pcie2Impl = [&](void * arg) -> void {
			PCIeKernel2(arg);
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
					{"image", 		required_argument,  0,  'c' },
					{"times", 		required_argument,  0,  'b' },
					{"kernels", 	required_argument,  0,  'd' },
					{0,			0,					0,	0   }
				};

		while ( (opt = getopt_long(argc, argv, "b:c:d:a:g:l:h:p:n:r:o:e:", long_options, &long_index )) != -1 ) 
			switch (opt) 
			{
				case 'b':
					image = str_merge(2,image,optarg);
					break;
					
				case 'c':
					times = str_merge(2,times,optarg);
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

		command_CPU_BIG 		= str_merge(6,command_CPU_BIG,			"sudo appSobel_local ",image," ",times," 0x3 1 0");
		command_CPU_LITTLE 		= str_merge(6,command_CPU_LITTLE ,		"sudo appSobel_local ",image," ",times," 0x3c 1 0");
		command_GPPU_BIG 		= str_merge(6,command_GPPU_BIG,			"sudo appSobel_HW_arm ",image," ",times," 2 0x3 s 1 0");
		command_GPPU_LITTLE 	= str_merge(6,command_GPPU_LITTLE ,		"sudo appSobel_HW_arm ",image," ",times," 2 0x3c s 1 0");
		command_NOGPPU_BIG 		= str_merge(6,command_NOGPPU_BIG,		"sudo appSobel_noGPPU ",image," ",times," 0x3 1 0");
		command_NOGPPU_LITTLE	= str_merge(6,command_NOGPPU_LITTLE ,	"sudo appSobel_noGPPU ",image," ",times," 0x3c 1 0");
		command_PCIe_BIG 		= str_merge(6,command_PCIe_BIG,			"sudo appSobel_pcie_mmap \"/usr/local/share/save_data/",image,".bmp\" ",times," 0");
		command_PCIe_LITTLE		= str_merge(6,command_PCIe_LITTLE ,		"sudo appSobel_pcie_sg \"/usr/local/share/save_data/",image,".bmp\" ",times," 0");	
		
		
		printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",command_CPU_BIG,command_CPU_LITTLE,command_GPPU_BIG,command_GPPU_LITTLE,command_NOGPPU_BIG,command_NOGPPU_LITTLE,command_PCIe_BIG,command_PCIe_LITTLE);

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
			printf("2 x GPPU");
			TConnector.registerImplementation(fpga1Impl, FPGA);
			
			TConnector.registerImplementation(fpga2Impl, FPGA);
		}
		
		if(strcmp(kernels,"2CPU 2noGPPU")==0)
		{
			printf("2 x noGPPU");	
			TConnector.registerImplementation(fpga3Impl, FPGA);
			TConnector.registerImplementation(fpga4Impl, FPGA);		
		}
		
		if(strcmp(kernels,"2CPU 2noGPPU 2GPPU")==0)
		{
			printf("2 x noGPPU 2 x GPPU");
			TConnector.registerImplementation(fpga1Impl, FPGA);
			TConnector.registerImplementation(fpga2Impl, FPGA);		
			TConnector.registerImplementation(fpga3Impl, FPGA);
			TConnector.registerImplementation(fpga4Impl, FPGA);		
		}

		if(strcmp(kernels,"2CPU 2PCIe")==0)
		{
			printf("2 x PCIe");
			TConnector.registerImplementation(pcie1Impl , FPGA);
			TConnector.registerImplementation(pcie2Impl , FPGA);			
		}
	
	
		if(strcmp(kernels,"2CPU 2PCIe 2GPPU")==0)
		{
			printf("2 x PCIe 2 x GPPU");
			TConnector.registerImplementation(fpga1Impl, FPGA);
			TConnector.registerImplementation(fpga2Impl, FPGA);	
			TConnector.registerImplementation(pcie1Impl , FPGA);
			TConnector.registerImplementation(pcie2Impl , FPGA);			
		}
		
		if(strcmp(kernels,"2CPU 2PCIe 2noGPPU")==0)
		{
			printf("2 x PCIe 2 x noGPPU");
			TConnector.registerImplementation(fpga3Impl, FPGA);
			TConnector.registerImplementation(fpga4Impl, FPGA);	
			TConnector.registerImplementation(pcie1Impl , FPGA);
			TConnector.registerImplementation(pcie2Impl , FPGA);			
		}
		
		if(strcmp(kernels,"2CPU 2PCIe 2GPPU 2noGPPU")==0)
		{
			printf("2 x PCIe 2 x noGPPU 2 x GPPU");
			TConnector.registerImplementation(fpga1Impl, FPGA);
			TConnector.registerImplementation(fpga2Impl, FPGA);		
			TConnector.registerImplementation(fpga3Impl, FPGA);
			TConnector.registerImplementation(fpga4Impl, FPGA);
			TConnector.registerImplementation(pcie1Impl , FPGA);
			TConnector.registerImplementation(pcie2Impl , FPGA);			
		}

		printf("\n");

		TConnector.setProfiling(profiling);
		
		::cout<<std::endl;
		
		for (int i = 0; i < execTimes; i++)
		{			
			
			TConnector.execute(NULL);		

		}
		
		TConnector.disconnect();
		
		return 0;
	}
