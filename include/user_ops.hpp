#ifndef __USER_OPS_HPP__
#define __USER_OPS_HPP__
#include <iostream>
#include <vector>

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

void create_new_user(std::string& user_name, std::string& password);

void clean_memory();

void login_(std::string& user_name, std::string& password);

extern std::vector<users*> user_data;


#endif
