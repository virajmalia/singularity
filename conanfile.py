from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain

class SingularityConan(ConanFile):
    name = "singularity"
    version = "0.1.0"
    
    settings = "os", "compiler", "build_type", "arch"
    # No generators here, we use the generate() method instead
    
    def requirements(self):
        self.requires("libcurl/7.86.0")
        self.requires("libgit2/1.5.0")
        self.requires("gtest/1.12.1")
    
    def configure(self):
        # Use dynamic libraries
        self.options["libcurl"].shared = True
        self.options["libcurl"].with_ssl = "openssl"  # Ensure OpenSSL is used
        self.options["libgit2"].shared = True
    
    def generate(self):
        # Generate CMake toolchain with preprocessor definitions
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "17"
        tc.generate()
        
        # Generate CMake dependency files
        deps = CMakeDeps(self)
        deps.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
