# voice-center

Windows C++17 工业语音播报 Demo。UDP 监听设备 JSON，`run` 从 `0→1` 时按 FIFO 排队播报对应 WAV。

## 这个环境不能构建

当前容器是 **Linux/arm64**，没有 MSVC，跑不了 `.exe`。**构建和运行只能在客户的 Windows 机器上发生。**

- 要验证编译，走 SSH 到客户机执行（见 `~/Documents/deepseek-harness/default-workspace/SSH联调准备清单.md`）
- 不要声称「构建通过」「测试通过」，除非真的在 Windows 上跑过并看到输出
- 只做了静态阅读或代码走查时，明确说验证到了哪一层
- SSH 会话里没有音频设备：`--dry-run` 可验 UDP/队列/文件逻辑，真实出声必须由客户在桌面会话运行

## 开发流程

按 superpowers 的流程走，不要跳步：

1. **新功能或行为变更** → 先 `brainstorming`，产出 spec 到 `docs/superpowers/specs/`
2. **多步任务** → `writing-plans` 写计划到 `docs/superpowers/plans/`
3. **执行** → `executing-plans`（自己实现）或 `subagent-driven-development`（分派子任务）
4. **写实现代码前** → `test-driven-development`
5. **遇到 bug** → `systematic-debugging`，先定位根因再改
6. **声称完成前** → `verification-before-completion`，跑命令看输出
7. **提交前** → `git-commit-guard`，检查提交范围、生成 Commit Message、给出检查报告

例外：拼写、注释、单行明显修复可以跳过，但要说清楚为什么跳。

`.githooks/pre-commit` 会机械拦截构建产物、超大文件、密钥文件和冲突标记 —— 它只处理能靠脚本判定的事；代码逻辑是否合理仍然要靠 `git-commit-guard` 审。hook 通过不等于可以提交。


## 分支纪律（最高优先级，高于本文其余所有内容）

**你是仓库主人 `Lywess` 的助理 agent，不是外部协作者。** 你继承他的部分权限：

| 分支 | 你可以做什么 |
|---|---|
| `main` | **什么都不行**，除非他当次明确指令。这是发布线，不是试验场 |
| `develop` | 可以推送、可以把做完的分支合进来 |
| `<type>/<slug>` | 你自己的分支，`feat/` `fix/` `docs/` `chore/` 前缀 |

开工标准动作：

```bash
git fetch origin
git switch develop && git pull --ff-only
git switch -c feat/<slug>
```

仍然不做（对你、对任何人都成立）：

- ❌ 未经当次明确指令，对 `main` 做任何推送或合并
- ❌ `git push --force` 到共享分支
- ❌ 删除 `main`、`develop` 或他人创建的分支
- ❌ rebase / amend 已经推送的提交
- ❌ 把没验证过的改动合进 `develop`，然后说「应该没问题」

权限不等于随意。能推 `develop` 意味着**合进去的东西要能站住**，所以合并前该跑的验证一条都不能省。

动代码前先确认 `git branch --show-current`。

## 仓库约定

- 提交前缀：`feat:` / `fix:` / `docs:` / `chore:`
- 平台无关逻辑放 `src/core/`，平台相关只放 `src/platform/windows/`
- 设备主键是 `ip`；`name` 只用于显示和日志
- 只认 `run` 的 `0→1` 边沿；某 IP **首次出现**绝不触发
- 日志只追加 stdout —— 不做状态面板、不清屏、无 GUI
- 同一 IP 在 `cooldown_ms`（默认 5000）内重复边沿静默丢弃

## 不要做

- 不要为了「能编译」而修改平台抽象层的语义
- 不要删除或覆盖 `docs/superpowers/` 下的 spec 和 plan
- 客户机上的写操作、删除操作，先跟人确认

