## 

### Thinking in target (80/20) 
- `add_library(ts_engine ...)` creates a library target
- `add_executable(lobd ...)` creates an executable target
- `target_link_libraries(lobd PRIVATE ts_engine)` links them
- `target_include_directories(ts_common PUBLIC ...)` sets include paths

### 20
Create target:
- `add_library(name ...)`
- `add_executable(name ...)`

Add source files:
- put them in `add_library/add_executable(...)`
- or `target_sources(name PRIVATE file.cpp ...)`

Include paths:
- `target_include_directories(name PUBLIC path)`

Link dependencies:

- target_link_libraries(name PRIVATE other_target)

Compiler options/defines:
- `target_compile_options(name PRIVATE -Wall ...)`
- `target_compile_definitions(name PRIVATE TS_FOO=1)`

Third-party deps:
- `FetchContent_Declare(...)` + `FetchContent_MakeAvailable(...)`

### Debugging builds (super high ROI)

See actual compile commands:

`cmake --build build -v`

Build one target:

`cmake --build build --target unit_tests`

Run tests:

`ctest --test-dir build`

That’s all you need until you’re polishing.