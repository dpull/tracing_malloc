# allocation

基于 Electron 的内存分配 (`tracing_malloc`) 数据可视化工具，**单文件展示**版本。

数据结构与处理逻辑参考 `tools/disuse/allocation_py/`（`data.py` / `view.py` / `main.py`）。

## 数据格式

工具消费 `addr2line` 处理后的文本文件，每条分配记录格式如下：

```
time:1700000000\tsize:128\tptr:0x12345678
function\tfile\tmodule
function\tfile\tmodule
...
========
```

- 第一行：分配的时间戳（秒）、大小（字节）、指针
- 后续若干行：调用栈，每行包含三列（function / file / module），制表符分隔
- 用 `========` 分隔不同的分配记录

## 功能

- 打开 `.addr2line` 文件，解析全部分配记录
- 三种视图（工具栏切换）：
  - **Timeline**：按秒（`HH:MM:SS`）聚合，柱状图展示每秒分配总量；点击柱子查看该时段的 key frame 列表
  - **Overall**：去掉时间维度，按 key frame 在整个文件范围内聚合；柱状图展示 Top-20 关键帧，列表给出全部聚合结果
  - **Compare**：加载 A、B 两个文件，下方分三列展示：左侧文件 A 的内存分配列表、中间文件 B 的内存分配列表、右侧栈详情。每个列表有 `Function` / `Size` 两栏，点击 `Size` 表头可在 `▼ 降序 / ▲ 升序` 之间切换，A、B 列表的排序互不影响；点击任一行 → 详情面板弹出该 key frame 在该侧文件的所有分配（栈条目带 `[A]` 或 `[B]` 来源标记）
- key frame 选取规则：忽略 `libc.so` / `libstdc++.so` / `ld-linux-x86-64.so` / `libtracing_malloc.so` 后的第一个非忽略帧
- 点击 key frame 行（任一视图均可），查看具体每次分配的栈信息，按大小排序，可使用 `<<` `>>` 翻页
- 状态栏显示当前视图、汇总信息及 ignore 设置
- 工具栏 `Ignore top frames` 可自定义忽略栈顶若干层后再选 key frame（适合屏蔽自己的 wrapper allocator），切换会即时重算所有视图的聚合结果（Compare 模式下 A、B 同时重算）
- 工具栏 `Settings` 弹出设置窗口：`Module` 列默认隐藏，可在此勾选显示；设置通过 `localStorage` 持久化

## Compare 用法

1. 切换到 `Compare` 视图
2. 点 `Open File A` 加载 A
3. 点 `Open File B` 加载 B（任意顺序，可随时重新加载替换其中一份）
4. 下方三列：
   - 左：File A 的内存分配（`Function` / `Size`）
   - 中：File B 的内存分配（`Function` / `Size`）
   - 右：选中行的栈详情，可 `<<` `>>` 翻页
5. 每个列表的 `Size` 表头独立可点：`▼` 降序 / `▲` 升序，A、B 互不影响
6. 点击 A 或 B 中的某一行 → 右侧详情显示该 key frame 在所属文件中的全部分配（按大小排序），详情头会带 `[A]` / `[B]` 来源标记；同时对侧列表如果存在同名（同 key frame）条目，会自动高亮并滚动到该行，方便横向对照
7. `Module` 列在 Compare 模式下强制隐藏（与 `Settings` 无关）
8. 改 `Ignore top frames`：A、B 同步用新 ignore 重算，再刷新两个列表

## 用法

```sh
cd tools/allocation_electron
npm install
npm start
```

启动后可通过菜单 `File -> Open` 或工具栏 `Open File` 按钮加载 `.addr2line` 文件。

## 目录结构

```
allocation_electron/
├── package.json
├── main.js              # 主进程：窗口、菜单、IPC
├── preload.js           # 预加载脚本：contextBridge API
├── src/
│   ├── dataSource.js    # 数据解析与聚合（对应 data.py）
│   ├── index.html       # 主界面
│   ├── renderer.js      # 渲染进程逻辑（对应 view.py）
│   └── styles.css       # 样式
└── README.md
```
