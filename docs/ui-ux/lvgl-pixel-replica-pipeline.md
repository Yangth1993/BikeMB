# BikeMB LVGL 像素级复刻开发链路

## 目标与边界

目标是在 `360 x 360` 圆屏上尽量贴近确认稿，同时保留实时数据、触摸翻页、档位切换、AI 助手状态和语音播报。设计稿不能直接变成可维护的 LVGL 页面，因此界面拆成两类资源：

- 静态视觉层：表盘底纹、金属边缘、发光弧和复杂纹理，使用透明 PNG 转为 LVGL C 数组。
- 动态交互层：速度、电量、网络状态、AI 对话状态、档位、时间和可点击区域，使用 LVGL 控件与自定义字体。

参考稿保存在 [avinox-ui-reference.png](../assets/avinox-ui-reference.png)。

## 工具链

| 阶段 | 工具 | 项目中的用途 |
| --- | --- | --- |
| 设计定稿 | Figma、Lunacy 或 Stitch | 输出 360 x 360 页面、透明装饰图层和尺寸标注 |
| 字体准备 | `@fontsource/rajdhani` | 提供可再分发的 Rajdhani 字体源 |
| 字体转换 | LVGL 官方 `lv_font_conv` | 只打包需要的字形，生成 LVGL C 字体 |
| 图片转换 | LVGL 官方 `lv_img_conv` | 把透明 PNG 转成 `lv_img_dsc_t` C 资源 |
| 桌面验证 | `tools/simulator/bikemb_ui` | 在 360 x 360 窗口检查布局、字体和交互 |
| 固件验证 | PlatformIO | 构建并烧录 `src/firmware/bikemb` |

本项目固定使用 Node `20.19.5`、TypeScript `4.9.5` 和 `ts-node 10.9.2` 运行旧版图片转换器。这样可避开 Node 24 与 `canvas` 原生模块的兼容问题，不改变固件编译环境。

## 设计稿输出规范

设计工具中的每个页面使用 `360 x 360 px` Frame，圆屏有效区域直径为 360 px。导出时遵循以下规则：

1. 动态文字不要烘焙进背景图。
2. 发光、纹理和金属边缘按独立透明 PNG 导出。
3. 图标优先导出单色透明 PNG，以便 LVGL 按档位重映射颜色。
4. PNG 文件使用 ASCII 小写文件名，避免官方 Windows 转换器的多字节路径问题。
5. 图片保持 1:1 尺寸，不在 LVGL 中二次缩放，以减少失真和运行时开销。
6. AI 助手页的电量、网络状态和对话状态必须保留为动态 LVGL 控件，不烘焙进背景图。

Figma、Lunacy 和 Stitch 都可以作为设计入口，但它们不能直接生成 BikeMB 当前架构可用的 LVGL 页面。稳定交付物是尺寸标注、字体信息以及 PNG 资源；动态控件仍在共享的 LVGL UI 层实现。

## 当前资源

| 源资源 | 生成资源 | 使用位置 |
| --- | --- | --- |
| `src/assets/source/ui/home_assist_glow.png` | `dashboard_img_home_assist_glow.c` | 首页圆形边缘与分段助力发光层 |
| Rajdhani Latin 700 WOFF | `dashboard_font_speed_88.c` | 首页速度整数和小数 |

首页发光图层转换为 `LV_IMG_CF_ALPHA_8BIT`，只存透明度、不重复存 RGB 通道，再通过 `lv_obj_set_style_img_recolor()` 跟随档位变色：ECO 绿色、TRAIL 黄色、AUTO 蓝色、BOOST 红色。速度和状态信息保持为实时 LVGL Label，因此后续接入真实数据不需要重做图片。

AI 助手页如果需要声波、圆环或状态图形，优先用 LVGL 原生 arc、line、bar 或少量单色透明图标实现。网络状态、电量和对话状态必须来自运行时状态，不能作为静态图片文字处理。

## 生成命令

首次拉取或工具版本变化后执行：

```powershell
cmd /c npm install
npm run assets:lvgl
```

单独生成字体或图片：

```powershell
npm run assets:fonts
npm run assets:images
```

生成文件会进入 `src/firmware/bikemb/src/app/assets/`，并由 `dashboard_assets.h`、`lv_conf.h` 和 `src/firmware/bikemb/src/CMakeLists.txt` 接入固件。模拟器同步脚本会复制同一份 UI 和资产，避免维护两套页面实现。

## 验证闭环

每次视觉改动按以下顺序验证：

1. 执行 `python tools/tests/test_visual_asset_pipeline_contract.py`，检查工具版本、资源声明、尺寸和页面接入。
2. 执行 `powershell -ExecutionPolicy Bypass -File tools/run-tests.ps1`，确认触摸、翻页、档位和音频契约未退化。
3. 同步并构建模拟器，在相同档位和相同数据下截取 360 x 360 页面。
4. 将模拟器截图与参考稿并排比较，重点检查坐标、字号、行高、边缘裁切、颜色和发光范围。
5. 通过后执行 PlatformIO 固件构建；只有用户确认模拟器效果后才烧录硬件。

## 官方参考

- [LVGL 8.4 Images](https://docs.lvgl.io/8.4/overview/image.html)
- [LVGL 8.4 Fonts](https://docs.lvgl.io/8.4/overview/font.html)
- [lvgl/lv_font_conv](https://github.com/lvgl/lv_font_conv)
- [lvgl/lv_img_conv](https://github.com/lvgl/lv_img_conv)
