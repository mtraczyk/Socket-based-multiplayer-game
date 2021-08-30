#include "client.h"

int main(int argc, char *argv[]) {
    try {
        Client c;
        c.set_parameters(argc, argv);
        c.test_parameters();
        c.run();
    } catch (std::string &s) {
        std::cerr << "Excepiton! Msg: " << s << std::endl;
        exit(1);
    }
}

