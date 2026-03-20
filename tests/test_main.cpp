#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>
#include <QApplication>

#ifdef slots
#undef slots
#endif
#include <pybind11/embed.h>
#define slots Q_SLOTS

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    pybind11::scoped_interpreter guard; // Global Python interpreter for all tests
    return Catch::Session().run(argc, argv);
}
