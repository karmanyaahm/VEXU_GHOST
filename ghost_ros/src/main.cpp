/*
 * Filename: main.cpp
 * Created Date: Monday October 24th 2022
 * Author: Maxx Wilson
 * Author Email: JesseMaxxWilson@utexas.edu
 * 
 * Last Modified: Friday October 28th 2022 3:45:21 pm
 * Modified By: Maxx Wilson
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>

#include <rclcpp/rclcpp.hpp>
#include <yaml-cpp/yaml.h>

#include "globals/globals.hpp"

#include "ghost_modules/particle_filter_main.hpp"
#include "ghost_modules/serial_main.hpp"

using namespace std::literals::chrono_literals;

// Define Global Variables in shared memory
namespace globals{
    std::string repo_base_dir;
    std::atomic_bool run(true);
    std::chrono::time_point<std::chrono::system_clock> program_start_time;
}

void SignalHandler(int) {
    if(rclcpp::ok()){
        rclcpp::shutdown();
    }

    if (!globals::run) {
        printf("Force Exit.\n");
        exit(0);
    }

    printf("Exiting.\n");
    globals::run = false;
}

int main(int argc, char* argv[]){
    globals::program_start_time = std::chrono::system_clock::now();
    
    signal(SIGINT, SignalHandler);
    rclcpp::init(argc, argv);
    
    globals::repo_base_dir = std::string(getenv("HOME")) + "/VEXU_GHOST/";

    // Initialize modules
    std::thread particle_filter_thread(
        ghost_modules::ghost_particle_filter_main,
        globals::repo_base_dir + "ghost_ros/config/particle_filter.yaml"
        );

    std::thread v5_serial_interfaces(
        ghost_modules::ghost_serial_main,
        globals::repo_base_dir + "ghost_ros/config/ghost_serial.yaml"
    );

    // particle_filter_thread.join();
    v5_serial_interfaces.join();

    return 0;
}