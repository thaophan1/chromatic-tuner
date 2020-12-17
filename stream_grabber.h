#pragma once

void stream_grabber_start();
unsigned stream_grabber_samples_sampled_captures();
void stream_grabber_wait_enough_samples(unsigned required_samples);
int stream_grabber_read_sample(unsigned which_sample);

void fill_samples(float* q, int n, int m, int incr, int sample_size);
