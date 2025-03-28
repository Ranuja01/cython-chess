"""
@author: Ranuja Pinnaduwage

This file is part of cython-chess, a Cython-optimized modification of python-chess.

Description:
This file runs when the module is imported to initialize it

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
import importlib.util
print("AAAA")
# Check if cython_chess module is available before importing
spec = importlib.util.find_spec(".Cython_Chess", package=__name__)
if spec is not None:
    cython_chess = importlib.import_module(".Cython_Chess", package=__name__)
    cython_chess.initialize()  # Call the function safely after import
else:
    raise ImportError("Cython module 'cython_chess' not found. Did you compile it?")

