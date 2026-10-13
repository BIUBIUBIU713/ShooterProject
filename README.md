# ShooterSam

基于 Unreal Engine 5.7、C++ 与蓝图的个人射击玩法原型，围绕波次战斗、资源与区域推进、Boss 遭遇及撤离结算展开。

项目用于游戏客户端开发学习与实习作品展示。玩法由本人构思，**大部分功能在 AI 辅助下完成**；角色移动参考教程与引擎模板。当前展示的是工程与验证记录，实机视频尚待补充。

## 玩法与代码入口

| 内容 | 实现入口 |
| --- | --- |
| 波次生成、存活敌人数量管理 | [WaveManager.cpp](Source/ShooterSamProject/WaveManager.cpp) |
| 武器射击与能量管理 | [Gun.cpp](Source/ShooterSamProject/Gun.cpp) |
| Boss 普通攻击、特殊阶段与战斗收尾 | [ShooterBossEnemy.cpp](Source/ShooterSamProject/ShooterBossEnemy.cpp) |
| 交互接口、区域门与供电设施 | [ShooterInteractable.h](Source/ShooterSamProject/ShooterInteractable.h)、[RegionDoor.cpp](Source/ShooterSamProject/RegionDoor.cpp)、[PowerGenerator.cpp](Source/ShooterSamProject/PowerGenerator.cpp) |
| 区域解锁、Boss 遭遇条件与撤离结算 | [ShooterSamProjectGameMode.cpp](Source/ShooterSamProject/ShooterSamProjectGameMode.cpp) |
| 自动化验证 | [Tests](Source/ShooterSamProject/Tests) |

## 在编辑器中运行

环境：Windows、Unreal Engine 5.7、可编译 UE 5.7 C++ 工程的 Visual Studio C++ 工具链及 Windows SDK、Git LFS。

```powershell
git lfs install
git clone https://github.com/BIUBIUBIU713/ShooterProject.git
cd ShooterProject
git lfs pull
.\Scripts\Build.ps1
```

打开 `ShooterSamProject.uproject`，加载 `/Game/MyStuff/Maps/Escape` 后点击运行。该地图也是当前配置中的默认启动地图。脚本无法自动定位引擎时，可指定实际安装目录，例如：

```powershell
.\Scripts\Build.ps1 -EngineRoot 'D:\UnReal\UE_5.7'
```

编译或测试前，保存工程并关闭 UE 编辑器。更多脚本与资产提交说明见 [Git 工作流](Docs/GitWorkflow.md)。尚未完成全新环境克隆及独立打包版的运行验收。

## 操作

以下按键来自当前玩家控制器加载的 Enhanced Input 映射配置。

| 输入 | 功能 |
| --- | --- |
| WASD／方向键 | 移动 |
| 鼠标移动 | 调整视角 |
| 空格 | 跳跃 |
| 鼠标左键 | 射击 |
| R | 武器充能 |
| E | 交互 |

## 自动化验证与修复记录

2026-10-13 的一次完整 `ShooterSam.*` 自动化运行：**26 项通过，其中 2 项带警告，0 项失败**。运行方式为 Windows Editor Development、NullRHI；该结果不代表画面、性能或完整试玩均已验收。

本轮修复了 Boss 射击测试中的两个问题：把指定行为树装饰器误认为开枪的必要条件，以及手动推进动画后遗漏结束事件派发。修改范围为测试代码；Boss 玩法代码与蓝图未在这次修复中变更。

- [原因、修复方式与验证记录](Docs/Boss射击自动化误报修复_2026-10-13.md)
- [修复前后测试结果摘要与原报告哈希](Docs/TestEvidence/2026-10-13/README.md)
- [测试源码](Source/ShooterSamProject/Tests/ShooterBossShootRangeTests.cpp)

复跑全部测试：

```powershell
.\Scripts\Test.ps1
```

脚本默认先编译；运行日志与完整报告保存在本地 `Saved/Workflow/Tests/`。仓库中的 JSON 是明确标注的提取摘要，并非完整原始报告。

## 开发说明与后续工作

本人提出玩法设想，并参与蓝图编辑、基础交互配置和实际运行检查；C++ 与蓝图功能大量借助 AI 编写和修改。目前仍在学习相关实现，不将 AI 辅助生成的全部代码表述为独立编写或已经熟练掌握。

工程包含引擎模板及外部美术资源；这些内容不作为个人原创成果。`Variant_*` 模板目录也不作为自研系统展示。

接下来优先补充 [60–90 秒实机演示](Docs/演示录制清单.md)，完成打包与运行检查，再根据实测数据安排性能改进。存档持久化、联网玩法及性能指标未在本页作为已验证成果。
