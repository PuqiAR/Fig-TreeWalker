add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

set_policy("run.autobuild", false)

target("Fig")
    set_kind("binary")
    set_languages("c++2b") 
    
    set_plat("mingw")
    --set_toolchains("clang")

    add_cxxflags("-static")
    add_cxxflags("-stdlib=libc++")

    add_files("src/main.cpp")
    add_files("src/Core/warning.cpp")
    add_files("src/Evaluator/evaluator.cpp")
    add_files("src/Lexer/lexer.cpp")
    add_files("src/Parser/parser.cpp")
    add_files("src/Value/value.cpp")
    
    add_includedirs("src")

    set_warnings("all")

    add_defines("__FCORE_COMPILE_TIME=\"" .. os.date("%Y-%m-%d %H:%M:%S") .. "\"")