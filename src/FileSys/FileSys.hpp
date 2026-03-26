#pragma once

#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <string>
#include <vector>

#include "Commands/AutoComplete.hpp"
#include "Commands/BuiltInCommand.hpp"
#include "ShellHelper/ShellHelper.hpp"

namespace Slime {

/**
 * Executes built in shell functions
 *
 * ARGS:
 * inputs: reference to the parsed inputs
 */
void execb(std::vector<std::string>& inputs);

/**
 * Executes non built in shell functions
 *
 * ARGS:
 * inputs: reference to the parsed inputs
 */
void execnb(std::vector<std::string>& inputs);

/**
 * Executes both built in and non built in. used for piping
 */
void execa(std::vector<std::string>& inputs);

/**
 * Returns a the directories tied with the given path
 *
 * ARGS:
 * char_path: the path we want to find
 *
 * RETURNS:
 * vector of strings that represents the path
 */
std::vector<std::string> get_directories(const char* char_path);

/**
 * checks the file permissions with the given user
 *
 * ARGS:
 * path: file system path of the file to be checked
 *
 * RETURNS:
 * true - the user has permission to use the file
 * false - the user doesnt have permission to use the file
 */
bool check_file_permission_status(const std::filesystem::path& path);

/**
 * checks where the given command is within the given path (sub-paths), also
 * checks if you have permission
 *
 * ARGS:
 * command: the command we are trying to find in path
 * path: the path we are trying to find the path in
 *
 * RETURNS:
 * returns a std::string of the full path if found, else it returns ""
 *
 */
std::string find_in_path(const std::string& command, const char* path);

/**
 * Checks if the given command (non built in) is executable
 *
 * ARGS:
 * command: the command trying to be executed
 *
 * RETURNS:
 * if the command is executable then true, else false
 */
bool is_executable(const std::string& command);

/**
 * finds the command in the file system
 *
 * ARGS:
 * command: the command we are attempting to buy
 *
 * RETURNS:
 * the full string of the path, "" if not found
 */
std::string find_in_file_system(const std::string& command) noexcept;

std::vector<std::string> find_all_execnb();

void insert_files_in_trie();

}  // namespace Slime
