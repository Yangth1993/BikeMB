# BikeMB UI/UX 文档

本目录保存 BikeMB 圆屏 UI 结构、交互规则、视觉规范、AI 状态表达和 LVGL 实现约束。

## 主要文档

- [current-ui-ux-map.md](current-ui-ux-map.md)：当前项目 UI/UX 总览图谱；以后查看 BikeMB 页面和流程先从这里开始。
- [interaction-model.md](interaction-model.md)：实体按键、触摸、手势、反馈和 UI 状态模型。
- [screen-flows.md](screen-flows.md)：启动流程、dashboard 页面顺序、设置流程、等待流程和错误流程。
- [ai-assistant-ui.md](ai-assistant-ui.md)：AI 助手页面布局、状态表、边界和 LVGL 组件指导。
- [visual-guidelines.md](visual-guidelines.md)：圆屏布局区域、视觉层级、颜色、字体、可读性和组件规则。

## 参考和交付链路文档

- [ui-redesign-avinox-ux.md](ui-redesign-avinox-ux.md)：当前 Avinox 风格圆屏 UI/UX 设计说明。
- [lvgl-pixel-replica-pipeline.md](lvgl-pixel-replica-pipeline.md)：设计稿到 LVGL 资源和模拟器验证的开发链路。

## 工作规则

做 UI 改动时，先更新最小相关的主要文档。只有当改动影响视觉方向、图片资源、字体资源或模拟器验证时，才更新参考和交付链路文档。
