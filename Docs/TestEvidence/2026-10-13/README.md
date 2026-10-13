# Boss 射击测试修复的验证记录

这些 JSON 是从本地 Unreal Automation `index.json` 提取的结果字段，**不是完整原始报告**。保留测试名称、结果、全部事件消息和统计；省略设备标识，将项目及引擎绝对路径替换为占位符。每份摘要包含原文件 SHA-256，便于与本地原始文件核对。

| 文件 | 运行范围 | 结果 |
| --- | --- | --- |
| [before-shoot.json](before-shoot.json) | 修改前 Boss 射击 | 1 项失败，含两条失败断言 |
| [diagnostic-shoot.json](diagnostic-shoot.json) | 添加临时诊断信息 | 确认旧蒙太奇实例仍残留 |
| [after-shoot.json](after-shoot.json) | 修复后 Boss 射击 | 1 项通过，带插件资产警告 |
| [after-all.json](after-all.json) | 修复后全部 ShooterSam 测试 | 26 项通过，其中 2 项带警告 |

运行环境为 UE 5.7、Windows Editor Development、NullRHI。时间字段保留引擎报告的 UTC 时间；对应本地日期为 2026-10-13。结果仅描述这些测试的行为，不是完整试玩验收、性能测试或未来每个版本的保证。

代码与原因见 [修复记录](../../Boss射击自动化误报修复_2026-10-13.md) 和 [测试源码](../../../Source/ShooterSamProject/Tests/ShooterBossShootRangeTests.cpp)。

复跑：保存工程并关闭 UE 编辑器，在仓库根目录执行：

```powershell
.\Scripts\Test.ps1
```

默认先编译，随后运行所有 `ShooterSam.*`。原始日志与完整报告生成在 `Saved/Workflow/Tests/`，该目录不提交。不能用 `-ValidateOnly` 的环境检查替代测试运行。
