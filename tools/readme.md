# tools

xia、trans50、abc 用于转换旧版资源。

## xia

贴图转换工具，用法为：

```
xia.exe idx文件名 grp文件名 输出目录 是否拆分同编号的多张图片（0或1）
```

一般将exe和批处理文件放到DOS游戏目录，直接执行即可。

最后一个参数为 `1` 时，编号相同的多张图片会导出为 `编号_帧号.png`；为 `0` 时只保留每个编号的第一张图片。它不是闪烁开关。地图资源通常需要拆分，头像和战斗贴图通常不需要。

你也可以查看批处理的内容，自己进行一些设定。

战斗贴图还需要动作帧数，下面abc工程中会有介绍。

不会特别处理物品图像，建议自己重新选图。

## trans50

包含两个功能，覆盖了原 sfe2kdefscript、talkmaker 的资源转换用途。

需注意该工具的一些激进方案转换出的脚本有可能会不能正确执行。

### 将原版对话转为txt文件

```
trans50 --talk --in path
```
其中path为talk.idx和talk.grp所在的文件夹，编码限制为BIG5。执行之后会在path文件夹中生成talkutf8.txt文件，每个对话一行。

需注意如果用编辑器查看的话，行号为对话的编号加一。

该功能会去掉*，改为游戏中自动计算换行。

默认的编码为cp950，可以通过--talkcoding传入其他编码。

如果有其他需求请自己修改代码处理。


### 将原版指令直接转为 Cifa 脚本

```
trans50 --kdef --in path --talkfile talkfile --out path_out
```
其中 `path` 为 `kdef.idx`、`kdef.grp` 所在目录，`talkfile` 为预先生成的 `talkutf8.txt` 路径。

执行后会直接在 `path_out/script/event-cifa/` 生成 `<事件编号>.c`。不生成 Lua 文件，也不经过 Lua 文本转换。条件跳转使用 Cifa 原生的 `if (...) goto label;` 与 `label:`。

一次 `--kdef` 调用内部包含两个转换阶段，不需要用户分别执行：`transk.cpp` 先按原始 kdef 字偏移将主指令翻译为 Cifa 文本，并处理 50/32 对后续指令参数的改写；随后 `trans50.cpp` 将 50 号扩展指令展开为等价的 Cifa 语句，并进行条件、跳转和临时变量简化。

可以修改 `transk.ini` 中的指令模板，适配不同资源版本。

### 批处理范例

```bat
trans50 --talk --in %1
trans50 --kdef --in %1 --talkfile %1/talkutf8.txt --out path_out
```

## abc

用于转换旧的存档和列表等

因为转换基本是一次性的工作，对效率要求不高，但是需避免出错。故有些功能建议使用VS打开，在其中用Debug模式运行。

该工程需要用C++23或更新标准编译。

### 转换存档为32位，生成数据库，以及战斗帧数

需注意先转换好fight的图片，才能生成战斗帧数。

一般使用以下命令就可以生成。如果保存的路径不一样，也可以用--path参数传递进去。更详细的内容请查看源码。

```
.\abc.exe --save
```

### 递归转换index.ka为index.txt

因为index.ka是二进制文件，且每两个int16为一组，比较麻烦，因此提供了这个功能来转换成文本格式，方便查看和修改。

```
.\abc.exe --trans-indexka --path 资源目录
```

会递归查找目录下所有`index.ka`，以及zip中的`index.ka`，并生成同位置的`index.txt`。

`index.txt`每行格式为：

```
编号: 数字1, 数字2
```

其中编号从0开始，按`index.ka`每两个`int16`为一组的顺序递增。不存在的编号视为两个0。

编号可以不按顺序。

### 验证战斗帧数

```c++
.\abc.exe --check-fightframe
```
主要用于检查帧数与贴图数是否一致，正确情况下总帧数应是四个战斗帧数加起再乘以4，但计算结果一致并不保证结果完全正确。

错误的部分建议手动修正。

需在生成帧数文件后执行。

### 合并wmp到smp

```shell
.\abc.exe --combine-wmpsmp
```
用于将smp和wmp的图片资源和index.ka合并。

### 分拆武功光影效果图

eft图片在导出时为一个目录，分拆后可以更方便地替换和添加。但是这一步需要pascal复刻版的effect.bin文件。如果没有，请从z.dat文件中将这个列表手动复制出来。

```shell
.\abc.exe --split-eft
```

更建议重配eft图片。

## 后面的内容不容易自动完成，最好是在VS下面自己修改代码，用Debug来运行

### 转换二进制列表

将离队列表和升级经验转为文本文件。

用法为：

```c++
trans_bin_list(path + "list/levelup.bin", path + "list/levelup.txt");
trans_bin_list(path + "list/leave.bin", path + "list/leave.txt");
```
这两个文件通常来自pascal复刻版，某些lua版后来也有沿用。

实际上这两个就够了。

kys并没有处理武功武器配合。

### 编号人物头像

```c++
//重新产生头像
void make_heads(std::string path)
```

从一个头像库里面选出头像，命名为初始存档的编号。

头像库可以从其他地方获取，git上没有保存。

更建议自行重配头像。

### 检查脚本文件正确性

```c++
//检查3号指令的最后2个参数正确性
void check_script(std::string path)
```
只检查3号指令的最后两个参数是否合理。因修改器默认值问题，部分MOD对此参数处理有误，但DOS版的bug会使结果正确。复刻版修正了这个问题，故可能导致旧的部分剧情出错误。前述的 Cifa 脚本转换工具会处理，此处为验证。

### 转换战斗帧数格式

```c++
//导出战斗帧数为文本
void trans_fight_frame(std::string path0)
```
仅用于转换金庸水浒传及相关MOD的战斗帧数格式。一般不需要。

## Python 资源脚本

以下脚本需要 Python 3 和 Pillow（`pip install pillow`）。它们会直接改写指定资源或输出目录，执行前应备份资源。

### 大地图切片

`cut_huge_map.py` 取代原有的 cuthug C++/OpenCV 工程。默认读取当前目录的 `map.png`，放入 `17280×8640` 的透明画布后按 8×8 顺序切片，仅输出像素并非完全相同的块；默认输出文件名为 `0.png` 到 `63.png`。

```shell
python tools/cut_huge_map.py
python tools/cut_huge_map.py map.png --output work/game-dev/resource/battle-earth
```

可通过 `--canvas-width`、`--canvas-height` 和 `--grid` 调整画布与切片网格。参看“图片资源”。

### PNG 转 WebP

`convert_png_to_webp.py` 可递归处理资源目录或单个 ZIP，将 PNG 转为 WebP；目录中的原 PNG 会被删除，ZIP 会原地重新打包。默认使用无损 WebP，也可指定有损质量：

```shell
python tools/convert_png_to_webp.py work/game-dev/resource
python tools/convert_png_to_webp.py work/game-dev/resource --lossy --quality 90 -j 4
```

### Paper 墙体贴图

`rectify_wall_textures.py` 从 `resource/smap` 的已知墙体编号生成 Paper 墙面纹理候选，默认输出到 `resource/paper-wall-texture`，不会覆盖原始 `smap`：

```shell
python tools/rectify_wall_textures.py
python tools/rectify_wall_textures.py --ids 701 702 1410 --mode strip --width 32 --height 128
```

该脚本只生成待筛选的候选纹理，墙体效果仍需人工检查。