# ShooterSam Git 工作流

## 项目约定

- UE 5.7，Windows，C++；主分支为 `main`，保持能编译、能运行。
- 新功能使用 `feat/描述`，修复使用 `fix/描述`；Codex 默认使用 `codex/描述`。
- 一个提交解决一个完整问题；相关 C++、蓝图、地图及配置应一起提交。
- `.uasset`、`.umap` 和二进制源素材使用 Git LFS。不要直接编辑 LFS 指针。
- `Source/`、`Config/`、`Content/`、`Docs/`、`.uproject` 和共享 `.vsconfig` 进入版本管理。
- 缓存、生成的工程文件、个人 IDE 设置由 `.gitignore` 排除。
- `Build/` 可能包含图标和打包配置，需要保留。未来引入仅有二进制的第三方插件时，需针对其必要文件调整忽略和检查规则。

## 本地首次使用 / 在新电脑上克隆后

先安装 Git for Windows、Git LFS、UE 5.7 和项目所需的 C++ 工具链。在项目根目录运行：

```powershell
git lfs install --local
git config --local core.autocrlf false
git config --local pull.ff only
git config --local fetch.prune true
git lfs pull  # 配置远程且存在已推送的 LFS 资源后运行
```

项目使用 `.gitattributes` 管理换行。以上配置只影响本仓库，不修改全局设置。
首次仓库初始化已经完成；克隆到其他机器后无需再次 `git init`。
通过 `.uproject` 生成 IDE 工程文件，再打开项目。

## 日常开发

在切换分支前保存所有资源、关闭 UE 编辑器，并确保当前修改已提交。

```powershell
git switch main
git pull --ff-only  # 仅接入远程并设置 upstream 后执行
git switch -c feat/wave-system

# 完成一项功能并在 UE 中保存；先查看改动。
git status --short
git diff

# 按本次功能选择路径，包含相关资源依赖。
git add Source/ShooterSamProject/WaveManager.cpp Source/ShooterSamProject/WaveManager.h
git diff --cached --stat
pwsh -NoProfile -File Scripts/Check-Git.ps1
git commit -m "feat: 完善波次生成规则"
```

上面的 `git add` 仅为示例；有蓝图或地图改动时应同时添加相应 `Content/` 路径。
提交前快速检查是显式命令，目前没有强制 pre-commit 钩子；Git LFS 的标准钩子由 `git lfs install --local` 安装。

## 验证与合并

```powershell
# 检查暂存区中的生成文件、冲突标记、大文件和 LFS 指针。
pwsh -NoProfile -File Scripts/Check-Git.ps1

# 完整编译 Editor 目标。
pwsh -NoProfile -File Scripts/Build.ps1

# 默认先编译，再运行所有 ShooterSam.* 自动化测试。
pwsh -NoProfile -File Scripts/Test.ps1

# 刚完成编译且代码未变化时，可跳过重复编译；也可缩小测试范围。
pwsh -NoProfile -File Scripts/Test.ps1 -SkipBuild -Filter ShooterSam.WaveManager.
```

这些脚本也支持 Windows PowerShell：将 `pwsh` 替换为 `powershell -ExecutionPolicy Bypass`（只对该次进程生效）。
脚本优先使用 `-EngineRoot` 参数，再查找 `UE_ROOT` 环境变量，最后读取 Epic Launcher 安装记录。
例如：`pwsh -File Scripts/Build.ps1 -EngineRoot 'D:\UnReal\UE_5.7'`。
`Build.ps1` 和 `Test.ps1` 的 `-ValidateOnly` 只检查引擎路径，不代表编译或测试通过。
运行编译和测试前关闭 UE 编辑器。测试默认使用无渲染模式，适合当前逻辑测试；视觉和玩法效果仍需在编辑器中实际验证。
编译日志与每次独立测试报告位于 `Saved/Workflow/`，不提交。
测试超时、非零退出码、缺失报告、零测试、失败或未完成测试都会报错。

本地单人开发验证通过后：

```powershell
git switch main
git merge --ff-only feat/wave-system
git branch -d feat/wave-system
```

如果无法快进，说明两个分支都有新提交：回到功能分支，用 `git merge main` 明确解决冲突并重新验证，再回到 `main` 快进合并。
不要自动选择整批“保留我方/对方”，尤其是蓝图和地图。
接入远程后，可将功能分支推送并创建 PR，通过检查后合并。多人审核和主分支保护在远程平台配置。

## UE 资源注意事项

- 蓝图和地图属于二进制包，不能像 C++ 一样逐行合并。尽量避免不同分支同时改同一个资源。
- 在 UE 内容浏览器中移动/重命名资源；按需要修复重定向器并提交相应修改。
- 不能仅提交代码而漏掉所引用的资源或配置。
- 多人协作时，确认服务器支持 Git LFS 文件锁后再配置 `lockable` 和资源锁定；本地阶段暂不启用只读锁属性。
- 如误将大资源直接加入 Git，先修正 `.gitattributes`，再重新暂存该文件；不要未经检查就迁移或改写已共享的历史。

## 版本和撤销

完成可演示版本且验证通过后，手动创建带说明的标签：

```powershell
git tag -a v0.1.0 -m "首个可演示版本"
```

已有共享提交需要撤销时使用 `git revert <提交号>`。不要向共享主分支强制推送，也不要用 `reset --hard` 清理尚未保存的工作。
首次基线提交仅记录当前项目快照，不自动代表已达到可发布质量；不会自动创建发布标签。

## 远程仓库

本项目的 `origin` 地址为 `https://github.com/BIUBIUBIU713/ShooterProject.git`。
仓库页面：<https://github.com/BIUBIUBIU713/ShooterProject>。
初始 `Content/` 约 2.65 GiB，后续修改二进制资产也会消耗 LFS 存储。

当前项目已设置 `origin`，无需重复添加。提交完成后推送主分支：

```powershell
git push -u origin main
```

在其他电脑安装 Git LFS 后，可克隆完整项目：

```powershell
git clone https://github.com/BIUBIUBIU713/ShooterProject.git
cd ShooterProject
git lfs install --local
git lfs pull
```

标准 LFS pre-push 钩子会先上传资源；首次上传耗时取决于网络速度。只有 `git push` 成功后，远程才包含对应提交。
如果因网络错误失败，修复网络后重新执行 `git push -u origin main`；不需要重新初始化仓库或强制推送。
如果服务端提示 LFS 配额不足，需要在 GitHub 账户侧处理配额后重试。
远程只包含已提交并推送的内容；未提交的代码、地图和蓝图仍然只在本地。
