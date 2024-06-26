# Available at setup time due to pyproject.toml
import os

os.environ["CC"] = "gcc"
os.environ["CXX"] = "g++"

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.0.1"
package_name = "digit_dot_tracking"

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

ext_modules = [
    Pybind11Extension(
        f"{package_name}",
        ["source/tracking_class.cpp"],
        # Example: passing in the version to the compiled code
        define_macros=[("VERSION_INFO", __version__)],
        cxx_std=17,
        include_dirs=["include"]
    ),
]

setup(
    name=f"{package_name}",
    version=__version__,
    author="Cole Ten",
    author_email="cten@ucla.edu",
    description="Track Digit dots",
    ext_modules=ext_modules,
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={"build_ext": build_ext},
    zip_safe=True,
    python_requires=">=3.10",
    setup_requires=["pybind11>=2.6"],
    package_data={f"{package_name}": ["py.typed", f"{package_name}.pyi"]}
)
