/**
 * Author:    Satyam Gaba <sgaba@ucsd.edu>
 * Created:   06/10/2020
 * 
 **/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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
#include <assert.h> 
#include "libimu_new.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define IIO_ENSURE(expr) { \
	if (!(expr)) { \
		(void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
		(void) abort(); \
	} \
}

class Source;
void shutdown(Source& source);

Source::Source(struct info& libimu_info, const char** dev_names, const char** trigger_name, imu_device* devices)
	: dev_names(dev_names),
		trigger_name(trigger_name),
		num_dev(libimu_info.num_dev),
		buffer_length(libimu_info.buffer_length)
{	
	int total_channel_count = 0;
	this->devices = devices;
	// devices = new imu_device[num_dev];
	for(int i = 0; i<num_dev; i++){
		devices[i].dev_name = this->dev_names[i];
		devices[i].trigger_name = this->trigger_name[i];
		std::cout << devices[i].dev_name<< " ";
	}
	
	has_repeat = version_check();
		printf("* Acquiring IIO context\n");
	
	for(int i = 0; i<num_dev; i++){
		IIO_ENSURE((devices[i].ctx = iio_create_default_context()) && "No context");
		IIO_ENSURE(iio_context_get_devices_count(devices[i].ctx) > 0 && "No devices");
	}

	for(int i = 0; i<num_dev; i++){
		printf("* Acquiring device %s\n", dev_names[i]);
		devices[i].dev = iio_context_find_device(devices[i].ctx, devices[i].dev_name);
		if (!devices[i].dev) {
			perror("No device found");
			shutdown(*this);
		}
	}

	printf("* Initializing IIO streaming channels:\n");
	for (int i = 0; i< num_dev; i++){
		for (int j = 0; j < iio_device_get_channels_count(devices[i].dev); ++j) {
			struct iio_channel *chn = iio_device_get_channel(devices[i].dev, j);
			if (iio_channel_is_scan_element(chn)) {
				printf("%s, ", iio_channel_get_id(chn));
				devices[i].channel_count++;
			}
		}
		printf("\n");
		if (devices[i].channel_count == 0) {
			printf("No scan elements found in device: %d\n", i);
			shutdown(*this);
		}

		devices[i].channels = (iio_channel **)calloc(devices[i].channel_count, sizeof *devices[i].channels);
		if (!devices[i].channels) {
			perror("Channel array allocation failed");
			shutdown(*this);
		}
		for (int j = 0; j < devices[i].channel_count; ++j) {
		struct iio_channel *chn = iio_device_get_channel(devices[i].dev, j);
		if (iio_channel_is_scan_element(chn))
			devices[i].channels[j] = chn;
		}
	}

	for (int i = 0; i< num_dev; i++){
		printf("* Acquiring trigger %s for device: %d \n", devices[i].trigger_name, i);
		devices[i].trigger = iio_context_find_device(devices[i].ctx, devices[i].trigger_name);
		if (!devices[i].trigger || !iio_device_is_trigger(devices[i].trigger)) {
			perror("No trigger found (try setting up the iio-trig-hrtimer module)");
			shutdown(*this);
		}
		
		printf("* Enabling IIO streaming channels for buffered capture\n");
		for (int j = 0; j < devices[i].channel_count; ++j)
			iio_channel_enable(devices[i].channels[j]);

		printf("* Enabling IIO buffer trigger\n");
		if (iio_device_set_trigger(devices[i].dev, devices[i].trigger)) {
			perror("Could not set trigger\n");
			shutdown(*this);
		}

		printf("* Creating non-cyclic IIO buffers with %d samples\n", buffer_length);
		devices[i].rxbuf = iio_device_create_buffer(devices[i].dev, buffer_length, false);
		if (!devices[i].rxbuf) {
			perror("Could not create buffer");
			shutdown(*this);
		}
	}

	for (int i=0; i<num_dev; i++){
		devices[i].has_ts = strcmp(iio_channel_get_id(devices[i].channels[devices[i].channel_count-1]), "timestamp") == 0;
	}

	// total channel count
	for(int i=0; i<num_dev; i++){
		total_channel_count += devices[i].channel_count;
	}
	std::cout << "* Number of IIO Channels: " << total_channel_count << std::endl;
}

bool Source::version_check(){
			unsigned int i, j, major, minor;
			char git_tag[8];
			iio_library_get_version(&major, &minor, git_tag);
			printf("IIO Library version: %u.%u (git tag: %s)\n", major, minor, git_tag);

			/* check for struct iio_data_format.repeat support
			 * 0.8 has repeat support, so anything greater than that */
			has_repeat = ((major * 10000) + minor) >= 8 ? true : false;
			return has_repeat;
		}

void Source::forward(std::vector<float>& send_sample){
	for (int i=0; i<num_dev; i++){
		ssize_t nbytes_rx;
		/* we use a char pointer, rather than a void pointer, for p_dat & p_end
		* to ensure the compiler understands the size is a byte, and then we
		* can do math on it.
		*/
		char *p_dat, *p_end;
		ptrdiff_t p_inc;
		int64_t now_ts;

		// Refill RX buffer
		nbytes_rx = iio_buffer_refill(devices[i].rxbuf);
		if (nbytes_rx < 0) {
			printf("Error refilling buf: %d\n", (int)nbytes_rx);
			shutdown(*this);
		}

		p_inc = iio_buffer_step(devices[i].rxbuf);
		p_end = (char *)iio_buffer_end(devices[i].rxbuf);

		// Print timestamp delta in ms
		if (devices[i].has_ts)
			for (p_dat = (char *)iio_buffer_first(devices[i].rxbuf, devices[i].channels[devices[i].channel_count-1]); p_dat < p_end; p_dat += p_inc) {
				now_ts = (((int64_t *)p_dat)[0]);
				int64_t latency = last_ts > 0 ? (now_ts - last_ts)/1000/1000 : 0;
				// fprintf(stderr, "[+%04" PRId64 "ms] ", last_ts > 0 ? (now_ts - last_ts)/1000/1000 : 0);
				// fprintf(stderr, "[+%04" PRId64 "ms] ", latency);
				send_sample.push_back((float)latency);
				last_ts = now_ts;
			}

		// Print each captured sample
		for (int j = 0; j < devices[i].channel_count-1; ++j) {
			const struct iio_data_format *fmt = iio_channel_get_data_format(devices[i].channels[j]);
			unsigned int repeat = has_repeat ? fmt->repeat : 1;

			// fprintf(stderr, "%s ", iio_channel_get_id(devices[i].channels[j]));
			for (p_dat = (char *)iio_buffer_first(devices[i].rxbuf, devices[i].channels[j]); p_dat < p_end; p_dat += p_inc) {
				for (int k = 0; k < repeat; ++k) {
					if (fmt->length/8 == sizeof(int16_t)) {
						if (fmt->with_scale) {
							double val = fmt->scale * ((int16_t *)p_dat)[k];
							// fprintf(stderr, "%f ", val);
							send_sample.push_back((float)val); 
						} else {
							// fprintf(stderr, "%" PRIi16 " ", ((int16_t *)p_dat)[k]);
							send_sample.push_back( (float)((int16_t *)p_dat)[k] ); 

						}
					} else if (fmt->length/8 == sizeof(int64_t)){
						// fprintf(stderr, "%" PRId64 " ", ((int64_t *)p_dat)[k]);
						send_sample.push_back( (float)((int64_t *)p_dat)[k] ); 
					}
				}
			}
		}
	}
}



Processing::Processing(struct info& libimu_info){
	this->num_dev = libimu_info.num_dev;
	this->data_filter_level = libimu_info.data_filter_level;


	if (this->data_filter_level == ALL){
		out_num_channel = 7*num_dev; // latency + 3 acc + 3 gyr
	}
	else{
		out_num_channel = 4*num_dev;
	}
}
void Processing::forward(std::vector<float>&data){
	/*			0
	data = ["sample_latency1","wx1","wy1","wz1","tot_accel_x_1","tot_accel_y_1","tot_accel_z_1",
				7
			"sample_latency2","wx2","wy2","wz2","tot_accel_x_2","tot_accel_y_2","tot_accel_z_2",
				14
			"sample_latency3","wx3","wy3","wz3","tot_accel_x_3","tot_accel_y_3","tot_accel_z_3"]
	*/

	if (this->data_filter_level == ALL){
		// both gyro and acc
	}
	else if (this->data_filter_level == GYRO){
		// only gyro
		for (int d=num_dev-1; d>=0; d--){
			data.erase(data.begin()+(d*7)+4, data.begin()+(d*7)+7);
		}
	}
	else{
		// only acc
		for (int d=num_dev-1; d>=0; d--){
			data.erase(data.begin()+(d*7)+1, data.begin()+(d*7)+4);
		}
	}
}


Sink::Sink(struct info& libimu_info, Source& source, Processing& process):
	num_channels(process.out_num_channel+1),
	is_lsl(libimu_info.is_lsl),
	lsl_stream_name(libimu_info.lsl_stream_name),
	lsl_stream_type(libimu_info.lsl_stream_type),
	lsl_info(lsl::stream_info(lsl_stream_name,lsl_stream_type, this->num_channels)),
	lsl_outlet(lsl::stream_info(this->lsl_info)),
	source(source)
	{
	//stream through lsl or save to file system
	// initial sample number
	sample_number = 0;

	if(this->is_lsl){
		//stream over network using lsl
		printf("* Initializing lsl outlet with %d channels\n", this->num_channels);
	}
	else{
		//store in local file system
		std::string out_file_name = libimu_info.out_name;
		out_file.open(out_file_name);
		std::cout << "Writing data to " << out_file_name << std::endl;
		//create header for output file
		std::string header = "";
		for(int i=0; i<libimu_info.num_dev; i++){
			header += "latency_" + std::to_string(i) + std::string(",");
			if (libimu_info.data_filter_level == GYRO || libimu_info.data_filter_level == ALL){
				for (auto c: {"x","y","z"}){
					header += std::string("gyro_") + std::string(c) + std::string("_") + std::to_string(i) + std::string(",");
				}
			}
			if (libimu_info.data_filter_level == ACCEL || libimu_info.data_filter_level == ALL){
				for (auto c: {"x","y","z"}){
					header += std::string("acc_") + std::string(c) + std::string("_") + std::to_string(i) + std::string(",");
				}
			}
		}
		header += std::string("sample_number");
		// std::cout << header<< std::endl;
		out_file << header << '\n';
	}
}
void Sink::forward(std::vector<float>& data){
	if(is_lsl){
		try{
			// std::cout << "lsl" <<std::endl;
			if(lsl_outlet.have_consumers()){
				have_cons = true;
				sample_number += 1;
				data.push_back(sample_number);
				lsl_outlet.push_sample(data);
				if (have_cons == true && last_cons == false){
					printf("Sending packets to client...\n");
				}
				last_cons = have_cons;
				look_cons = true;
			} else{
				have_cons = false;
				if (!last_cons && look_cons){
					printf("Looking for LSL consumer...\n");
					look_cons = false;
				}
				if (last_cons){
					printf("LSL consumer lost...\n");
				}
			}
			last_cons = have_cons;
		} catch (std::exception& e) {
			std::cerr << "Got an exception in sending samples with lsl: " << e.what() << std::endl;
			std::cout << "Press any key to exit. " << std::endl;
			std::cin.get();
			shutdown(source);
		}
	}

	else{
		if(out_file.is_open()){
			sample_number += 1;
			data.push_back(sample_number);
			std::string out_str = "";
			int vsize = (int)data.size();
			for (int i=0; i<vsize; i++){
				out_str += std::to_string(data[i]) + std::string(", ");
			}
			// write to string comma-seperated
			out_file << out_str << '\n';
		}
		else{
			std::cout << "output file is closed!" << std::endl;
		}
	}
	data.clear();
}


Bind::Bind(Source& source, Processing& processing, Sink& sink, bool& stop){
	this->source = &source;
	this->processing = &processing;
	this->sink = &sink;
}
	
void Bind::fetch_data(bool& stop){
	printf("* Starting IO streaming (press CTRL+C to cancel)\n");
	while(!stop){
		std::vector<float> data;
		(this->source)->forward(data);
		(this->processing)->forward(data);
		(this->sink)->forward(data);
	}
	printf("* Stopped IO streaming\n");
	shutdown(*source);
}


/* cleanup and exit */
void shutdown(Source& source)
{
	int ret;
	for (int i =0; i < source.num_dev; i++){
		if (source.devices[i].channels) { free(source.devices[i].channels); }

		printf("* Destroying buffers %d\n", i);
		if (source.devices[i].rxbuf) { iio_buffer_destroy(source.devices[i].rxbuf); }

		printf("* Disassociate trigger %d\n", i);
		if (source.devices[i].dev) {
			ret = iio_device_set_trigger(source.devices[i].dev, NULL);
			if (ret < 0) {
				char buf[256];
				iio_strerror(-ret, buf, sizeof(buf));
				fprintf(stderr, "%s (%d) while Disassociate trigger %d\n", buf, ret, i);
			}
		}

		printf("* Destroying context %d\n", i);
		if (source.devices[i].ctx) { iio_context_destroy(source.devices[i].ctx); }
	}
	exit(0);
}
