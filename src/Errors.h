#pragma once
#include <stdexcept>
#include <exception>
class FileOpenError : public std::runtime_error {
public:
    explicit FileOpenError(const std::string &msg) : std::runtime_error(msg) {}
};