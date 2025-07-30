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

class users
{
    protected:
    std::string user_name;
    std::string password;

    public:
    users(std::string user_name_, std::string password_);
    
    void set_user_name(std::string user_name);
    void set_password(std::string password);
    std::string get_user_name();
    std::string get_password();
};

extern int my_image_width;
extern int my_image_height;
extern GLuint my_image_texture;
const std::string sentinel_code = "__::R7g!zPq$w9__";
extern std::mutex globalMutex;
extern std::atomic<bool> server_exit_check;
extern std::atomic<bool> client_exit_check;
extern std::atomic<bool> server_socket_active;
extern std::atomic<bool> client_socket_active;
extern std::string client_id_str;
extern std::vector<std::string> message_log;
extern std::vector<users*> user_data;
extern std::string response;
extern std::string* response_ptr;
extern std::thread zmq_client_funcThread;
extern std::thread zmq_server_funcThread;
extern zmq::context_t context;
extern zmq::socket_t socket;
extern zmq::message_t identity;
extern zmq::message_t request;
extern std::string* received_message_ptr;
extern bool running;
extern int current_window;
extern SDL_Window* window;

void zmq_client_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr);

void zmq_server_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, 
     zmq::context_t& context, std::string*& received_message_ptr);

int sdl_init();

void sdl_event_check();

void window_name_switch();

void window_name_adress();

void window_name_client();

void window_name_server(SDL_GLContext& gl_context);

void window_name_register();

void window_name_login();

ImFont* get_icon_font(ImGuiIO& io);

ImFont* get_main_font(ImGuiIO& io);

void set_style();

#endif
