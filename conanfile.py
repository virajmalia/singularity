from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain

class SingularityConan(ConanFile):
    name = "singularity"
    version = "0.1.0"
    
    settings = "os", "compiler", "build_type", "arch"
    
    def requirements(self):
        self.requires("libcurl/8.12.1")
        self.requires("libgit2/1.8.4")
        self.requires("gtest/1.16.0")
        self.requires("fmt/10.0.0")
        self.requires("nlohmann_json/3.11.3")
        self.requires("cxxopts/3.1.1")  # For command-line parsing
    
    def configure(self):
        # Use dynamic libraries
        self.options["libcurl"].shared = True
        self.options["libcurl"].with_ssl = "openssl"  # Ensure OpenSSL is used
        self.options["libgit2"].shared = True
    
    def generate(self):
        # Generate CMake toolchain with preprocessor definitions
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "17"
        tc.variables["BUILD_SHARED_LIBS"] = True  # Build shared libraries
        tc.generate()
        
        # Generate CMake dependency files
        deps = CMakeDeps(self)
        deps.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
