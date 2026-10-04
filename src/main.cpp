#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ast.hpp"
#include "codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"

namespace {

void printUsage(const char* prog) {
    std::cerr
        << "Amethyst compiler\n"
        << "Usage: " << prog << " [options] <file.amt>\n"
        << "Options:\n"
        << "  -o <path>    output file (default: a.out)\n"
        << "  -S           emit assembly only (.s), do not assemble/link\n"
        << "  --emit-asm   write intermediate .s next to output (for debug)\n";
}

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "error: cannot open '" << path << "'\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool runCommand(const std::string& cmd) {
    int rc = std::system(cmd.c_str());
    return rc == 0;
}

std::string replaceExtension(const std::string& path, const std::string& ext) {
    size_t slash = path.find_last_of("/\\");
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return path + ext;
    }
    return path.substr(0, dot) + ext;
}

}  // namespace

int main(int argc, char** argv) {
    std::string input;
    std::string output = "a.out";
    bool asmOnly = false;
    bool keepAsm = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-S") {
            asmOnly = true;
        } else if (arg == "--emit-asm") {
            keepAsm = true;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "error: -o requires a path\n";
                return 1;
            }
            output = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "error: unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            return 1;
        } else {
            if (!input.empty()) {
                std::cerr << "error: multiple input files (only one supported)\n";
                return 1;
            }
            input = arg;
        }
    }

    if (input.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    std::string source = readFile(input);

    try {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        Program program = parser.parseProgram();

        Sema sema;
        sema.analyze(program);
        for (const auto& w : sema.warnings()) {
            std::cerr << input << ":" << w.line << ":" << w.col
                      << ": warning: " << w.msg << "\n";
        }

        Codegen codegen;
        std::string asmText = codegen.emit(program);

        std::string asmPath =
            asmOnly ? output : replaceExtension(input, ".s");

        {
            std::ofstream out(asmPath, std::ios::binary);
            if (!out) {
                std::cerr << "error: cannot write '" << asmPath << "'\n";
                return 1;
            }
            out << asmText;
        }

        if (asmOnly) {
            std::cout << asmPath << "\n";
            return 0;
        }

        std::string objPath = replaceExtension(input, ".o");
        std::string asCmd = "as --64 -o " + objPath + " " + asmPath;
        if (!runCommand(asCmd)) {
            std::cerr << "error: assembler failed: " << asCmd << "\n";
            return 1;
        }

        // Link with the system linker driver (ld + crt + libc for printf).
        std::string ldCmd = "gcc -no-pie -o " + output + " " + objPath;
        if (!runCommand(ldCmd)) {
            std::cerr << "error: linker failed: " << ldCmd << "\n";
            return 1;
        }

        if (!keepAsm) {
            std::remove(asmPath.c_str());
            std::remove(objPath.c_str());
        }

        return 0;
    } catch (const LexError& e) {
        std::cerr << input << ":" << e.line << ":" << e.col
                  << ": error: " << e.what() << "\n";
    } catch (const ParseError& e) {
        std::cerr << input << ":" << e.line << ":" << e.col
                  << ": error: " << e.what() << "\n";
    } catch (const SemaError& e) {
        std::cerr << input << ":" << e.line << ":" << e.col
                  << ": error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "internal error: " << e.what() << "\n";
    }
    return 1;
}
