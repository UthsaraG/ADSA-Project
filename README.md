# ADSA Problem C - Smart City Transport System


| Member | Work |
| --- | --- |
| 1 - Hemsara | Shared graph, city data, bus network, Part I entry point |
| 2 - Dulhan | Train network (Part II) |
| 3 - Supun | Variable passenger demand (Part III) |
| 4 - Ayas|  Shortest paths and efficiency profiling (Part IV) |

## Run in Visual Studio Code

1. Open this repository folder in **Visual Studio Code** (File > Open Folder).
2. Have a C++17 compiler available: Apple Clang on macOS; GCC/G++ on Linux or
   Windows (for example, an existing MinGW-w64 installation on Windows).
3. Press **Ctrl+Shift+B** (**Cmd+Shift+B** on macOS) to build.
4. Select **Terminal > Run Task > Run transport** to use the interactive menu.
5. Select **Run Part I demo** for a reproducible demonstration, or **Run bus tests**
   for the automated checks. Select **Run train tests** or **Run Part II demo**
   for the train module.

The checked-in tasks use Clang on macOS and `g++` elsewhere. All use C++17 and
compile the group source files. The Microsoft C/C++ extension is recommended for
editing and F5 debugging; it is not required to build or run. F5 also requires
LLDB on macOS or GDB on Windows/Linux, available to the extension.

If you use the installed **Code Runner** extension's **Run Code** button on
macOS, the workspace settings also build the complete project and launch the
interactive menu in the terminal. Open the repository folder, then run any
project `.cpp` file. Compiling `bus_network.cpp` alone cannot produce an
executable: it needs the graph implementation and `main.cpp`. The build/run
tasks above remain available on all supported platforms.

Member 3 can use `planTrip` and `vehiclesAt` on either network. Member 4 should
use the combined `train.graph()` for multimodal routing and respect the agreed
transfer policy using `Graph::canTransfer(id, fromMode, toMode)`.
`Graph::transferStops()` returns the designated stops that have both bus and
train edges. Include `train_network.h` alongside `bus_network.h` when using
these APIs.
