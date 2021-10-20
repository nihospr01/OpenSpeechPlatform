/**
 * Author:    Satyam Gaba <sgaba@ucsd.edu>
 * Created:   06/10/2020
 * 
 **/

#include "libimu_new.h"
#define PCD_DEV "iio:device1"
#define LEFT_EAR_DEV "iio:device2"
#define RIGHT_EAR_DEV "iio:device3"
#define PCD_TRIG "trigger1"
#define LEFT_EAR_TRIG "trigger2"
#define RIGHT_EAR_TRIG "trigger3"

bool stop;

void handle_sig(int sig)
{
	printf("Waiting for process to finish... got signal : %d\n", sig);
	stop = true;
}


/* simple configuration and streaming */
int main (int argc, char* argv[])
{	
	// define iio
	int num_dev = 3;
	const char *dev_names[]   = {PCD_DEV, LEFT_EAR_DEV, RIGHT_EAR_DEV};
	const char *trigger_names[] = {PCD_TRIG, LEFT_EAR_TRIG,RIGHT_EAR_TRIG};
	const int buffer_length = 1; // non-1 buffer_length not implemented yet
	// define processing
	PassFilter filt = ALL;

	// define lsl
	bool is_lsl = false;
	std::string lsl_stream_name = "my_name";
	std::string lsl_stream_type = "imu_data";
	std::string out_file_name = "output.csv";

	if ( argc == 1 ) {
		printf("* No arguments were passed.\n" );
		printf("* Taking Default arguments.\n");
	} else {
		num_dev = atoi(argv[1]);
		if(argc > 2 && std::string(argv[2]) == "lsl"){
			is_lsl = true;
		}
		if(argc > 3){
			if (std::string(argv[3]) == "all"){
				filt = ALL;
			} else if (std::string(argv[3]) == "acc"){
				filt = ACCEL;
			} else if (std::string(argv[3]) == "gyr"){
				filt = GYRO;
			} else{
				std::cout << std::string(argv[2]) << std::endl;
				std::cout << "Invalid Filter Argument" << std::endl;
				std::cout << "Valid options: all(default), acc or gyr" << std::endl;
				throw;
			}	
		}
		if(argc > 4){
			std::string dir_name = "/opt/imu_data/";
			out_file_name = dir_name + std::string(argv[4]) + std::string("_imu.csv");
		}
	}

	std::cout << "out_file_name" <<out_file_name << std::endl;
	// only uni-length buffer implemented
	static_assert (buffer_length==1);

	if(!is_lsl){
		// std::string subject_id;
		// std::string activity;

		// std::cout << "Enter Subject ID: ";
		// std::cin >> subject_id;
		// std::cout << "Enter Activity Name: ";
		// std::cin >> activity;

		// out_file_name = subject_id + "_" + activity + ".csv";
	}
	std::cout << "Output will be saved to " << out_file_name << std::endl;

	struct info libimu_info = {
		filt,
		is_lsl,
		lsl_stream_name,
		lsl_stream_type,
		out_file_name,
		num_dev,
		buffer_length,
	};

	imu_device *devices;
	devices = new imu_device[num_dev];
	
	// Listen to ctrl+c and assert
	signal(SIGINT, handle_sig);

	Source iio(libimu_info, dev_names, trigger_names, devices);
	Processing process(libimu_info);
	Sink lsl(libimu_info, iio, process);
	Bind refimu(iio, process, lsl, stop);

	refimu.fetch_data(stop);

	return 0;
}
