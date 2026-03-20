#pragma once

#include <string>
#include <vector>

namespace Slime {

/**
 * parsed the args to check for quoting and changes the args accordingly
 *
 * ARGS:
 * user_args: the string that we will parse for quoting
 *
 * RETURNS:
 * returns a vector of strings that represents the parsed string
 */
std::vector<std::string> parse_args(const std::string& user_args);

/**
 * finds if there is a redirect and returns the string that will be redirected,
 * the redirections must be done in another function
 *
 * ARGS:
 * args: vector of strings that will be parsed
 *
 * RETURNS:
 * std::string which represents the command
 * ex: ls > ex.txt hello -> ls
 */
std::string find_redirect(std::vector<std::string>& args);

/**
 * Checks if the input/command is a built in
 *
 * ARGS:
 * command: the input that will be checked if its built in
 *
 * RETURNS:
 * true if the command is a built in
 * false otherwise
 */
bool is_built_in(const std::string& command) noexcept;
};  // namespace Slime
