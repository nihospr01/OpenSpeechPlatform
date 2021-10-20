/**
 * Author:    Anshuman Dewangan <adewangan@ucsd.edu>
 * Created:   04/30/2021
 **/

#include <arpa/inet.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#define PORT 8001

std::string send_messsage(json json_messsage);
std::string get_messsage(void);
void toggle_beamforming(void);
