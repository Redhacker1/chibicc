# Compiler Architecture & Optimization Guide

This document details the architecture, design principles, intermediate representations, analysis passes, optimization pipelines, and developer workflows for extending `chibicc`.

---

## 1. End-to-End Compiler Pipeline

The compilation pipeline transforms C source code into machine code across modular stages:

```
Source (.c / .h)
      │
      ▼
  Tokenizer       (tokenize.c)      ──> Token stream
      │
      ▼
 Preprocessor     (preprocess.c)    ──> Macro expansion, #include, conditional compilation
      │
      ▼
    Parser        (parse.c)         ──> Abstract Syntax Tree (AST / Node) & Type System (type.c)
      │
      ▼
 HLIR Lowering    (ir/hlir.c)       ──> High-Level Intermediate Representation (HLIRProg / HLIRFunction)
      │
      ▼
 HLIR Optimizer   (ir/hlir_opt*.c)  ──> High-level optimizations (Folding, Algebraic, CSE, Load/Store, Inlining)
      │
      ▼
 LLIR Lowering    (ir/ir.c)         ──> Low-Level 3-address IR with virtual registers (LLIRFunction)
      │
      ▼
 Analysis Engine  (ir/analysis.c)   ──> CFG construction, Dominator Tree, Liveness intervals
      │
      ▼
 LLIR Optimizer   (ir/opt_*.c)      ──> Low-level optimizations (Copy-prop, DCE, CFG, Peephole, Legalization)
      │
      ▼
Pass Manager (ir/pass_manager.c)──> Fixed-point iteration driver & Pass registry
      │
      ▼
Regalloc Engine   (ir/regalloc.c)   ──> Linear scan register allocator with spilling & coalescing
      │
      ▼
Codegen Backends  (codegen/*)       ──> Target emission: x86_64, m68k, z80
      │
      ▼
ABI & Object Fmt  (abi/*)           ──> SysV64, Win64, Win32, ELF/PE/COFF/Mach-O emission
```

---

## 2. High-Level Intermediate Representation (HLIR)

HLIR provides a high-level, SSA-like representation operating on values (`HLIRVal`) and typed instructions (`HLIRInsn`). It preserves type fidelity and variable symbol attachments (`Obj *var`), making high-level semantics easy to reason about and transform.

### Directory & Source Layout
All HLIR optimization passes reside in dedicated modules:
- `ir/hlir_opt.h`: Central optimizer interface, shared predicates (`hlir_is_pure`, `hlir_is_commutative`), and block-local tracking environments (`HLIREnv`).
- `ir/hlir_opt_fold.c`: Constant folding (binary arithmetic, bitwise, shifts, comparisons, type cast truncations/extensions) and operand canonicalization.
- `ir/hlir_opt_algebraic.c`: Algebraic identities (identities, annihilators, self-operands, negation rules, shift-arithmetic strength reduction, reassociation, cancellation, and absorption).
- `ir/hlir_opt_copy_prop.c`: Value alias tracking through type-compatible casts/copies across basic blocks.
- `ir/hlir_opt_cse.c`: Local Common Subexpression Elimination for pure expressions within basic blocks.
- `ir/hlir_opt_load_store.c`: Local Store-to-Load and Load-to-Load forwarding, combined with Dead Store Elimination.
- `ir/hlir_opt_control_flow.c`: Branch simplification on constant conditions, jump-to-next-label pruning, and jump threading.
- `ir/hlir_opt_dce.c`: Dead Value Elimination (removes unused pure definitions) and Unreachable Code Elimination.
- `ir/hlir_opt_inlining.c`: Inline expansion of small leaf and single-return functions.
- `ir/hlir_opt.c`: Pipeline driver and execution coordinator.

---

## 3. Low-Level Intermediate Representation (LLIR)

LLIR provides a machine-independent 3-address instruction format with virtual registers (`LLIRVReg`) and explicit basic blocks (`LLIRBlock`).

### Analysis Infrastructure (`ir/analysis.c`)
1. **Control Flow Graph (CFG)**:
   - Divides functions into basic blocks by identifying leaders (entry, labels, instructions after branches).
   - Computes directed predecessor and successor edges.
   - Computes reachability from the entry block.
2. **Dominator Tree (`IRDomTree`)**:
   - Implements the Cooper-Harvey-Kennedy iterative dominance algorithm.
   - Provides $O(1)$ / fast tree ancestor dominance queries via `ir_dom_dominates`.
3. **Liveness Analysis (`IRLiveness`)**:
   - Computes definition positions, last usage positions, and detects virtual registers that remain live across function calls (`is_live_across_calls`).
4. **IR Integrity Verifier**:
   - Validates bidirectional linked-list integrity and vreg index bounds.

### LLIR Optimization Passes
- `ir/opt_copy_prop.c`: Register copy propagation (`IR_MOV`) across basic blocks, resetting aliases at control-flow boundaries.
- `ir/opt_dce.c`: Iterative fixed-point Dead Code Elimination.
- `ir/opt_cfg.c`: CFG simplification, constant branch evaluation, unreachable code pruning, and identical branch folding.
- `ir/opt_peephole.c`: Windowed peephole optimizations, commutative canonicalization, algebraic identities, strength reduction (power-of-2 multiplication/division/modulo), and load/store forwarding.
- `ir/opt_legalize.c`: Instruction selection and legalization, standardizing addressing modes (`base + index * scale + disp`) and immediate operands for architecture code generators.

---

## 4. Pass Manager & Registry (`ir/pass_manager.c`)

Optimizations are managed through a centralized Pass Manager with dynamic pass discovery and CLI flag fine-tuning.

### Registered Passes Table
| Pass Name | Description | Default Level | Target IR |
|---|---|---|---|
| `hlir-const-fold` | Constant folding & canonicalization | `-O1` | HLIR |
| `hlir-algebraic` | Algebraic simplification & strength reduction | `-O1` | HLIR |
| `hlir-copy-prop` | Copy and alias propagation | `-O1` | HLIR |
| `hlir-local-cse` | Common subexpression elimination | `-O2` | HLIR |
| `hlir-load-store` | Store-to-load forwarding & dead store removal | `-O2` | HLIR |
| `hlir-control-flow` | Branch simplification & jump threading | `-O1` | HLIR |
| `hlir-dead-code` | Dead code & unused value elimination | `-O1` | HLIR |
| `hlir-inlining` | Function inlining | `-O2` | HLIR |
| `copy-prop` | LLIR copy propagation | `-O1` | LLIR |
| `peephole` | LLIR peephole & strength reduction | `-O2` | LLIR |
| `cfg-simplify` | LLIR CFG simplification | `-O1` | LLIR |
| `dce` | LLIR dead code elimination | `-O1` | LLIR |
| `verifier` | IR structural verification | `-O0` | LLIR |
| `legalize-isel` | Address mode & immediate legalization | `-O1` | LLIR |

---

## 5. How to Write and Add a New Optimization Pass

Adding a new pass in `chibicc` is straightforward and modular.

### Step 1: Implement the Pass Function
Create a function adhering to the `IRPassFunctionFn` signature (for LLIR) or `HLIRFunction -> bool` (for HLIR):

```c
// ir/opt_my_pass.c
#include "ir/opt.h"

bool ir_opt_my_pass(IRFunction *fn) {
  if (!fn) return false;
  bool changed = false;

  for (IRInsn *insn = fn->head; insn; insn = insn->next) {
    // Inspect and mutate instruction
    if (insn->kind == IR_ADD && insn->src2 && insn->src2->def_insn &&
        insn->src2->def_insn->kind == IR_IMM && insn->src2->def_insn->imm == 0) {
      insn->kind = IR_MOV;
      insn->src2 = NULL;
      changed = true;
    }
  }

  return changed;
}
```

### Step 2: Declare the Pass in Header
Add your prototype to `ir/opt.h`:
```c
bool ir_opt_my_pass(IRFunction *fn);
extern IRPass pass_my_pass;
```

### Step 3: Register in Pass Manager (`ir/pass_manager.c`)
```c
static bool run_pass_my_pass(IRFunction *fn, IRPassContext *ctx) {
  (void)ctx;
  return ir_opt_my_pass(fn);
}

IRPass pass_my_pass = {
  .name = "my-pass",
  .description = "Custom optimization pass",
  .type = IR_PASS_FUNCTION,
  .enabled = false,
  .default_opt_level = 2,
  .run_on_function = run_pass_my_pass,
};

// In ir_init_pass_registry():
ir_register_pass(&pass_my_pass);
```

---

## 6. Building and Running Tests

### CMake & CTest
```powershell
# Configure & Build
cmake -B build -G Ninja
cmake --build build

# Run CTest
cd build; ctest --output-on-failure
```

### PowerShell Test Runner
```powershell
.\run_tests.ps1
```

### Makefile (GCC / Clang)
```sh
make
make test
```
