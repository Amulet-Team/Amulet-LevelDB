import sys
from setuptools import setup
from Cython.Build import cythonize
import versioneer

# Note this will error if no pyx files are present
ext_modules = cythonize(
    f"leveldb/**/*.pyx",
    language_level=3,
)

setup(
    version=versioneer.get_version(),
    cmdclass=versioneer.get_cmdclass(),
    ext_modules=ext_modules,
    package_data={
        "leveldb": [
            "*.pyi",
            "py.typed",
        ] + [
            "*.pyd",
            "*.dll",
        ] * (sys.platform == "win32") + [
            "*.so",
        ] * (sys.platform == "linux") + [
            "*.dylib",
        ] * (sys.platform == "darwin")
    }
)
