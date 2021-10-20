#include <cmath>
#include <chrono>
#include <queue>
#include <vector>

class TapDetection {
  private:
    int fs = 100;
    float acc_thr = 11;
    float da_dt_thr = 100;
    float t_tap_low = 200;
    float t_tap_high = 400;
    float ts = 1000 / 100;

    // initialize
    float curr_tot_acc = 9999;
    float last_tot_acc = 9999;
    float curr_d_tot_acc_dt = 9999;
    long unsigned int sample_number = 0;
    float last_tap_time = 0;
    std::vector<int> single_tap_queue; // for storing sample number

    bool isTap(void);

  public:
	TapDetection(void) {};
    TapDetection(int fs, float acc_thr, float da_dt_thr, float t_tap_low, float t_tap_high);
    void forward(float a_x, float a_y, float a_z);
    void singleTap_action(void);
    void doubleTap_action(void);
};

class SpinDetection {
  private:
    std::chrono::high_resolution_clock::time_point t_start, t_end;
    double t_diff;
    int total_above_threshold = 0;
    std::queue<int> values_queue;

    float magnitude_threshold = -70; // how much change in gyr_y should be sustained
    int duration_threshold = 60;     // in 1/100 of a second. how long change in gyr_y should be sustained
    float proportion_threshold =
        70 / 100;          // what % of data points should be above magnitude_threshold for duration_threshold
    float cooldown = 3000; // in ms. Cooldown after detecting a spin.

  public:
	SpinDetection(void) {};
    SpinDetection(float magnitude_threshold, int duration_threshold, float proportion_threshold, float cooldown);
    void forward(float gyr_y);
    void spin_action(void);
};

class FlipDetection {
  private:
    std::chrono::high_resolution_clock::time_point t_start, t_end;
    double t_diff;
    int total_above_threshold = 0;
    std::queue<int> values_queue;

    float magnitude_threshold = 70; // how much change in gyr_x should be sustained
    int duration_threshold = 60;    // in 1/100 of a second. how long change in gyr_x should be sustained
    float proportion_threshold =
        70 / 100;          // what % of data points should be above magnitude_threshold for duration_threshold
    float cooldown = 3000; // in ms. Cooldown after detecting a spin.

  public:
  	FlipDetection(void) {};
    FlipDetection(float magnitude_threshold, int duration_threshold, float proportion_threshold, float cooldown);
    void forward(float gyr_x);
    void flip_action(void);
};
