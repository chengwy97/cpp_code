#include <CLI/CLI.hpp>
#include <iostream>
#include <iox/cli_definition.hpp>
#include <iox/string.hpp>

struct CommandLine {
    IOX_CLI_DEFINITION(CommandLine);
    IOX_CLI_OPTIONAL(iox::string<100>, stringValue, "default value", 's', "string-value",
                     "some description");
    IOX_CLI_REQUIRED(int, intValue, 'i', "int-value", "some description");
    IOX_CLI_SWITCH(boolValue, 'b', "bool-value", "some description");
};

int main(int argc, char** argv) {
    auto cmd = CommandLine::parse(argc, argv, "My program description");
    std::cout << cmd.binaryName() << std::endl;
    std::cout << cmd.stringValue() << " " << cmd.intValue() << " " << cmd.boolValue() << std::endl;
    return 0;
}
