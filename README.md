## About This Project  
This project is a heavily optimized Cython version of [`python-chess`](https://github.com/niklasf/python-chess). It implements the same move generation functionality but with
performance improvements using Cython and C++. Since `python-chess` is licensed under **GPL v3**,
this project is also licensed under **GPL v3**.

Cython is a programming language that serves as a bridge between Python and C/C++. It allows you to
write Python-like code that can be compiled into highly efficient C/C++ code. This makes it an ideal
tool when you want to optimize the performance of certain Python programs while still leveraging the
simplicity of Python. Cython also allows you to invoke custom C++ code to further boost speed by bypassing
Python alltogether. In this project, I rewrote functions pertaining to move generation from the
`python-chess` using Cython and C++. I use Cython to interact with Python objects from `python-chess` such as
chess.Board or chess.Move objects but try to seperate code that involves bitmask parsing and other such
operations into a C++ file for even more speed.

I may in the future look to optimize other portions of the `python-chess` library but as it stands,
the overhead of calling Cython functions for small functions such as is_capture seem to be more costly
than simply calling the same functions from `python-chess`. Move generation however is somewhat lengthy and therefore
the Cythonized code (as well as the C++ injections) can give around a **40% improvement** in move generation.

*Note:* This project includes C++ components and therefore requires the ability to compile C++ as well as Python.
When pip installing the package, the required Python components come with it but you need to ensure you
have the ability to compile the C++ files. To do this ensure you follow the build requirements below.

## Build Requirements
Ensure you have Python 3.8 or greater installed on your system along with an udpated version of pip.
When installing this package, all required Python dependencies will be installed alongside it.
To ensure full functionality, confirm that you have a C++ compiler installed. If not, use the following
instructions to download one.

### Windows  
- Download and install **Microsoft Visual C++ Build Tools 2022** or later from:  
  [https://visualstudio.microsoft.com/visual-cpp-build-tools/](https://visualstudio.microsoft.com/visual-cpp-build-tools/)  
- Ensure you select the **C++ CMake tools for Windows** and **MSVC compiler** during installation.
- To verify you have correctly installed the C++ compiler run this in the Developer Command Prompt (not the regular command prompt):
  ```sh
  cl
  ```
- *Note* that simply installing MSVC build tools may not initialize your system properly. You may have to set environment
  variables or update your path with the cl.exe compiler.
### Linux  
- Install the necessary C++ compiler and build tools:
  - For Ubuntu: 
  ```sh
  sudo apt update && sudo apt install build-essential
  ```
  - For Fedora:
  ```sh
  sudo dnf install gcc-c++ make
  ```
- To verify you have correctly installed the C++ compiler run this in a terminal:
  ```sh
  g++ --version 
  ```
### For MacOS
- Install Apple's command-line developer tools (includes clang for C++):
  
  ```sh
  xcode-select --install
  ```
- To verify you have correctly installed the C++ compiler run this in a terminal:
  ```sh
  g++ --version 
  ```

## Download
To download this package, use the following instructions:
  ```sh
  pip install cython-chess
  ```
If this does not work, then simply:
  ```sh
  pip install git+https://github.com/Ranuja01/cython-chess.git
  ```

### **Example Usage:**
To use the module, simply import cython-chess as seen below:
```python
import cython_chess
```
The module will self initialize, allowing you to use it automatically.

Since this module currently only improves the move generation speed, it still relies on the board objects from the original `python-chess`
library. The following example demonstrates how to use the `python-chess` board object for move generation while utilizing the optimized
cython-chess library to speed up the process. The start and end squares for move generation are defined using bitmasks from the original
`python-chess` library:
```python
import cython_chess
import chess

board = chess.Board()
cython_chess.generate_legal_moves(board,chess.BB_ALL,chess.BB_ALL)
```

The following code can be found in the example usage file. Run this script to test the speed improvements.
*Note:* You may need to pip install timeit if not already done so for this
```python
from timeit import default_timer as timer
import cython_chess
import chess

board = chess.Board()

## SPEED COMPARISON
t0= timer()
for i in range (100000):
    for move in board.generate_legal_moves():
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)

t0= timer()
for i in range (100000):
    for move in cython_chess.generate_legal_moves(board,chess.BB_ALL,chess.BB_ALL):
        pass
t1 = timer()
print("Time elapsed: ", t1 - t0)
```
