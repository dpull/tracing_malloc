local lib = {}

---@class alloc_info @alloc_info
 ---@field time number @alloc time
 ---@field size number @alloc size
 ---@field ptr string @alloc ptr
 ---@field stack table @stack, string array

function lib.load(file)
    local alloc_infos = {}
    local alloc_info
    
    local f = io.open(file, "r")
    while true do
        local line = f:read("*l")
        if not line then
            break
        end

        if line:match("^time:") then
            local ok, _, t, s, p = line:find('time:(%d+)\tsize:(%d+)\tptr:(0[xX][0-9a-fA-F%-]+)')
            assert(ok)

            alloc_info = {
                time = tonumber(t),
                size = tonumber(s),
                ptr = p,
                stack = {},
            }
        elseif line:match("^========") then
            table.insert(alloc_infos, alloc_info)
        else
            table.insert(alloc_info.stack, line)
        end
    end
    f:close()
    return alloc_infos
end

_G.alloc_cache = {}
function lib.cache(file)
    local alloc_infos = _G.alloc_cache[file]
    if not alloc_infos then
        alloc_infos = lib.load(file)
        _G.alloc_cache[file] = alloc_infos
    end
    return alloc_infos
end

function lib.time2str(timestamp)
    return os.date("%H:%M:%S", timestamp)
end

function lib.size2str(size)
    local i = 0
    local byteUnits = {' kB', ' MB', ' GB', ' TB'}
    repeat
        size = size / 1024
        i = i + 1
    until (size <= 1024)
    return string.format('%.1f', size) .. byteUnits[i]
end

function lib.load_script(filename)
    local env = setmetatable({}, {__index = _G})
    local fn, msg = loadfile(filename, "t", env)
    if not fn then
        print(string.format("load file: %s failed error:%s", filename, msg))
        return
    end

    local ok, err = xpcall(fn, debug.traceback)
    if not ok then
        print(string.format("exec file: %s error:%s", filename, err))
    end    
end

return lib
