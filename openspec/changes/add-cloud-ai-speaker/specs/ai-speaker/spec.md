# ai-speaker 变更规格

## ADDED Requirements

### Requirement: 复用 BOOT 实体键按住说话

系统应仅在启动保护完成后、用户按住复用的 `Key1/BOOT (GPIO0)` 时录音，并在松开按键后提交本次语音请求。按键低电平有效，使用板载 `10 kΩ` 上拉。

#### Scenario: 启动保护期忽略按键

- Given 设备上电未满 3000 ms
- When GPIO0 电平发生变化或 BOOT 键被按住
- Then 系统不应产生 AI `PRESS` 或 `RELEASE` 事件
- And 系统不应开始录音

#### Scenario: 释放后才解锁

- Given 设备已经上电满 3000 ms
- And BOOT 键从启动阶段持续处于按下状态
- When 系统尚未观察到连续 50 ms 的释放状态
- Then AI 按键应保持未解锁
- And 系统不应补发 `PRESS`

#### Scenario: 运行态按键消抖

- Given AI 按键已在连续释放 50 ms 后解锁
- When GPIO0 连续保持低电平 30 ms
- Then 系统应产生一次 AI `PRESS`
- And 后续抖动不应重复产生 `PRESS`

#### Scenario: 按下后开始录音

- Given AI 助手已启用、配置有效且 Wi-Fi 已连接
- And AI 助手处于 `Idle`
- When 用户按下已解锁的 BOOT/AI 按键
- Then 系统应进入 `Recording`
- And 系统应开始采集有时长上限的麦克风音频

#### Scenario: 松开后停止录音

- Given 系统处于 `Recording`
- When 用户松开 BOOT/AI 按键
- Then 系统应停止采集音频
- And 系统应进入 `Recognizing`

#### Scenario: 未按键时不录音

- Given 用户没有按住 BOOT/AI 按键
- When 设备处于普通码表显示或 AI 空闲状态
- Then 系统不应采集 AI 语音录音

#### Scenario: 录音达到上限

- Given 用户持续按住 BOOT/AI 按键
- When 录音达到 10 秒上限
- Then 系统应停止录音
- And 系统应提示本次录音超时
- And 系统不应静默上传被截断的录音

#### Scenario: 录音过短或为空

- Given 系统处于 `Recording`
- When 用户在 300 ms 内松开按键或系统没有采集到有效样本
- Then 系统应取消本次录音并回到 `Idle`
- And 系统不应调用 STT

### Requirement: 分阶段云端语音问答

系统应按 STT、DeepSeek 和 TTS 三个阶段处理一次短语音问题，并通过板载喇叭播放回答。

#### Scenario: 完成一次语音问答

- Given 设备已连接 Wi-Fi
- And STT、DeepSeek 和 TTS 配置有效
- When 用户松开 AI 按键并产生有效录音
- Then 系统应通过 STT 把录音转为文字
- And 系统应把文字问题发送给 DeepSeek
- And 系统应通过 TTS 把 DeepSeek 的短回答转换为音频
- And 系统应通过板载喇叭播放回答

#### Scenario: 各阶段状态可见

- Given 系统正在处理一次 AI 请求
- When 请求依次经过 STT、DeepSeek、TTS 和播放
- Then UI 应依次显示 `正在识别`、`正在思考`、`正在准备语音` 和 `正在回答`

### Requirement: STT、LLM 和 TTS 边界可替换

系统应通过独立 provider 接口隔离 STT、LLM 和 TTS，供应商字段不得进入 AI 状态机或 UI。

#### Scenario: 使用 V1 默认服务

- Given V1 使用默认 provider 配置
- When 系统执行语音问答
- Then STT 应调用阿里云百炼 `qwen3-asr-flash` Base64 WAV REST 接口
- And LLM 应调用 DeepSeek
- And TTS 应调用 `cosyvoice-v3-flash` HTTP/SSE 接口并请求 16 kHz mono PCM

#### Scenario: 替换语音供应商

- Given 后续需要替换 STT 或 TTS 服务
- When 开发工程师提供相同 provider 接口的新 adapter
- Then AI 状态机和 AI UI 不应因供应商 JSON、鉴权头或 SDK 类型而修改

### Requirement: 单一音频会话所有权

系统应由 `audio_session` 唯一拥有 I2S0、ES7210 和 ES8311，并以半双工方式仲裁录音和播放。

#### Scenario: 录音时停止音乐

- Given 系统处于 `MusicPlaying`
- When 用户按下 AI 按键
- Then 系统应停止音乐并释放播放资源
- And 系统应进入 `Recording`

#### Scenario: AI 会话期间普通提示不抢占

- Given 系统处于 `Recording`、`Recognizing`、`Thinking`、`Synthesizing` 或 `Speaking`
- When 普通档位提示请求到达
- Then 普通提示应被丢弃或延后
- And 当前 AI 会话不应被普通提示打断

### Requirement: 云音频流播放

系统应在 V1 支持预设或私有配置中的 HTTPS MP3 直链播放，并为未来点歌保留曲目解析边界。

#### Scenario: 播放配置的音频流

- Given AI 助手已启用且 Wi-Fi 已连接
- And 配置包含有效 HTTPS MP3 URL
- When 用户在 AI 页面选择播放
- Then 系统应显示 `正在连接音乐`
- And 连接和解码成功后进入 `MusicPlaying`

#### Scenario: 停止音乐播放

- Given 系统处于 `MusicPlaying`
- When 用户触发停止
- Then 系统应停止音频输出并释放音频会话
- And 页面切换和核心骑行数据显示应继续可用

#### Scenario: V1 不执行语音点歌

- Given V1 收到包含歌曲名的语音问题
- When AI 完成问答
- Then 系统不应把该问题转换为音乐服务命令
- And 系统不应绕过配置 URL 播放任意来源

### Requirement: 主界面提示和独立 AI 页面

系统应在码表主界面显示不遮挡核心信息的轻量 AI 状态，并提供独立 AI 页面显示详细状态和控制。

#### Scenario: 主界面显示等待状态

- Given 用户当前位于码表主界面
- When AI 处于录音、识别、思考、合成、回答或音乐状态
- Then UI 应显示对应轻量提示
- And 提示不应遮挡速度、电量、里程或骑行时间

#### Scenario: AI 页面显示详情

- Given 用户进入独立 AI 页面
- When AI 助手已启用
- Then 页面应显示当前阶段、Wi-Fi 状态和脱敏错误摘要
- And 页面应在适用状态提供取消或停止入口

### Requirement: 取消、超时和旧请求隔离

系统应允许取消当前交互，并阻止已取消请求的晚到结果修改新状态。

#### Scenario: 取消当前交互

- Given 系统处于 `Recording`、`Recognizing`、`Thinking`、`Synthesizing`、`Speaking`、`ConnectingMusic` 或 `MusicPlaying`
- When 用户触发取消或停止
- Then 系统应停止相关网络和音频工作
- And 系统应回到 `Idle`

#### Scenario: 丢弃晚到结果

- Given 请求 A 已取消并已开始请求 B
- When 请求 A 的云端结果晚到
- Then 系统应根据 request ID 丢弃请求 A 的结果
- And 请求 B 的状态和音频不得被修改

#### Scenario: 忙态按键替换请求

- Given 系统处于 `Recognizing`、`Thinking`、`Synthesizing`、`Speaking`、`ConnectingMusic` 或 `MusicPlaying`
- And Wi-Fi 已连接
- When 用户再次按下已解锁的 BOOT/AI 按键
- Then 系统应使旧 request ID 失效
- And 系统应停止旧音频并进入新的 `Recording`

#### Scenario: 阻塞云调用不阻塞取消

- Given cloud worker 正在执行一个尚未返回的 provider 请求
- When 用户触发取消
- Then AI control task 应立即使当前 request ID 失效并更新 UI 状态
- And 核心码表 UI 应继续刷新
- And 云调用的晚到结果不应恢复旧状态

#### Scenario: 总处理超时

- Given 用户已经松开 AI 按键
- When STT、DeepSeek 和 TTS 在 15 秒内未产生可播放音频
- Then 系统应进入 `Error`
- And UI 应显示 `AI 暂时不可用`
- And 用户应能够重试

### Requirement: AI 失败不影响核心码表

系统应在 Wi-Fi、STT、DeepSeek、TTS、TLS、解码或音乐流失败时保持核心码表功能可用。

#### Scenario: Wi-Fi 不可用

- Given AI 助手已启用但 Wi-Fi 未连接
- When 用户按下 AI 按键
- Then 系统不应开始录音
- And 系统应提示 AI 暂不可用
- And 速度、电量、里程和页面切换应继续运行

#### Scenario: 云端或播放失败

- Given 系统正在处理 AI 请求或音乐流
- When 任一云服务、TLS、音频解码或播放失败
- Then 系统应停止当前失败流程并进入 `Error` 或 `Idle`
- And 核心码表 UI 应继续刷新

### Requirement: 延迟与资源受预算约束

系统应在明确的延迟和内存预算内实现 AI V1，并记录目标板实测结果。

#### Scenario: 短问题延迟目标

- Given 用户在正常 Wi-Fi 网络下提交短语音问题
- When 系统完成 STT、DeepSeek 和 TTS
- Then 目标应在松开按键后 8 秒内开始语音回复
- And 超过 15 秒应明确失败

#### Scenario: 音乐连接目标

- Given 用户请求播放有效的配置音频流
- When 系统连接云音频流
- Then 系统应在 10 秒内开始播放或明确失败

#### Scenario: 资源预算验证

- Given 开发工程师完成一个 AI 固件里程碑
- When 在目标 ESP32-S3R8 板上测试该里程碑已经实现的阶段
- Then AI 增量 PSRAM 峰值应不超过 1.5 MiB
- And AI 增量 internal DRAM 峰值应不超过 256 KiB
- And 测试记录应包含 free heap、largest block、free PSRAM、task stack high-water mark 和 audio underrun count

#### Scenario: 资源增量使用一致基线

- Given 需要计算 AI 增量资源
- When 记录基线数据
- Then 应使用相同 dashboard build、关闭 AI 功能并在启动稳定 30 秒后采样
- And AI 默认启用前应完成启动、录音、TLS、TTS 和音乐峰值全量测试

### Requirement: 敏感配置和隐私数据不进入仓库

系统应从 Git 忽略的本地编译配置读取 Wi-Fi 密码、API token 和用户音频 URL，并避免持久化语音交互内容。

#### Scenario: 使用占位配置

- Given 文档或 tracked 示例需要描述 AI 配置
- When 配置涉及 token、密码、个人信息或用户 URL
- Then 示例应使用明显占位符
- And 仓库不应包含真实敏感值

#### Scenario: TLS 和日志安全

- Given 固件连接云端服务
- When 建立 HTTPS 请求或输出诊断日志
- Then 固件应校验证书
- And 正式实现不应使用 `setInsecure()`
- And 日志不应输出 token、密码、完整敏感 URL、录音、转写全文或回答全文

#### Scenario: 不持久化交互内容

- Given 一次语音问答已经结束或取消
- When 系统释放该请求资源
- Then 录音、转写文本和回答不应写入 Flash 或 Git 跟踪文件

### Requirement: 录音键打开独立 AI 页面

系统应在实体录音/AI 键触发录音请求前，先导航到独立 AI 助手页面。

#### Scenario: 录音键打开独立 AI 页面

- Given 用户位于任意当前 UI 页面
- When 用户按下已解锁的 BOOT/AI 录音键
- Then dashboard 应立即导航到独立 AI 页面
- And AI 助手应为同一次按键启动录音请求
