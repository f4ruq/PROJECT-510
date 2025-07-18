#ifndef __510_HPP__
#define __510_HPP__
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <SDL.h>
#include <SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <vector>

const std::string sentinel_code = "__::R7g!zPq$w9__";
extern std::mutex globalMutex;
extern std::atomic<bool> server_exit_check;
extern std::atomic<bool> client_exit_check;
extern std::atomic<bool> server_socket_active;
extern std::atomic<bool> client_socket_active;
extern std::string client_id_str;
extern std::vector<std::string> message_log;

void zmq_client_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr);

void zmq_server_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, 
    zmq::context_t& context, std::string*& received_message_ptr);


#endif