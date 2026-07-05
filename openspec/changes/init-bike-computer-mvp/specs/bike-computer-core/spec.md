# bike-computer-core 变更规格

## ADDED Requirements

### Requirement: 实时速度显示

系统应基于轮速传感器显示当前骑行速度。

#### Scenario: 收到轮速脉冲后更新速度

- Given 已配置轮周长
- And 轮速传感器已产生至少两个有效脉冲
- When 系统计算最近脉冲间隔
- Then 主界面应显示当前速度

#### Scenario: 长时间无脉冲后速度归零

- Given 当前速度大于 0
- When 系统超过配置时间未收到新脉冲
- Then 当前速度应显示为 0

### Requirement: 圆屏主骑行界面

系统应提供适合圆形屏幕的主骑行界面。

#### Scenario: 骑行时查看核心数据

- Given 系统处于骑行状态
- When 用户查看主界面
- Then 用户应能看到当前速度、单位、单次里程、骑行时间和电量

### Requirement: 总里程持久化

系统应保存累计总里程。

#### Scenario: 断电后恢复总里程

- Given 系统已有累计总里程
- When 设备断电并重新启动
- Then 系统应恢复之前保存的总里程

