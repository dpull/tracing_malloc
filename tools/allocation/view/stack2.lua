local core = require("core")

function string.starts(String, Start)
    return string.sub(String, 1, string.len(Start))==Start
end

local function stat(alloc_infos)
    local root = {}
    for _, info in ipairs(alloc_infos) do
        local stack = root
        for _, frame in ipairs(info.stack) do
            stack[frame] = stack[frame] or {size = 0, count = 0}

            local cur = stack[frame]
            cur.size = cur.size + info.size
            cur.count = cur.count + 1
            stack = cur
        end
	end
    return root
end

local function deepCopy(original, level, maxLevel)
    if level > maxLevel then
        return nil
    end

    local copy = {}

    for key, value in pairs(original) do
        if type(value) == "table" then
            copy[key] = deepCopy(value, level + 1, maxLevel)
        else
            copy[key] = value
        end
    end

    return copy
end

local function filterFrame(frame)
    local sortableList = {}
    for key, value in pairs(frame) do
        if type(value) == "table" then
            table.insert(sortableList, { key = key, size = value.size})
        end
    end
    table.sort(sortableList, function(a, b)
        return a.size > b.size
    end)
    local keys = {count=true, size=true}
    for i = 1, math.min(6, #sortableList) do
        keys[sortableList[i].key] = true
    end
    for key, value in pairs(frame) do
        if not keys[key] then
            frame[key] = nil
        end
    end
end

local function filterStack(st, level, filterLevel)
    if level > filterLevel then
        return
    end

    filterFrame(st)
    for k, v in pairs(st) do
        if type(v) == "table" then
            if level <= filterLevel then
                filterFrame(v)
                filterStack(v, level+1, filterLevel)
            end
        end
    end
end

_G.view_stack2_cache = _G.view_stack2_cache or {}
-- _G.view_stack2_cache = {}
local function stat_all()
    for file, alloc_infos in pairs(alloc_cache) do
        local st = _G.view_stack2_cache[file] or stat(alloc_infos)
        _G.view_stack2_cache[file] = st
    end
    for k, v in pairs(_G.view_stack2_cache) do
        local fd = io.open(k .. ".tree", "w+")
        local tb = deepCopy(v, 1, 15)
        filterStack(tb, 1, 15)
        log_tree_callback(k, tb, function (str)
            fd:write(str)
            fd:write("\n")
        end)
        fd:close()
    end
end

stat_all()