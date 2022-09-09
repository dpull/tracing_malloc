local core = require("core")

local function stat(alloc_infos)
    local root = {}
    local start = alloc_infos[1].time + 30 * 60
    for _, info in ipairs(alloc_infos) do
        if info.time > start then
            local stack = root
            for _, frame in ipairs(info.stack) do
                stack[frame] = stack[frame] or {size = 0, count = 0}

                local cur = stack[frame]
                cur.size = cur.size + info.size
                cur.count = cur.count + 1
                stack = cur
            end
        end
	end
    return root
end

local function stat_debug()
    local fd = io.open("stack_debug.txt", "w+")
    local write = function(...)
        fd:write(...)
        fd:write("\n")
    end

    for file, st in pairs(view_stack_cache) do
        log_tree(string.format("file:%s", file), st, write)
    end

    fd:close()
end

local function for_node(frame, st, dep, print_dep, print_fn)
    if dep > print_dep then
        return 
    end

    if dep == print_dep then
        print_fn(frame, st)
        return 
    end

    for k, v in pairs(st) do
        if type(v) == "table" then
            for_node(k, v, dep + 1, print_dep, print_fn)
        end        
    end
end

local function stat_print(filename, print_dep)
    local fd = io.open(filename, "w+")
    fd:write("size", "\t", "count", "\t", "function\tline\tfile", "\n")

    for file, st in pairs(view_stack_cache) do
        for_node("", st, 0, print_dep, function(frame, cur)
            fd:write(cur.size, "\t", cur.count, "\t", frame, "\n")
        end)
    end

    fd:close()    
end

_G.view_stack_cache = _G.view_stack_cache or {}
-- _G.view_stack_cache = {}
local function stat_all()
    for file, alloc_infos in pairs(alloc_cache) do
        local st = view_stack_cache[file] or stat(alloc_infos)
        view_stack_cache[file] = st
    end

    stat_print("stack2.tab", 2)
    stat_print("stack3.tab", 3)
    stat_print("stack4.tab", 4)
end

stat_all()