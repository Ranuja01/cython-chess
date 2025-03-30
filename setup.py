"""
@author: Ranuja Pinnaduwage

This file is part of cython-chess, a Cython-optimized modification of python-chess.

Description:
This file implements the conversion of the file to a usable python module

Based on python-chess: https://github.com/niklasf/python-chess  
Copyright (C) 2025 Ranuja Pinnaduwage  
Licensed under the GNU General Public License (GPL) v3 or later.  

This program is distributed in the hope that it will be useful,  
but WITHOUT ANY WARRANTY; without even the implied warranty of  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
See the GNU General Public License for more details.  

You should have received a copy of the GNU General Public License  
along with this program. If not, see <https://www.gnu.org/licenses/>.
"""

from setuptools import setup, Extension
import sys
import subprocess
from Cython.Build import cythonize
import numpy as np

# Define the extension module
extensions = [
    Extension(
        "cython_chess", # Name of the compiled extension
        sources=["src/cython_chess_backend.cpp", "src/cython_chess.pyx"], # Source Cython file
        language="c++", # Use C++ compiler
        extra_compile_args=["-Ofast", "-march=native", "-ffast-math", 
        "-funroll-loops", "-flto", "-fomit-frame-pointer", "-std=c++20"], # Optimization flags
        define_macros=[("NPY_NO_DEPRECATED_API", "NPY_1_7_API_VERSION")], 
        include_dirs=[np.get_include()],   
    )
]

setup(
    name="cython-chess",  # Name of the package
    version="0.2",  # Version of the package
    author="Ranuja Pinnaduwage",
    author_email="Ranuja.Pinnaduwage@gmail.com", 
    description="A Cython-based chess library that optimizes the python-chess library",
    long_description=open('README.md').read(),  # Read the contents of README.md for long description
    long_description_content_type="text/markdown",  # Format of the long description (markdown)
    url="https://github.com/Ranuja01/cython-chess", 
    packages=["src"],  # List of Python packages included in the distribution
    ext_modules=cythonize(extensions),  # List of extension modules to build with Cython
    install_requires=[  # List of Python dependencies 
        "python-chess",
        "cython",
        "setuptools",
        "wheel",
    ],
    classifiers=[  # Classifiers help categorize your package
        "Programming Language :: Python :: 3",
        "Programming Language :: Cython",
        "Programming Language :: C++",
        "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
        "Operating System :: OS Independent",
        "Intended Audience :: Developers",
    ],
    python_requires=">=3.8,<=3.13.1",  # Minimum Python version required
    include_package_data=True,  # Ensure non-Python files (like README.md) are included
    zip_safe=False,  # Indicate if the package can be reliably used as a .egg file
)
