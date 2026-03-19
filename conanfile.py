from conan import ConanFile
from conan.tools.cmake import cmake_layout


class CodeHexConan(ConanFile):
    name = "codehex"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = ["CMakeDeps", "CMakeToolchain"]

    def requirements(self):
        self.requires("nlohmann_json/3.11.3")
        self.requires("sol2/3.3.0")
        self.requires("lua/5.4.6")
        self.requires("pybind11/2.11.1")
        self.requires("catch2/3.4.0")

    def layout(self):
        cmake_layout(self)
