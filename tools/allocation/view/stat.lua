local core = require("core")
local interval = 5 * 60

local function stat(alloc_infos)
    local start = alloc_infos[1].time
    local stop = alloc_infos[#alloc_infos].time

    local st = {
        total = 0,
        interval = {},
        start = core.time2str(start),
        stop = core.time2str(stop),
    }

    local next_interval = start + interval
    local next_interval_key = start

    for _, info in ipairs(alloc_infos) do
		st.total = st.total + info.size

        if info.time > next_interval then
            next_interval_key = info.time
            next_interval = info.time + interval
            next_interval_total = 0
        end
        local interval_total = st.interval[next_interval_key] or 0
		interval_total = interval_total + info.size
        st.interval[next_interval_key] = interval_total
	end
    return st
end

local function stat_all()
    for file, alloc_infos in pairs(alloc_cache) do
        local st = stat(alloc_infos)
        print(file, string.format("%s-%s", st.start, st.stop), core.size2str(st.total))
        for k, v in pairs(st.interval) do
            print("\t", core.time2str(k), core.size2str(v))
        end
    end
end

stat_all()