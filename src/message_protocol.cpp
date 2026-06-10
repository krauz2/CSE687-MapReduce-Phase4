#include "message_protocol.h"

#include <sstream>
#include <vector>

namespace {
    std::vector<std::string> split(const std::string& value, char delimiter) {
        std::vector<std::string> parts;
        std::stringstream stream(value);
        std::string item;

        while (std::getline(stream, item, delimiter)) {
            parts.push_back(item);
        }

        return parts;
    }
}

std::string build_message(
    const std::string& command,
    const std::string& role,
    const std::map<std::string, std::string>& fields) {

    std::ostringstream stream;
    stream << command;
    if (!role.empty()) {
        stream << "|" << role;
    }

    for (const auto& field : fields) {
        stream << "|" << field.first << "=" << field.second;
    }

    return stream.str();
}

ParsedMessage parse_message(const std::string& line) {
    ParsedMessage result;
    const auto parts = split(line, '|');

    if (parts.empty()) {
        return result;
    }

    result.command = parts[0];
    if (parts.size() >= 2) {
        result.role = parts[1];
    }

    for (std::size_t i = 2; i < parts.size(); ++i) {
        const std::string& token = parts[i];
        const std::size_t eq_pos = token.find('=');

        if (eq_pos == std::string::npos) {
            continue;
        }

        const std::string key = token.substr(0, eq_pos);
        const std::string value = token.substr(eq_pos + 1);
        result.fields[key] = value;
    }

    result.valid = true;
    return result;
}

std::string get_field(
    const ParsedMessage& message,
    const std::string& key,
    const std::string& default_value) {

    const auto it = message.fields.find(key);
    if (it == message.fields.end()) {
        return default_value;
    }
    return it->second;
}

int get_int_field(
    const ParsedMessage& message,
    const std::string& key,
    int default_value) {

    const auto it = message.fields.find(key);
    if (it == message.fields.end()) {
        return default_value;
    }

    try {
        return std::stoi(it->second);
    }
    catch (...) {
        return default_value;
    }
}
