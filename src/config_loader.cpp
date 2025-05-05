#include "../includes/config_loader.h"

Config_Loader::Config_Loader(const std::string& file_path_):
    _config()
{
    set_config_file(file_path_);
}

void Config_Loader::set_config_file(const std::string& file_path_){
    std::ifstream fin_(file_path_);
    
    if(!fin_.is_open()){
        throw std::runtime_error("Wrong path to config.json or just deleted");
    }

    fin_ >> _config;
}