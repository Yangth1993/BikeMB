# 串口日志查看

BikeMB 当前开发板默认串口为 `COM5`，波特率为 `115200`。

## 双击启动

打开：

`tools\open-serial-monitor.bat`

默认会监听：

`COM5 @ 115200`

## 指定端口

如果 Windows 以后把开发板识别成其他端口，例如 `COM7`，可以在命令行运行：

```powershell
tools\open-serial-monitor.bat COM7 115200
```

## 注意

- 同一时间只能有一个程序占用串口。
- 如果 PlatformIO monitor、Arduino IDE 串口监视器或其他软件已经打开了 `COM5`，请先关闭它。
- 当前脚本不会主动切换 DTR/RTS，避免 ESP32-S3 被带入下载模式。

