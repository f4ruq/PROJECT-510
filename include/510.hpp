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
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#define switch 0
#define client 1
#define enter_adress 2
#define server 3
#define register 4
#define login 5
#define testwindow 6


extern int my_image_width;
extern int my_image_height;
extern GLuint my_image_texture;
const std::string sentinel_code = "__::R7g!zPq$w9__";
extern ImFont* icons;
extern ImFont* main_font;
extern std::mutex globalMutex;
extern std::atomic<bool> server_exit_check;
extern std::atomic<bool> client_exit_check;
extern std::atomic<bool> server_socket_active;
extern std::atomic<bool> client_socket_active;
extern std::atomic<bool> login_successful;
extern std::atomic<bool> running;
extern std::atomic<bool> scroll;
extern std::atomic<int> current_user_index;
extern std::string client_id_str;
extern std::vector<std::string>message_log;
extern std::string response;
extern std::string* response_ptr;
extern std::thread zmq_client_funcThread;
extern std::thread zmq_server_funcThread;
extern zmq::context_t context;
extern zmq::socket_t socket;
extern zmq::message_t identity;
extern zmq::message_t request;
extern std::string* received_message_ptr;
extern int current_window;
extern SDL_Window* window;

void zmq_client_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr);

void zmq_server_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, 
     zmq::context_t& context, std::string*& received_message_ptr);

int sdl_init();

void sdl_event_check();

void window_name_switch(ImFont*& icons, ImFont*& main_font);

void window_name_adress(ImFont*& icons, ImFont*& main_font);

void window_name_client(ImFont*& icons, ImFont*& main_font);

void window_name_server(SDL_GLContext& gl_context, ImFont*& icons, ImFont*& main_font);

void window_name_register(ImFont*& icons, ImFont*& main_font);

void window_name_login(ImFont*& icons, ImFont*& main_font);

void test_window(SDL_GLContext& gl_context, ImFont*& icons, ImFont*& main_font);

void set_style();

void main_ui_func(SDL_GLContext& gl_context);

#endif
