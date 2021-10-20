#ifndef LIBIMU_NEW_H
#define LIBIMU_NEW_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <cstring>
#include <iio.h>
#include <lsl_cpp.h>
#include <thread>
#include <iostream>
#include <fstream>


// void send_imu_data(const char *dev_names[], const char *trigger_str[], const int num_dev, const int buffer_length, std::string lsl_stream_name, std::string lsl_stream_type); 

struct imu_device{
	// device identification 
	const char *dev_name;
	const char *trigger_name;
	iio_device *dev;

	/* IIO structs required for streaming */
	iio_context *ctx;
	iio_buffer  *rxbuf;
	iio_channel **channels;
	unsigned int channel_count;

	// hardware trigger
	iio_device *trigger;

	bool has_ts;
};

// bool has_repeat;
enum PassFilter {ALL, ACCEL, GYRO};

struct info{
	// PassFilter f;
	PassFilter data_filter_level;

	bool is_lsl;
	std::string lsl_stream_name;
	std::string lsl_stream_type;
	std::string out_name;

	const int num_dev;     
	const int buffer_length;
};

class Source{
	private:
		const char** dev_names;
		const char** trigger_name;
		const int buffer_length;
		bool has_repeat;
		int64_t last_ts = 0;

		bool version_check();

	public:
		const int num_dev;
		imu_device *devices;
		int total_channel_count;

		Source(struct info& libimu_info, const char** dev_names, const char** trigger_name, imu_device* devices);
		void forward(std::vector<float>& send_sample);
};

class Processing{
	/* Process the raw data*/
	private:
		PassFilter data_filter_level; //1,2 # 0 -> both, 1 -> gyro only, 2 -> acc only 
		int num_dev;

	public:
		int out_num_channel;
		Processing(struct info& libimu_info);
		void forward(std::vector<float> &data);
};

class Sink{
	/* sink for the processed data*/
	private:
		std::ofstream out_file;
		float sample_number;
		int num_channels;
		std::string lsl_stream_name;
		std::string lsl_stream_type;
		Source source;
		bool last_cons = false;
		bool have_cons = false;
		bool look_cons = true;

	
	public:
		bool is_lsl;
		const char* file_name;
		lsl::stream_info lsl_info;
		lsl::stream_outlet lsl_outlet;

		Sink(struct info& libimu_info, Source& source, Processing& process);
		void forward(std::vector<float>& data);
};


class Bind{
	private:
		Source* source;
		Processing* processing;
		Sink* sink;

	public:
		Bind(Source& source, Processing& processing, Sink& sink, bool& stop);
        void fetch_data(bool& stop);
};

void handle_sig(int sig);
void shutdown(Source& source);



#endif