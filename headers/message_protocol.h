#pragma once

#include <map>
#include <string>

struct ParsedMessage {
    std::string command;
    std::string role;
    std::map<std::string, std::string> fields;
    bool valid = false;
};

std::string build_message(
    const std::string& command,
    const std::string& role,
    const std::map<std::string, std::string>& fields);

ParsedMessage parse_message(const std::string& line);

std::string get_field(
    const ParsedMessage& message,
    const std::string& key,
    const std::string& default_value = "");

int get_int_field(
    const ParsedMessage& message,
    const std::string& key,
    int default_value = 0);
