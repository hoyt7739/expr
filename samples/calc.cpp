#include <iostream>
#include "expr_handler.h"

int main(int argc, char* argv[]) {
    expr::string_t expr;
    while (true) {
        std::getline(std::wcin, expr);
        if (STR("exit") == expr) {
            break;
        }

        std::wcout << "valid: " << expr::handler::check(expr) << std::endl;
        std::wcout << "result: " << expr::handler::calculate(expr).to_string() << std::endl;
    }

    return 0;
}
