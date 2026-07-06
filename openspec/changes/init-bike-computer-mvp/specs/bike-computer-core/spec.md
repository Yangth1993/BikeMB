# bike-computer-core 变更规格

## ADDED Requirements

### Requirement: 实时速度显示

系统应基于轮速传感器显示当前骑行速度。

#### Scenario: 收到轮速脉冲后更新速度

- Given 已配置轮周长
- And 轮速传感器产生有效脉冲
- When 系统计算轮转间隔
- Then 主界面应更新当前速度

### Requirement: 圆屏主骑行界面

系统应提供适合圆形屏幕的主骑行界面。

#### Scenario: 查看核心信息

- Given 系统处于骑行或演示状态
- When 用户查看主界面
- Then 用户应看到速度、单次里程、骑行时间和电量信息

### Requirement: 总里程持久化

系统应保存累计总里程。

#### Scenario: 断电后恢复总里程

- Given 系统已保存累计总里程
- When 设备断电后重新启动
- Then 系统应恢复之前保存的总里程
