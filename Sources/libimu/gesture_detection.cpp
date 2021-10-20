#include "gesture_detection.hpp"
#include "osp_socket.hpp"

bool TapDetection::isTap() {
    /*checks current magintude values for a tap*/
    if (curr_tot_acc > acc_thr && curr_d_tot_acc_dt > da_dt_thr) {
        return true;
    }
    return false;
}

TapDetection::TapDetection(int fs, float acc_thr, float da_dt_thr, float t_tap_low, float t_tap_high)
    : fs(fs), acc_thr(acc_thr), da_dt_thr(da_dt_thr), t_tap_low(t_tap_low), t_tap_high(t_tap_high) {}

void TapDetection::forward(float a_x, float a_y, float a_z) {
    // calculate total acceleration
    curr_tot_acc = std::sqrt(a_x * a_x + a_y * a_y + a_z * a_z);
    // calc d_tot_acc__dt
    curr_d_tot_acc_dt = ((curr_tot_acc - last_tot_acc) / ts) * 1000;

    float curr_time = sample_number * ts;

    if (isTap()) {
        float delta_tap_time = curr_time - last_tap_time;
        if (delta_tap_time < t_tap_low) { // false tap (due to ossication in accel)
            ;
        } else if (delta_tap_time >= t_tap_low && delta_tap_time < t_tap_high) {
            // multiple taps
            if (single_tap_queue.size() > 0) {
                single_tap_queue.pop_back();
                doubleTap_action();
            }
            last_tap_time = curr_time;
        } else {
            single_tap_queue.push_back(sample_number);
            last_tap_time = curr_time;
        }
    }

    if (single_tap_queue.size() > 0) {
        /* wait until cool down period to confirm single tap, then raise interrupt */
        if (curr_time - last_tap_time > t_tap_high) {
            // single tap confirmed
            single_tap_queue.pop_back();
            singleTap_action();
        }
    }

    last_tot_acc = curr_tot_acc;
    sample_number++;
    return;
}

void TapDetection::singleTap_action() {
    /*Single Tap Procedure*/
    printf("Single Tap Detected\n");
    fflush(stdout);
    return;
}

void TapDetection::doubleTap_action() {
    /*Double Tap Procedure*/
    printf("Double Tap Detected\n");
    toggle_beamforming();
    fflush(stdout);
    return;
}

SpinDetection::SpinDetection(float magnitude_threshold, int duration_threshold, float proportion_threshold,
                             float cooldown)
    : magnitude_threshold(magnitude_threshold), duration_threshold(duration_threshold),
      proportion_threshold(proportion_threshold), cooldown(cooldown) {}

void SpinDetection::forward(float gyr_y) {
    int toAdd = 0;

    t_end = std::chrono::high_resolution_clock::now();
    t_diff = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    bool cooldown_passed = t_diff > cooldown;

    // If gyr_y < threshold, add 1 to the queue
    if (gyr_y < magnitude_threshold && cooldown_passed) {
        toAdd = 1;
    }

    values_queue.push(toAdd);
    total_above_threshold += toAdd;

    // If values_queue is larger than duration_threshold, pop earliest from values_queue
    if (values_queue.size() > duration_threshold) {
        total_above_threshold -= values_queue.front();
        values_queue.pop();
    }

    // If propotion of points are above threshold for the duration, recognize spin gesture
    if ((total_above_threshold / duration_threshold) > (proportion_threshold) && cooldown_passed) {
        t_start = t_end;
        spin_action();
    }
}

void SpinDetection::spin_action() {
    printf("\nSpin gesture detected \n");
    toggle_beamforming();
    fflush(stdout);
}

FlipDetection::FlipDetection(float magnitude_threshold, int duration_threshold, float proportion_threshold,
                             float cooldown)
    : magnitude_threshold(magnitude_threshold), duration_threshold(duration_threshold),
      proportion_threshold(proportion_threshold), cooldown(cooldown) {}

void FlipDetection::forward(float gyr_x) {
    int toAdd = 0;

    t_end = std::chrono::high_resolution_clock::now();
    t_diff = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    bool cooldown_passed = t_diff > cooldown;

    // If gyr_x > threshold, add 1 to the queue
    if (gyr_x > magnitude_threshold && cooldown_passed) {
        toAdd = 1;
    }

    values_queue.push(toAdd);
    total_above_threshold += toAdd;

    // If values_queue is larger than duration_threshold, pop earliest from values_queue
    if (values_queue.size() > duration_threshold) {
        total_above_threshold -= values_queue.front();
        values_queue.pop();
    }

    // If propotion of points are above threshold for the duration, recognize spin gesture
    if ((total_above_threshold / duration_threshold) > (proportion_threshold) && cooldown_passed) {
        t_start = t_end;
        flip_action();
    }
}

void FlipDetection::flip_action() {
    printf("\nFlip gesture detected \n");
    toggle_beamforming();
    fflush(stdout);
}

/////////////////////////////////
// For Testing
/////////////////////////////////

// #include <fstream>
// #include <iostream>
// #include <sstream>

// std::vector<std::vector<float> > read_csv_file(std::string file_name){
//     std::vector< std::vector <float> > data;
//
//     std::ifstream in_file;
//
//     in_file.open(file_name);
//     if(in_file.is_open()){
// 		in_file.ignore(10000,'\n');
//         std::string line;
//         while (getline(in_file,line)) {
// 			// std::cout<< line << std::endl;
//
// 			std::vector<float> res;
// 			std::stringstream ss_line(line);
// 			while( ss_line.good() ) {
// 				std::string substr;
// 				getline( ss_line, substr, ',' );
//
// 				if (substr != " "){
// 					res.push_back( std::stof(substr) );
// 				}
// 			}
// 			// for(auto i:res){
// 	        //     std::cout << i << ", ";
// 			// }
// 			// std::cout << '\n';
// 			data.push_back(res);
//         }
//     }
// 	return data;
//
// }

// int main() {
//
// 	std::string file_name = "./tap_data/sub2_double_tap_imu.csv";
// 	std::vector<std::vector<float>> data = read_csv_file(file_name);
//
// 	int fs = 50;
// 	float acc_thr = 11;
// 	float da__dt_thr = 100;
// 	float t_tap_low = 200; // msec
// 	float t_tap_high = 400; // msec
//
// 	TapDetection tap_det(fs, acc_thr, da__dt_thr, t_tap_low, t_tap_high);
//
// 	for(auto sample:data){
// 		std::vector<float> dev_sample(sample.begin()+15, sample.begin()+21);
// 		tap_det.forward(dev_sample);
// 	}
//
//
// 	return 0;
//
// }
