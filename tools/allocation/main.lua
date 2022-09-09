local arg = {...}

require("tree")
local core = require("core")

for k, v in ipairs(arg) do
    core.cache(v)
end

while true do
    io.write("input file path:")
    local filename = io.read()
    core.load_script(filename)
end