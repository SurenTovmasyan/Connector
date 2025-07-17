#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// @TODO: Add implementation of reading YAML and XML files too, make it using std::variant
// Add implementation of pattern Singleton

class Config_Loader
{
public:
    Config_Loader(const std::string&);

    auto get_config() const noexcept { return _config; };
    void set_config_file(const std::string&);

private:
    nlohmann::json _config;
};