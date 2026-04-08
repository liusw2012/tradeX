---
name: check-all
description: 执行tradeX项目全流程工程技术指标检查，覆盖从需求到发布运维的完整环节
---

## 执行范围

本命令执行以下环节的检查：

### 开发阶段
1. **SPEC文档评审** - /spec-review
2. **技术方案评审** - /design-review
3. **代码评审** - /code-review
4. **单元测试** - /unit-test
5. **提交前检查** - /pre-commit-check

### 测试阶段
6. **CI测试** - /ci-test
7. **功能测试** - /functional-test
8. **性能测试** - /performance-test
9. **兼容性测试** - /compatibility-test
10. **集成测试** - /integration-test
11. **边界测试** - /boundary-test
12. **测试报告** - /test-report

### 发布阶段
13. **发布方案评审** - /release-plan
14. **发布执行** - /release-execute
15. **发布报告** - /release-report

### 运维阶段
16. **监控评估** - /monitor
17. **应急预案** - /emergency
18. **上线后评估** - /online-assessment

## 执行方式

根据项目当前阶段选择性执行：

```bash
# 开发阶段检查
/spec-review
/design-review
/code-review
/unit-test

# 测试阶段检查
/ci-test
/functional-test
/performance-test
/compatibility-test
/integration-test
/boundary-test

# 发布阶段检查
/release-plan
/release-execute
/release-report

# 运维阶段检查
/monitor
/emergency
/online-assessment
```

## 输出格式

检查完成后，生成汇总报告：

```
# tradeX 全流程工程技术指标检查报告

## 检查概览
- 检查时间: YYYY-MM-DD HH:mm:ss
- 检查范围: [执行环节列表]
- 总体得分: XX%

## 环节得分汇总

| 环节 | 得分 | 状态 |
|------|------|------|
| SPEC评审 | XX% | [通过/警告/不通过] |
| 技术方案评审 | XX% | [通过/警告/不通过] |
| 代码评审 | XX% | [通过/警告/不通过] |
| 单元测试 | XX% | [通过/警告/不通过] |
| ... | ... | ... |

## 问题汇总
- Critical问题: XX个
- Blocker问题: XX个
- 优化建议: XX个

## 检查结论
[通过/警告/不通过] - 详细说明

## 后续行动
1. [优先级行动]
2. [其他行动]
```

## 注意事项

- 根据项目当前阶段选择需要执行的检查
- 建议按顺序执行各环节检查
- 检查结果应及时反馈给相关方
- 使用中文输出报告