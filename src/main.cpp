/*
clang++ final.cpp 510.cpp \
imgui/imgui*.cpp \
imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_opengl2.cpp \
-I/opt/homebrew/include -L/opt/homebrew/lib -lzmq -Iimgui -Iimgui/backends \
-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
-L/opt/homebrew/lib \
-lSDL2 -framework OpenGL \
-std=c++17 -o final
*/

#include "510.hpp"

int main()
{
    socket.setsockopt(ZMQ_LINGER, 0);
    sdl_init();

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 
    ImGuiIO& io = ImGui::GetIO();
    
    ImFontConfig cfg;
    cfg.SizePixels = 100.0f;

    //ImFont* icons = io.Fonts->AddFontFromFileTTF("assets/FontAwesome6Free-Regular-400.otf", 20.0f, &icons_config, icons_ranges);
    ImFont* default_font = io.Fonts->AddFontFromFileTTF("assets/Inter_28pt-Regular.ttf", 18.0f);
    ImFont* bigg_font = io.Fonts->AddFontFromFileTTF("assets/Roboto_Condensed-Regular.ttf", 100.0f);
    ImFont* big_font = io.Fonts->AddFontFromFileTTF("assets/BitcountGridSingle_Cursive-Regular.ttf", 1.0f, &cfg);

    set_style();
    // backend init
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    while (running) 
    {
        sdl_event_check();
        
        // start imgui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if(current_window == switch){window_name_switch();}
        else if(current_window == enter_adress){window_name_adress();}
        else if(current_window == client){window_name_client();}
        else if(current_window == server){window_name_server(gl_context);}
        
        // rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData()); 
        SDL_GL_SwapWindow(window);
    }
    
    //cleanup
    server_socket_active = 0;
    client_socket_active = 0;
    client_exit_check = 1;
    server_exit_check = 1;
    socket.close();
    context.shutdown();
    context.close();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    if(zmq_client_funcThread.joinable()){zmq_client_funcThread.join();}
    if(zmq_server_funcThread.joinable()){zmq_server_funcThread.join();}
    return 0;
}
