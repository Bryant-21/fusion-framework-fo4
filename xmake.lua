local fusionframework_root = os.scriptdir()
local fusionframework_minhook = path.absolute("vendor/minhook", fusionframework_root)
local fusionframework_standalone = os.projectdir() == fusionframework_root

if fusionframework_standalone then
    includes(os.getenv("COMMONLIBF4_DIR") or "lib/commonlibf4")
    set_project("fusion-framework-fo4")
    set_languages("c++23")
    set_warnings("allextra")
    add_rules("mode.debug", "mode.releasedbg")
    set_defaultmode("releasedbg")
end

function fusionframework_minhook_dir()
    return fusionframework_minhook
end

target("fusion-framework-minhook")
    set_kind("static")
    set_default(false)
    add_files(path.join(fusionframework_minhook, "src/buffer.c"), path.join(fusionframework_minhook, "src/hook.c"),
        path.join(fusionframework_minhook, "src/trampoline.c"), path.join(fusionframework_minhook, "src/hde/hde64.c"))
    add_includedirs(path.join(fusionframework_minhook, "include"), { public = true })
target_end()

add_requires("toml11 v4.4.0")
add_requires("nlohmann_json")

rule("fusion.exports")
    on_config(function (target)
        target:add("shflags", "/DEF:" .. path.join(target:autogendir(), "rules", "fusion", "exports.def"), { force = true })
    end)
    before_link(function (target)
        import("lib.detect.find_tool")
        import("core.tool.toolchain")
        import("core.project.depend")
        local conf = target:extraconf("rules", "fusion.exports") or {}
        local inputs = table.join(target:objectfiles())
        for _, name in ipairs(table.wrap(conf.packages)) do
            local pkg = assert(target:pkg(name), "fusion.exports: package " .. name .. " is not added to " .. target:name())
            for _, dir in ipairs(table.wrap(pkg:get("linkdirs"))) do
                for _, link in ipairs(table.wrap(pkg:get("links"))) do
                    local lib = path.join(dir, link .. ".lib")
                    if os.isfile(lib) then table.insert(inputs, lib) end
                end
            end
        end
        local deffile = path.join(target:autogendir(), "rules", "fusion", "exports.def")
        depend.on_changed(function ()
            local msvc = toolchain.load("msvc", { plat = target:plat(), arch = target:arch() })
            local dumpbin = assert(find_tool("dumpbin", { envs = msvc:runenvs() }), "fusion.exports: dumpbin not found")
            local function qualified(undecorated)
                local out, depth = {}, 0
                for c in undecorated:gmatch(".") do
                    if c == "<" or c == "(" then depth = depth + 1
                    elseif (c == ">" or c == ")") and depth > 0 then depth = depth - 1
                    elseif depth == 0 then table.insert(out, c) end
                end
                local words = table.concat(out):trim():split("%s+")
                return words[#words]
            end
            local exports = {}
            for _, input in ipairs(inputs) do
                local listing = input:find("%.rc%.obj$") and "" or os.iorunv(dumpbin.program, { "/symbols", "/nologo", input })
                for line in listing:gmatch("[^\r\n]+") do
                    local call, symbol, rest = line:match("SECT%x+%s+notype%s*(%(?%)?)%s+External%s+| (%S+)%s*(.*)$")
                    if symbol then
                        local name = rest:startswith("(") and qualified(rest:sub(2, -2)) or symbol
                        for _, prefix in ipairs(conf.prefixes) do
                            if name and name:startswith(prefix) and not name:find("`", 1, true) then
                                exports[symbol] = call == ""
                                break
                            end
                        end
                    end
                end
            end
            local lines = { "EXPORTS" }
            for _, symbol in ipairs(table.orderkeys(exports)) do
                table.insert(lines, "    " .. symbol .. (exports[symbol] and " DATA" or ""))
            end
            assert(#lines < 65536, "fusion.exports: more than 65535 exports in " .. target:name())
            io.writefile(deffile, table.concat(lines, "\n") .. "\n")
        end, { dependfile = deffile .. ".d", files = inputs, changed = target:is_rebuilt() })
    end)
rule_end()

rule("fusion.version_header")
    on_load(function (target)
        local file, prefix = table.unpack(target:values("fusion.version_header"))
        local text = io.readfile(file)
        local function part(name)
            return assert(text:match("#define%s+" .. prefix .. "_" .. name .. "%s+(%d+)"), file .. ": no " .. prefix .. "_" .. name)
        end
        target:set("version", part("MAJOR") .. "." .. part("MINOR") .. "." .. part("PATCH"))
    end)
rule_end()

target("FusionFramework")
    add_rules("fusion.version_header")
    set_values("fusion.version_header", path.join(fusionframework_root, "include/fusionframework/Version.h"), "FUSIONFRAMEWORK_VERSION")
    add_rules("commonlibf4.plugin", {
        name = "FusionFramework",
        author = "B21",
        description = "Shared runtime core for B21 F4SE plugins"
    })
    add_rules("fusion.exports", { prefixes = { "fusionframework::" } })
    set_default(fusionframework_standalone)
    add_includedirs(path.join(fusionframework_root, "include"))
    add_files(path.join(fusionframework_root, "src/*.cpp"))
    add_packages("toml11", "nlohmann_json")
    add_shflags("/PDBALTPATH:FusionFramework.pdb", { force = true })
    add_shflags("/PDBSTRIPPED:$(builddir)/FusionFramework.stripped.pdb", { force = true })
target_end()

function fusionframework_use()
    add_includedirs(path.join(fusionframework_root, "include"))
    add_files(path.join(fusionframework_root, "src/consumer/*.cpp"))
    add_packages("toml11")
    add_deps("fusion-framework-minhook", "FusionFramework")
    add_shflags("/DELAYLOAD:FusionFramework.dll", { force = true })
    add_syslinks("delayimp")
end

function fusionframework_use_config()
    add_includedirs(path.join(fusionframework_root, "include"))
    add_files(path.join(fusionframework_root, "src/Config.cpp"), path.join(fusionframework_root, "src/ConfigStore.cpp"))
    add_packages("toml11")
end

function fusionframework_use_pure()
    add_includedirs(path.join(fusionframework_root, "include"))
    add_files(path.join(fusionframework_root, "src/Config.cpp"))
    add_packages("toml11")
end

