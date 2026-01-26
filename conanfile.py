from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain

class ParquetPadConan(ConanFile):
    name = "parquetpad"
    version = "0.1.0"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("arrow/22.0.0")

    def configure(self):
        self.options["arrow"].parquet = True
        self.options["arrow"].shared = True # Match the existing vcpkg setup

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
