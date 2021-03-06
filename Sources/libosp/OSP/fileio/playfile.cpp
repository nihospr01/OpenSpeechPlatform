#include "playfile.h"
#include <cstring>
#include <stdint.h>
#include <string>
#include <unistd.h>

namespace params {
extern float alpha[2];
extern float audio_alpha;
} // namespace params

// file_play::file_play(const char* f,int r,int s,int re, int cp)
file_play::file_play() {
    std::shared_ptr<playfile_param_t> data_current = std::make_shared<playfile_param_t>();
    // data_current->audiofile = new AudioFile<float>();
    data_current->numSamples = 0;
    data_current->current_position_l = data_current->numSamples;
    data_current->current_position_r = data_current->numSamples;
    data_current->repeat = 0;
    data_current->finish = true;
    releasePool.add(data_current);
    std::atomic_store(&currentParam, data_current);
}

void file_play::rtmha_play(int num_sample, float *out, int channel) {
    std::shared_ptr<playfile_param_t> data_current = std::atomic_load(&currentParam);
    int playback_channel = channel;
    int current_position;

    if (data_current->finish) {
        memset(out, 0, sizeof(float) * num_sample);
        return;
    }

    if (playback_channel == 0) {
        current_position = data_current->current_position_l;
    } else {
        current_position = data_current->current_position_r;
    }
    if (data_current->mono)
        playback_channel = 0;

    if (current_position + num_sample < data_current->numSamples && !data_current->finish) {
        memcpy(out, &data_current->audiofile->samples[playback_channel][current_position], sizeof(float) * num_sample);
        current_position = current_position + num_sample;
    } else {
        int temp = current_position + num_sample - data_current->numSamples;
        int remain = data_current->numSamples - current_position;
        if (data_current->repeat == 0) {
            memcpy(out, &data_current->audiofile->samples[playback_channel][current_position], sizeof(float) * remain);
            data_current->finish = 1;
            if (params::audio_alpha > 0) {
                params::alpha[0] = saved_alpha[0];
                params::alpha[1] = saved_alpha[1];
                params::audio_alpha = 0;
            }
            std::cout << "Played" << std::endl;
        } else {
            memcpy(out, &data_current->audiofile->samples[playback_channel][current_position], sizeof(float) * remain);
            memcpy(&out[remain], &data_current->audiofile->samples[playback_channel][0], sizeof(float) * temp);
            current_position = current_position + num_sample - data_current->numSamples;
        }
    }
    if (channel == 0) {
        data_current->current_position_l = current_position;
    } else {
        data_current->current_position_r = current_position;
    }
}

void file_play::get_params(int &finish) {
    std::shared_ptr<playfile_param_t> data_current = std::atomic_load(&currentParam);
    finish = data_current->finish;
}

void file_play::set_params(std::string file_, int reset_, int repeat_, int play_) {
    std::shared_ptr<playfile_param_t> data_next;
    if (play_) {
        AudioFile<float> *audiofile = new AudioFile<float>();

        if (audiofile->load(file_) == false)
            return;

        if (params::audio_alpha > 0) {
            saved_alpha[0] = params::alpha[0];
            saved_alpha[1] = params::alpha[1];
            params::alpha[0] = params::audio_alpha;
            params::alpha[1] = params::audio_alpha;
        }
        std::cout << "Play File: " << file_ << std::endl;
        audiofile->printSummary();

        data_next = std::make_shared<playfile_param_t>();
        data_next->filename = file_;
        data_next->audiofile = audiofile;
        data_next->numSamples = audiofile->getNumSamplesPerChannel();
        data_next->mono = audiofile->isMono();
        data_next->reset = reset_;
        data_next->play = play_;
        data_next->current_position_l = 0;
        data_next->current_position_r = 0;
        data_next->finish = false;
        data_next->repeat = repeat_;
    } else {
        data_next = std::make_shared<playfile_param_t>();
        data_next->play = 0;
        data_next->finish = true;
        if (params::audio_alpha > 0) {
            params::alpha[0] = saved_alpha[0];
            params::alpha[1] = saved_alpha[1];
            params::audio_alpha = 0;
        }
    }

    releasePool.add(data_next);
    std::atomic_store(&currentParam, data_next);
    releasePool.release();
}

file_play::~file_play() {
    releasePool.release();
    // delete audiofile;
}
