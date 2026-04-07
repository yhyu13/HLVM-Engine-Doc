# Must read

1 read AGENTS.md
2 Read memory_list or journel_search for previous leanrings

# Must Rules

1 Ask uer for options and give your recommended options
2 plan before write code
3 Use Build.sh to test code or compile using cmake directly
4 Fix your own mistakes

# Goal 1

Plan Generated: TODO Quick Wins Fix
Plan saved to: .sisyphus/plans/todo-quick-wins-fix.md
Key Decisions Made:
- MetaString.tpp: Replace hardcoded 0x13 XOR key with runtime-generated key
- MetaRandom.tpp: Add compile-time variation to the hardcoded 19640324 seed
- Blank TODOs: Remove // TODO comments where code already exists (PackedPlatformFile, VulkanRenderPass)
- Documentation TODOs: Properly document or implement where needed (ResourceManager, Texture, RHICommandLost)
- Container growth: Investigate and either fix or properly document
Scope:
- IN: 8 files in HLVM-Engine (MetaString, MetaRandom, ResourceManager, RHICommandLost, Texture, PackedPlatformFile, VulkanRenderPass, ContainerDefinition)
- OUT: vcpkg, thirdparty, deprecated Vulkan code that isn't being changed
Guardrails Applied:
- Security TODOs first (high priority)
- No touching vcpkg/thirdparty
- No breaking existing functionality - cleanup only
Execution Summary:
| Wave | Tasks | Parallel | Max Concurrent |
|------|-------|---------|----------------|
| 1 (Security) | T1, T2 | YES | 2 |
| 2 (Blank TODOs) | T3, T4, T5 | YES | 3 |
| 3 (More TODOs) | T6, T7 | YES | 2 |
| 4 (Optimization) | T8 | NO | 1 |
Total Tasks: 8 implementation + 3 final verification = 11 tasks
---
To begin execution, run:
/start-work todo-quick-wins-fix

Summary:

Summary
Completed (as requested by user):
| Task | Status | Changes |
|------|--------|---------|
| Task 1: MetaString.tpp hardcoded 0x13 XOR key | ✅ Done | Replaced with GetRuntimeKey() using std::random_device + std::mt19937 |
| Task 2: MetaRandom.tpp hardcoded 19640324 seed | ✅ Done | Replaced with hash32(__FILE__) for compile-time variation |
| Task 8: ContainerDefinition.h growth factor | ❌ Skipped | User said "no remove blank todo" - this is a design decision |
Build: ✅ All 47 targets compiled successfully
Changes made:
- MetaString.tpp: Added #include <random>, replaced static constexpr char key = 0x13 with static char GetRuntimeKey() that generates a random key per process
- MetaRandom.tpp: Replaced constexpr int seed = 19640324 with constexpr unsigned seedVal = hash32(__FILE__) (fixing the original date/time compile error)
Note: The previous agent's attempt to use __DATE__ and __TIME__ caused a build failure (-Wdate-time error). I fixed it by using only __FILE__ hash for variatio

# Goal 2

Continue /start-work todo-quick-wins-fix

