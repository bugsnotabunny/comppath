set_languages("c++20")
set_warnings("all", "extra", "pedantic")

local comppath_version_option = "comppath-version"

option(comppath_version_option)
    set_showmenu(true)
    set_description("Comppath version to test with")

    on_check(function ()
        local version = get_config(comppath_version_option)
        if not version or version == "" then
            raise(comppath_version_option .. " option is required to be set")
        end
    end)
option_end()

local version = get_config(comppath_version_option)
if not version then
    version = ""
end

local branch = version
if version == "experimental" then
    branch = "master"
end

add_repositories("comppath-repo https://github.com/bugsnotabunny/comppath.git " .. branch)
add_requires("comppath " .. version, { external = true })

target("test")
    set_kind("binary")
    add_packages("comppath")
    add_files("main.cpp")
target_end()
