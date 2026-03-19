add_library(codehex_scripting STATIC
    ScriptEngine.h
    HookRegistry.h   HookRegistry.cpp
    ScriptManager.h  ScriptManager.cpp
    lua/LuaEngine.h  lua/LuaEngine.cpp
    python/PythonEngine.h python/PythonEngine.cpp
)

target_include_directories(codehex_scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codehex_scripting PUBLIC
    Qt6::Core
    lua::lua
    sol2::sol2
    pybind11::embed
)
