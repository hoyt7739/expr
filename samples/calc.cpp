#include <iostream>
#include "expr_handler.h"

void handle(const expr::string_t& expr) {
    expr::handler hdl(expr);
    size_t failed_pos = 0;
    bool valid = hdl.is_valid(&failed_pos);
    std::cout << "expr: " << expr::to_utf8(expr);
    std::cout << "\nvalid: " << valid;
    if (valid) {
        std::cout << "\nresult: " << expr::to_utf8(hdl.calc().to_text());
        std::cout << "\ntree: " << expr::to_utf8(hdl.tree(4)) << std::endl;
    } else {
        std::cout << "\nfailed_pos: " << failed_pos << std::endl;
    }
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
