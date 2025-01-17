#include <iostream>
#include "expr_handler.h"

void handle(const expr::string_t& expr) {
    expr::node* nd = expr::handler::parse(expr);
    std::cout << "expr: " << expr::to_utf8(expr)
              << "\nvalid: " << (nullptr != nd)
              << "\nresult: " << expr::to_utf8(expr::handler::calculate(nd).to_string())
              << "\ntree: " << expr::to_utf8(expr::handler::tree(nd, 4))
              << std::endl;
}

int main(int argc, char* argv[]) {
    if (1 < argc) {
        handle(expr::from_utf8(argv[1]));
        return 0;
    }

    std::string expr;
    while (true) {
        std::getline(std::cin, expr);
        if ("exit" == expr) {
            break;
        }

        handle(expr::from_utf8(expr));
    }

    return 0;
}
