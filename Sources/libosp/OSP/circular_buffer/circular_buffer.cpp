//======================================================================================================================
/** @file circular_buffer.cpp
 *  @author Open Speech Platform (OSP) Team, UCSD
 *  @copyright Copyright (C) 2020 Regents of the University of California Redistribution and use in source and binary
 *  forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *      1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *      following disclaimer.
 *
 *      2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *      following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//======================================================================================================================

//
// Last Modified by Dhiman Sengupta on 2020-05-04.
//

#include <OSP/circular_buffer/circular_buffer.hpp>
#include <cmath>
#include <cstring>
#include <shared_mutex>

circular_buffer::circular_buffer(uint size, float reset) {
    size_ = (uint)pow(2.0, ceil(log2((double)size)));
    buf_ = new float[size_];
    reset_ = reset;
    for (uint i = 0; i < size_; i++) {
        buf_[i] = reset_;
    }
    head_.store(0);
    mask_ = size_ - 1;
}

circular_buffer::~circular_buffer() { delete[] buf_; }

void circular_buffer::set(const float *item, uint buf_size) {
    uint head = head_.load();
    uint temp = head + buf_size - 1;
    uint remainder = (temp + 1) & mask_;
    if (temp <= mask_) {
        std::memcpy(&buf_[head], item, sizeof(float) * buf_size);
    } else {
        uint indexing = buf_size - remainder;
        std::memcpy(&buf_[head], item, sizeof(float) * indexing);
        std::memcpy(buf_, &item[indexing], sizeof(float) * remainder);
    }
    head = remainder;
    head_.store(head);
};

void circular_buffer::get(float *data, uint buf_size) {
    uint read_head = (head_.load() - buf_size) & mask_;
    uint temp = read_head + buf_size - 1;
    uint remainder = (temp + 1) & mask_;
    if (temp <= mask_) {
        std::memcpy(data, &buf_[read_head], sizeof(float) * buf_size);
    } else {
        uint indexing = buf_size - remainder;
        std::memcpy(data, &buf_[read_head], sizeof(float) * indexing);
        std::memcpy(&data[indexing], buf_, sizeof(float) * remainder);
    }
}

void circular_buffer::delay_block(float *data, uint buf_size, uint delay) {
    uint read_head = (head_.load() - buf_size - delay) & mask_;
    uint temp = read_head + buf_size - 1;
    uint remainder = (temp + 1) & mask_;
    if (temp <= mask_) {
        std::memcpy(data, &buf_[read_head], sizeof(float) * buf_size);
    } else {
        uint indexing = buf_size - remainder;
        std::memcpy(data, &buf_[read_head], sizeof(float) * indexing);
        std::memcpy(&data[indexing], buf_, sizeof(float) * remainder);
    }
}

void circular_buffer::reset() {
    for (uint i = 0; i < size_; i++) {
        buf_[i] = reset_;
    }
    head_ = 0;
}

uint circular_buffer::size() const { return size_; }
