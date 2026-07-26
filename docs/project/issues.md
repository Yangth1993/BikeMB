# 项目问题

本文记录当前项目层面的开放问题和风险。

## Open

- 中文文档在部分终端环境中显示为乱码，需要统一确认编码和查看方式。
- 根目录仍有 `tools/` 等开发辅助目录；模拟器源已放入 `tools/simulator/`。
- `docs/assets/` 当前包含文档截图和 UI 参考图，需要确认是否继续作为文档资产保留。
- ADR-0004 ESP-IDF 双核门禁尚未关闭：2026-07-22/23 第一轮已验证启动、双核 task 归属、LVGL 单 owner、资源日志、BOOT 按住上电不误触发、ESP-IDF Wi-Fi STA 上线、4MB app 分区、CST816 初始化、用户确认触摸正常和 90 秒以上显示长稳；AI/BOOT 键 poll 已确认运行并完成释放解锁，用户确认按键正常；ESP-IDF AudioSession codec/I2S 初始化已到 `session ready`；ESP-IDF Qwen ASR/Qwen Chat/CosyVoice TTS transport 已构建通过，但串口重新枚举异常导致启动日志、发声板测、取消回归和完整语音闭环仍未完成。
- ESP-IDF 默认 Wi-Fi/Netif info 日志会输出本地网络标识和地址；直接在 `WifiService` 内压低 SDK 日志曾触发启动前 PSRAM panic，已撤回。后续应优先通过 `sdkconfig.defaults` 或全局日志策略验证后再处理。

## Closed

- 暂无。
