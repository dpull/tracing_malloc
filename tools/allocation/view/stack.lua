local core = require("core")
local debug1 = true

local function stat(alloc_infos)
    local root = {}

    for _, info in ipairs(alloc_infos) do
        local stack = root
        for i = #info.stack, 1, -1 do
            local frame = info.stack[i]
            stack[frame] = stack[frame] or {size = 0, frame = frame}
            stack[frame].size = stack[frame].size + info.size
            stack = stack[frame]

            if debug1 then
                debug1 = false
                log_tree("aaaa", root)
            end
        end
	end
    return root
end

local function stat_all()
    for file, alloc_infos in pairs(alloc_cache) do
        local st = stat(alloc_infos)
        log_tree(string.format("file:%s", file), st)
    end
end

stat_all()