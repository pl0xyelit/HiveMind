#pragma once
#include <stdexcept>
#include <exception>
#include <string>
class FileOpenError : public std::runtime_error {
public:
    explicit FileOpenError(const std::string &msg) : std::runtime_error(msg) {}
};

class MapGenerationError : public std::runtime_error {
public:
    explicit MapGenerationError(const std::string &msg) : std::runtime_error(msg) {}
};