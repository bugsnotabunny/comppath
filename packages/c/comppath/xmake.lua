package("comppath")
    set_description("comppath - Compile-time reflection over filesystem paths")
    set_homepage("https://github.com/bugsnotabunny/comppath")
    set_license("MIT")

    add_urls("https://github.com/bugsnotabunny/comppath.git")

    add_versions("experimental", "master")
    add_versions("v0.1.0", "v0.1.0")

    set_kind("library", {headeronly = true})

    on_install(function (package)
        import("package.tools.xmake").install(package, { tests = false })
    end)

    on_test(function (package)
        assert(package:has_cxxincludes("comppath/CompPath.hpp", {configs = {languages = "cxx20"}}))
    end)
