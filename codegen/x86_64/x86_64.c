#include "chibicc.h"
#include "codegen/codegen.h"
#include "codegen/common/common.h"
#include "ir/ir.h"
#include "ir/regalloc.h"

static FILE *output_file;
static int depth;
Obj *current_fn = NULL;
static bool *current_needs_mat = NULL;

__attribute__((format(printf, 1, 2)))
static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fputc('\n', output_file);
}

// ============================================================================
// Materialization Analysis
// Determines whether virtual registers can be folded directly into memory operands,
// immediate instructions, or conditional branches, avoiding stack spill/load traffic.
// ============================================================================
static void compute_materialization(LLIRFunction *fn) {
  if (current_needs_mat) {
    free(current_needs_mat);
    current_needs_mat = NULL;
  }
  if (!fn || fn->num_vregs == 0)
    return;

  int *def_count = calloc(fn->num_vregs, sizeof(int));
  for (int i = 0; i < fn->num_vregs; i++) {
    if (fn->vregs[i])
      fn->vregs[i]->def_insn = NULL;
  }
  for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->dst) {
      def_count[insn->dst->id]++;
      insn->dst->def_insn = insn;
    }
  }
  for (int i = 0; i < fn->num_vregs; i++) {
    if (def_count[i] != 1 && fn->vregs[i])
      fn->vregs[i]->def_insn = NULL;
  }
  free(def_count);

  current_needs_mat = calloc(fn->num_vregs, sizeof(bool));
  for (int i = 0; i < fn->num_vregs; i++)
    current_needs_mat[i] = true;

  int *use_count = calloc(fn->num_vregs, sizeof(int));
  for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
    if (insn->kind == LLIR_BR_COND && insn->src1 && insn->src1->def_insn) {
      LLIRInsn *def = insn->src1->def_insn;
      if ((!def->src1 || !def->src1->ty || !is_flonum(def->src1->ty)) &&
          (def->kind == LLIR_CMP_EQ || def->kind == LLIR_CMP_NE ||
           def->kind == LLIR_CMP_LT || def->kind == LLIR_CMP_LE ||
           def->kind == LLIR_CMP_GT || def->kind == LLIR_CMP_GE ||
           def->kind == LLIR_LOGNOT)) {
        // Condition is folded directly into jump instruction without materializing boolean
        continue;
      }
    }
    if (insn->src1) use_count[insn->src1->id]++;
    if (insn->src2) use_count[insn->src2->id]++;
    if (insn->src3) use_count[insn->src3->id]++;
    for (int a = 0; a < insn->num_args; a++) {
      if (insn->args[a]) use_count[insn->args[a]->id]++;
    }
  }

  // Iteratively identify folded address computations, immediate operands, and dead pure vregs
  bool changed = true;
  while (changed) {
    changed = false;

    for (int i = 0; i < fn->num_vregs; i++) {
      LLIRVReg *v = fn->vregs[i];
      if (!v || !v->def_insn || !current_needs_mat[v->id])
        continue;

      LLIRInsn *def = v->def_insn;

      // Unused pure result
      if (use_count[v->id] <= 0) {
        if (def->kind == LLIR_IMM || def->kind == LLIR_FIMM || def->kind == LLIR_LEA || def->kind == LLIR_MOV ||
            def->kind == LLIR_ADD || def->kind == LLIR_SUB || def->kind == LLIR_CAST ||
            def->kind == LLIR_CMP_EQ || def->kind == LLIR_CMP_NE ||
            def->kind == LLIR_CMP_LT || def->kind == LLIR_CMP_LE ||
            def->kind == LLIR_CMP_GT || def->kind == LLIR_CMP_GE ||
            def->kind == LLIR_LOGNOT || def->kind == LLIR_NOT || def->kind == LLIR_NEG ||
            def->kind == LLIR_SHL || def->kind == LLIR_SHR ||
            def->kind == LLIR_AND || def->kind == LLIR_OR || def->kind == LLIR_XOR) {
          current_needs_mat[v->id] = false;
          changed = true;
          bool is_cmp = (def->kind >= LLIR_CMP_EQ && def->kind <= LLIR_CMP_GE) || def->kind == LLIR_LOGNOT;
          if (!is_cmp) {
            if (def->src1) use_count[def->src1->id]--;
            if (def->src2) use_count[def->src2->id]--;
            if (def->src3) use_count[def->src3->id]--;
          }
          continue;
        }
      }

      // 1. Foldable LEA of variables or labels
      if (def->kind == LLIR_LEA && (def->var || def->label)) {
        bool all_uses_foldable = true;
        for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
          if (insn->src1 == v) {
            if (insn->kind == LLIR_LOAD || insn->kind == LLIR_STORE) {
              // Direct memory load/store operand
            } else {
              all_uses_foldable = false;
              break;
            }
          }
          if (insn->src2 == v || insn->src3 == v) {
            all_uses_foldable = false;
            break;
          }
          for (int a = 0; a < insn->num_args; a++) {
            if (insn->args[a] == v) {
              all_uses_foldable = false;
              break;
            }
          }
        }

        if (all_uses_foldable) {
          current_needs_mat[v->id] = false;
          changed = true;
        }
      }

      // 2. Foldable IMM used in foldable immediate operations
      if (def->kind == LLIR_IMM) {
        int64_t imm = def->imm;
        bool imm32 = ((int32_t)imm == imm);
        bool all_uses_folded = true;
        for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
          if (insn->src1 == v) {
            if (imm32 && (insn->kind == LLIR_ADD || insn->kind == LLIR_MUL ||
                          insn->kind == LLIR_AND || insn->kind == LLIR_OR ||
                          insn->kind == LLIR_XOR) && (!insn->ty || !is_flonum(insn->ty))) {
              if (insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM) {
                all_uses_folded = false;
                break;
              }
            } else {
              all_uses_folded = false;
              break;
            }
          }
          if (insn->src2 == v) {
            if (imm32 && (insn->kind == LLIR_ADD || insn->kind == LLIR_SUB ||
                          insn->kind == LLIR_MUL || insn->kind == LLIR_AND ||
                          insn->kind == LLIR_OR || insn->kind == LLIR_XOR) && (!insn->ty || !is_flonum(insn->ty))) {
              // binop with imm
            } else if (insn->kind == LLIR_SHL || insn->kind == LLIR_SHR) {
              // shift with imm
            } else if (imm32 && (insn->kind == LLIR_CMP_EQ || insn->kind == LLIR_CMP_NE ||
                                 insn->kind == LLIR_CMP_LT || insn->kind == LLIR_CMP_LE ||
                                 insn->kind == LLIR_CMP_GT || insn->kind == LLIR_CMP_GE) &&
                       (!insn->src1 || !insn->src1->ty || !is_flonum(insn->src1->ty))) {
              // comparison with imm
            } else if (imm32 && insn->kind == LLIR_STORE && (!insn->ty || (insn->ty->kind != TY_STRUCT && insn->ty->kind != TY_UNION))) {
              // store imm
            } else {
              all_uses_folded = false;
              break;
            }
          }
          if (insn->src3 == v) {
            all_uses_folded = false;
            break;
          }
          for (int a = 0; a < insn->num_args; a++) {
            if (insn->args[a] == v) {
              all_uses_folded = false;
              break;
            }
          }
        }
        if (all_uses_folded) {
          current_needs_mat[v->id] = false;
          changed = true;
        }
      }

      // 4. Foldable CMP / LOGNOT used ONLY in conditional branches
      if ((def->kind == LLIR_CMP_EQ || def->kind == LLIR_CMP_NE ||
           def->kind == LLIR_CMP_LT || def->kind == LLIR_CMP_LE ||
           def->kind == LLIR_CMP_GT || def->kind == LLIR_CMP_GE ||
           def->kind == LLIR_LOGNOT) && (!def->src1 || !def->src1->ty || !is_flonum(def->src1->ty))) {
        bool all_uses_br = true;
        for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
          if (insn->src1 == v) {
            if (insn->kind != LLIR_BR_COND) {
              all_uses_br = false;
              break;
            }
          }
          if (insn->src2 == v || insn->src3 == v) {
            all_uses_br = false;
            break;
          }
          for (int a = 0; a < insn->num_args; a++) {
            if (insn->args[a] == v) {
              all_uses_br = false;
              break;
            }
          }
        }
        if (all_uses_br) {
          current_needs_mat[v->id] = false;
          changed = true;
        }
      }
    }
  }

  free(use_count);
}

// ============================================================================
// Register Names & Sizing Helpers
// ============================================================================
static const char *x86_reg32(const char *r64) {
  if (!strcmp(r64, "%rax")) return "%eax";
  if (!strcmp(r64, "%rcx")) return "%ecx";
  if (!strcmp(r64, "%rdx")) return "%edx";
  if (!strcmp(r64, "%rbx")) return "%ebx";
  if (!strcmp(r64, "%rsi")) return "%esi";
  if (!strcmp(r64, "%rdi")) return "%edi";
  if (!strcmp(r64, "%rbp")) return "%ebp";
  if (!strcmp(r64, "%rsp")) return "%esp";
  if (!strcmp(r64, "%r8"))  return "%r8d";
  if (!strcmp(r64, "%r9"))  return "%r9d";
  if (!strcmp(r64, "%r10")) return "%r10d";
  if (!strcmp(r64, "%r11")) return "%r11d";
  if (!strcmp(r64, "%r12")) return "%r12d";
  if (!strcmp(r64, "%r13")) return "%r13d";
  if (!strcmp(r64, "%r14")) return "%r14d";
  if (!strcmp(r64, "%r15")) return "%r15d";
  return r64;
}

static const char *x86_reg16(const char *r64) {
  if (!strcmp(r64, "%rax")) return "%ax";
  if (!strcmp(r64, "%rcx")) return "%cx";
  if (!strcmp(r64, "%rdx")) return "%dx";
  if (!strcmp(r64, "%rbx")) return "%bx";
  if (!strcmp(r64, "%rsi")) return "%si";
  if (!strcmp(r64, "%rdi")) return "%di";
  if (!strcmp(r64, "%rbp")) return "%bp";
  if (!strcmp(r64, "%rsp")) return "%sp";
  if (!strcmp(r64, "%r8"))  return "%r8w";
  if (!strcmp(r64, "%r9"))  return "%r9w";
  if (!strcmp(r64, "%r10")) return "%r10w";
  if (!strcmp(r64, "%r11")) return "%r11w";
  if (!strcmp(r64, "%r12")) return "%r12w";
  if (!strcmp(r64, "%r13")) return "%r13w";
  if (!strcmp(r64, "%r14")) return "%r14w";
  if (!strcmp(r64, "%r15")) return "%r15w";
  return r64;
}

static const char *x86_reg8(const char *r64) {
  if (!strcmp(r64, "%rax")) return "%al";
  if (!strcmp(r64, "%rcx")) return "%cl";
  if (!strcmp(r64, "%rdx")) return "%dl";
  if (!strcmp(r64, "%rbx")) return "%bl";
  if (!strcmp(r64, "%rsi")) return "%sil";
  if (!strcmp(r64, "%rdi")) return "%dil";
  if (!strcmp(r64, "%rbp")) return "%bpl";
  if (!strcmp(r64, "%rsp")) return "%spl";
  if (!strcmp(r64, "%r8"))  return "%r8b";
  if (!strcmp(r64, "%r9"))  return "%r9b";
  if (!strcmp(r64, "%r10")) return "%r10b";
  if (!strcmp(r64, "%r11")) return "%r11b";
  if (!strcmp(r64, "%r12")) return "%r12b";
  if (!strcmp(r64, "%r13")) return "%r13b";
  if (!strcmp(r64, "%r14")) return "%r14b";
  if (!strcmp(r64, "%r15")) return "%r15b";
  return r64;
}

static int x86_int_size(Type *ty) {
  if (!ty)
    return 8;

  switch (ty->size) {
    case 1:
    case 2:
    case 4:
    case 8:
      return ty->size;
    default:
      return 8;
  }
}

// ============================================================================
// Data Section Emission
// ============================================================================
static void emit_data(Obj *prog, FILE *out) {
  (void)out;

  for (Obj *var = prog; var; var = var->next) {
    if (var->is_asm) {
      println("  .text");
      println("  %s", var->asm_str ? var->asm_str : "");
      continue;
    }
    if (var->is_function || !var->is_definition)
      continue;

    if (current_objfmt && current_objfmt->emit_var_decl)
      current_objfmt->emit_var_decl(var, output_file);
    else if (!var->is_static)
      println("  .globl %s", var->name);

    int align =
      (opt_fpic && var->ty->kind == TY_ARRAY && var->ty->size >= 16)
        ? MAX(16, var->align)
        : var->align;

    if (opt_fcommon && var->is_tentative) {
      println("  .comm %s, %d, %d", var->name, var->ty->size, align);
      continue;
    }

    if (var->init_data) {
      if (var->is_tls) {
        if (current_objfmt == &objfmt_coff)
          println("  .section .tls$");
        else
          println("  .section .tdata,\"awT\",@progbits");
      } else if (var->is_readonly) {
        if (opt_fdata_sections) {
          if (current_objfmt == &objfmt_coff)
            println("  .section .rdata$%s,\"dr\"", var->name);
          else if (current_objfmt == &objfmt_macho)
            println("  .section __TEXT,__cstring,cstring_literals");
          else
            println("  .section .rodata.%s,\"a\",@progbits", var->name);
        } else {
          if (current_objfmt == &objfmt_coff)
            println("  .section .rdata,\"dr\"");
          else if (current_objfmt == &objfmt_macho)
            println("  .section __TEXT,__cstring,cstring_literals");
          else
            println("  .section .rodata");
        }
      } else {
        if (opt_fdata_sections) {
          if (current_objfmt == &objfmt_coff)
            println("  .section .data$%s,\"dw\"", var->name);
          else if (current_objfmt == &objfmt_macho)
            println("  .section __DATA,__data");
          else
            println("  .section .data.%s,\"aw\",@progbits", var->name);
        } else {
          println("  .data");
        }
      }

      if (current_objfmt && current_objfmt->emit_var_type_size)
        current_objfmt->emit_var_type_size(var, output_file);

      println("  .align %d", align);
      println("%s:", var->name);

      Relocation *rel = var->rel;
      int pos = 0;

      while (pos < var->ty->size) {
        if (rel && rel->offset == pos) {
          println("  .quad %s%+ld", *rel->label, rel->addend);
          rel = rel->next;
          pos += 8;
        } else {
          int end = rel ? rel->offset : var->ty->size;
          if (end > var->ty->size)
            end = var->ty->size;

          int z = pos;
          while (z < end && var->init_data[z] == 0)
            z++;
          if (z - pos >= 4) {
            println("  .zero %d", z - pos);
            pos = z;
            continue;
          }

          int count = end - pos;
          if (count > 16)
            count = 16;

          for (int k = 1; k < count; k++) {
            if (pos + k + 3 < end &&
                var->init_data[pos + k] == 0 &&
                var->init_data[pos + k + 1] == 0 &&
                var->init_data[pos + k + 2] == 0 &&
                var->init_data[pos + k + 3] == 0) {
              count = k;
              break;
            }
          }

          char buf[512];
          int off = sprintf(buf, "  .byte %d", (unsigned char)var->init_data[pos]);
          for (int i = 1; i < count; i++) {
            off += sprintf(buf + off, ", %d", (unsigned char)var->init_data[pos + i]);
          }
          println("%s", buf);
          pos += count;
        }
      }

      continue;
    }

    if (var->is_tls) {
      if (current_objfmt == &objfmt_coff)
        println("  .section .tls$");
      else
        println("  .section .tbss,\"awT\",@nobits");
    } else if (opt_fdata_sections) {
      if (current_objfmt == &objfmt_coff)
        println("  .section .bss$%s,\"bw\"", var->name);
      else if (current_objfmt == &objfmt_macho)
        println("  .section __DATA,__bss");
      else
        println("  .section .bss.%s,\"aw\",@nobits", var->name);
    } else {
      println("  .bss");
    }

    println("  .align %d", align);
    println("%s:", var->name);
    println("  .zero %d", var->ty->size);
  }
}

// ============================================================================
// Virtual Register Spill/Load Caching
// ============================================================================
static LLIRVReg *cached_rax_vreg = NULL;
static const char *x86_64_gp_regs[] = { "%rbx", "%r12", "%r13", "%r14", "%r15", "%r11" };
#define NUM_X86_64_GP_REGS 6

static void invalidate_cached_regs(void) {
  cached_rax_vreg = NULL;
}

static void load_vreg(LLIRVReg *v, const char *reg) {
  if (!v) return;
  if (!strcmp(reg, "%rax") && cached_rax_vreg == v)
    return;
  if (v->phys_reg >= 0 && v->phys_reg < NUM_X86_64_GP_REGS && !v->is_float) {
    const char *src = x86_64_gp_regs[v->phys_reg];
    if (strcmp(src, reg) != 0) {
      println("  movq %s, %s", src, reg);
      if (!strcmp(reg, "%rax"))
        cached_rax_vreg = v;
    }
    return;
  }
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  if (v->is_float) {
    if (reg[1] == 'x') { // %xmm...
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movss %d(%%rbp), %s", offset, reg);
      else
        println("  movsd %d(%%rbp), %s", offset, reg);
    } else {
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movq %d(%%rbp), %s", offset, reg);
    }
    cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
  } else {
    int sz = v->ty ? v->ty->size : 8;
    if (reg[1] == 'x') { // %xmm...
      println("  movq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = NULL;
    } else if (cached_rax_vreg == v && !strcmp(reg, "%rax")) {
      return;
    } else if (cached_rax_vreg == v && strcmp(reg, "%rax") != 0) {
      println("  movq %%rax, %s", reg);
    } else if (sz == 1) {
      if (v->ty && v->ty->is_unsigned)
        println("  movzbl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movsbq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
    } else if (sz == 2) {
      if (v->ty && v->ty->is_unsigned)
        println("  movzwl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movswq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
    } else if (sz == 4) {
      if (v->ty && v->ty->is_unsigned)
        println("  movl %d(%%rbp), %s", offset, x86_reg32(reg));
      else
        println("  movslq %d(%%rbp), %s", offset, reg);
      cached_rax_vreg = (!strcmp(reg, "%rax")) ? v : NULL;
    } else {
      println("  movq %d(%%rbp), %s", offset, reg);
      if (!strcmp(reg, "%rax"))
        cached_rax_vreg = v;
    }
  }
}

static void store_vreg(const char *reg, LLIRVReg *v) {
  if (!v) return;
  if (current_needs_mat && !current_needs_mat[v->id]) {
    if (!strcmp(reg, "%rax"))
      cached_rax_vreg = v;
    else
      cached_rax_vreg = NULL;
    return;
  }
  if (v->phys_reg >= 0 && v->phys_reg < NUM_X86_64_GP_REGS && !v->is_float) {
    const char *dst = x86_64_gp_regs[v->phys_reg];
    if (strcmp(reg, dst) != 0) {
      println("  movq %s, %s", reg, dst);
    }
    if (!strcmp(reg, "%rax"))
      cached_rax_vreg = v;
    else
      cached_rax_vreg = NULL;
    return;
  }
  int offset = v->spill_offset ? v->spill_offset : -((v->id + 1) * 8);
  if (v->is_float) {
    if (reg[1] == 'x') {
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movss %s, %d(%%rbp)", reg, offset);
      else
        println("  movsd %s, %d(%%rbp)", reg, offset);
    } else {
      if (v->ty && v->ty->kind == TY_FLOAT)
        println("  movl %s, %d(%%rbp)", x86_reg32(reg), offset);
      else
        println("  movq %s, %d(%%rbp)", reg, offset);
    }
    cached_rax_vreg = NULL;
  } else {
    if (reg[1] == 'x') {
      println("  movq %s, %d(%%rbp)", reg, offset);
      cached_rax_vreg = NULL;
    } else {
      println("  movq %s, %d(%%rbp)", reg, offset);
      if (!strcmp(reg, "%rax"))
        cached_rax_vreg = v;
    }
  }
}

// ============================================================================
// Memory Addressing & Displacement
// ============================================================================
typedef struct {
  const char *base_reg;
  int offset;
  char buf[64];
} X86Addr;

static void x86_get_insn_addr(LLIRInsn *insn, LLIRVReg *fallback_vreg, X86Addr *addr) {
  if (insn && insn->has_mem_op) {
    if (insn->var) {
      if (insn->var->is_local) {
        int total_off = insn->var->offset + (int)insn->disp;
        addr->base_reg = "%rbp";
        addr->offset = total_off;
        sprintf(addr->buf, "%d(%%rbp)", total_off);
        return;
      } else {
        addr->base_reg = "%rip";
        addr->offset = (int)insn->disp;
        if (insn->disp == 0)
          sprintf(addr->buf, "%s(%%rip)", insn->var->name);
        else
          sprintf(addr->buf, "%s%+d(%%rip)", insn->var->name, (int)insn->disp);
        return;
      }
    } else if (insn->label) {
      addr->base_reg = "%rip";
      addr->offset = (int)insn->disp;
      if (insn->disp == 0)
        sprintf(addr->buf, "%s(%%rip)", insn->label);
      else
        sprintf(addr->buf, "%s%+d(%%rip)", insn->label, (int)insn->disp);
      return;
    } else if (insn->base_reg) {
      const char *breg = "%rax";
      if (insn->base_reg->phys_reg >= 0 && insn->base_reg->phys_reg < NUM_X86_64_GP_REGS && !insn->base_reg->is_float) {
        breg = x86_64_gp_regs[insn->base_reg->phys_reg];
      } else {
        load_vreg(insn->base_reg, "%rax");
      }
      if (insn->index_reg) {
        const char *ireg = "%rcx";
        if (insn->index_reg->phys_reg >= 0 && insn->index_reg->phys_reg < NUM_X86_64_GP_REGS && !insn->index_reg->is_float) {
          ireg = x86_64_gp_regs[insn->index_reg->phys_reg];
        } else {
          load_vreg(insn->index_reg, "%rcx");
        }
        int sc = insn->scale > 0 ? insn->scale : 1;
        if (insn->disp == 0)
          sprintf(addr->buf, "(%s,%s,%d)", breg, ireg, sc);
        else
          sprintf(addr->buf, "%d(%s,%s,%d)", (int)insn->disp, breg, ireg, sc);
        addr->base_reg = breg;
        addr->offset = (int)insn->disp;
        return;
      } else {
        if (insn->disp == 0)
          sprintf(addr->buf, "(%s)", breg);
        else
          sprintf(addr->buf, "%d(%s)", (int)insn->disp, breg);
        addr->base_reg = breg;
        addr->offset = (int)insn->disp;
        return;
      }
    }
  }

  LLIRVReg *addr_vreg = fallback_vreg;
  if (!addr_vreg) {
    addr->base_reg = "%rax";
    addr->offset = 0;
    sprintf(addr->buf, "(%%rax)");
    return;
  }

  // Fallback: addr_vreg is directly in a physical register
  if (addr_vreg->phys_reg >= 0 && addr_vreg->phys_reg < NUM_X86_64_GP_REGS && !addr_vreg->is_float) {
    const char *breg = x86_64_gp_regs[addr_vreg->phys_reg];
    addr->base_reg = breg;
    addr->offset = 0;
    sprintf(addr->buf, "(%s)", breg);
    return;
  }

  // Default: load address into %rax
  load_vreg(addr_vreg, "%rax");
  addr->base_reg = "%rax";
  addr->offset = 0;
  sprintf(addr->buf, "(%%rax)");
}

static void x86_get_addr(LLIRVReg *addr_vreg, X86Addr *addr) {
  x86_get_insn_addr(NULL, addr_vreg, addr);
}

// ============================================================================
// Instruction Generation Helpers
// ============================================================================

// Emits loading an integer constant with optimal instruction encoding
static void x86_emit_load_imm(LLIRInsn *insn) {
  if (insn->dst && !insn->dst->is_float && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS) {
    const char *dst = x86_64_gp_regs[insn->dst->phys_reg];
    if (insn->imm == 0)
      println("  xor %s, %s", x86_reg32(dst), x86_reg32(dst));
    else if (insn->imm > 0 && (uint64_t)insn->imm <= 0xFFFFFFFFULL)
      println("  mov $%u, %s", (uint32_t)insn->imm, x86_reg32(dst));
    else if ((int64_t)(int32_t)insn->imm == insn->imm)
      println("  mov $%lld, %s", (long long)insn->imm, dst);
    else
      println("  movabs $%lld, %s", (long long)insn->imm, dst);
    return;
  }

  if (insn->imm == 0)
    println("  xor %%eax, %%eax");
  else if (insn->imm > 0 && (uint64_t)insn->imm <= 0xFFFFFFFFULL)
    println("  mov $%u, %%eax", (uint32_t)insn->imm);
  else if ((int64_t)(int32_t)insn->imm == insn->imm)
    println("  mov $%lld, %%rax", (long long)insn->imm);
  else
    println("  movabs $%lld, %%rax", (long long)insn->imm);

  store_vreg("%rax", insn->dst);
}

// Emits loading a floating-point constant using SSE2 vector instructions
static void x86_emit_load_fimm(LLIRInsn *insn) {
  if (insn->ty && insn->ty->kind == TY_FLOAT) {
    if (insn->fimm == 0.0) {
      println("  xorps %%xmm0, %%xmm0");
    } else {
      union {
        float f;
        uint32_t u;
      } u = { .f = (float)insn->fimm };

      println("  mov $%u, %%eax", u.u);
      println("  movd %%eax, %%xmm0");
    }
  } else {
    if (insn->fimm == 0.0) {
      println("  xorpd %%xmm0, %%xmm0");
    } else {
      union {
        double d;
        uint64_t u;
      } u = { .d = insn->fimm };

      println("  movabs $%llu, %%rax", (unsigned long long)u.u);
      println("  movq %%rax, %%xmm0");
    }
  }

  store_vreg("%xmm0", insn->dst);
}

static void x86_64_emit_int_result(LLIRInsn *insn, const char *op) {
  load_vreg(insn->src1, "%rax");
  load_vreg(insn->src2, "%rdx");
  println("  %s %%rdx, %%rax", op);
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_float_binop(LLIRInsn *insn,
                                    const char *f32,
                                    const char *f64) {
  load_vreg(insn->src1, "%xmm0");
  load_vreg(insn->src2, "%xmm1");
  println("  %s %%xmm1, %%xmm0",
          insn->ty->kind == TY_FLOAT ? f32 : f64);
  store_vreg("%xmm0", insn->dst);
}

static void x86_64_emit_binop_imm(LLIRInsn *insn,
                                  const char *iop,
                                  const char *f32,
                                  const char *f64,
                                  bool is_commutative) {
  if (insn->ty && is_flonum(insn->ty)) {
    x86_64_emit_float_binop(insn, f32, f64);
    return;
  }

  bool is_imm = insn->is_imm_op;
  int64_t imm = insn->imm;
  LLIRVReg *base = insn->src1;

  if (!is_imm) {
    bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
    bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
    if (s2_imm && (int32_t)insn->src2->def_insn->imm == insn->src2->def_insn->imm) {
      is_imm = true;
      imm = insn->src2->def_insn->imm;
      base = insn->src1;
    } else if (is_commutative && s1_imm && (int32_t)insn->src1->def_insn->imm == insn->src1->def_insn->imm) {
      is_imm = true;
      imm = insn->src1->def_insn->imm;
      base = insn->src2;
    }
  }

  int sz = x86_int_size(insn->ty);

  // If dst has a physical register assigned:
  if (insn->dst && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS && !insn->dst->is_float) {
    const char *dreg = x86_64_gp_regs[insn->dst->phys_reg];
    const char *dreg_sz = (sz <= 4) ? x86_reg32(dreg) : dreg;

    if (is_imm && base) {
      load_vreg(base, dreg);
      if (!strcmp(iop, "add")) {
        if (imm == 1) println("  inc %s", dreg_sz);
        else if (imm == -1) println("  dec %s", dreg_sz);
        else if (imm != 0) println("  add $%lld, %s", (long long)imm, dreg_sz);
      } else if (!strcmp(iop, "sub")) {
        if (imm == 1) println("  dec %s", dreg_sz);
        else if (imm == -1) println("  inc %s", dreg_sz);
        else if (imm != 0) println("  sub $%lld, %s", (long long)imm, dreg_sz);
      } else if (!strcmp(iop, "imul")) {
        println("  imul $%lld, %s, %s", (long long)imm, dreg_sz, dreg_sz);
      } else {
        println("  %s $%lld, %s", iop, (long long)imm, dreg_sz);
      }
      return;
    }

    if (insn->src1 && insn->src1->phys_reg == insn->dst->phys_reg) {
      if (insn->src2 && insn->src2->phys_reg >= 0 && insn->src2->phys_reg < NUM_X86_64_GP_REGS && !insn->src2->is_float) {
        const char *s2 = x86_64_gp_regs[insn->src2->phys_reg];
        println("  %s %s, %s", iop, (sz <= 4) ? x86_reg32(s2) : s2, dreg_sz);
      } else {
        load_vreg(insn->src2, "%rdx");
        println("  %s %s, %s", iop, (sz <= 4) ? "%edx" : "%rdx", dreg_sz);
      }
      return;
    }

    if (is_commutative && insn->src2 && insn->src2->phys_reg == insn->dst->phys_reg) {
      if (insn->src1 && insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS && !insn->src1->is_float) {
        const char *s1 = x86_64_gp_regs[insn->src1->phys_reg];
        println("  %s %s, %s", iop, (sz <= 4) ? x86_reg32(s1) : s1, dreg_sz);
      } else {
        load_vreg(insn->src1, "%rax");
        println("  %s %s, %s", iop, (sz <= 4) ? "%eax" : "%rax", dreg_sz);
      }
      return;
    }

    load_vreg(insn->src1, dreg);
    if (insn->src2 && insn->src2->phys_reg >= 0 && insn->src2->phys_reg < NUM_X86_64_GP_REGS && !insn->src2->is_float) {
      const char *s2 = x86_64_gp_regs[insn->src2->phys_reg];
      println("  %s %s, %s", iop, (sz <= 4) ? x86_reg32(s2) : s2, dreg_sz);
    } else {
      load_vreg(insn->src2, "%rdx");
      println("  %s %s, %s", iop, (sz <= 4) ? "%edx" : "%rdx", dreg_sz);
    }
    return;
  }

  if (is_imm && base) {
    load_vreg(base, "%rax");
    if (!strcmp(iop, "add")) {
      if (imm == 1) println(sz <= 4 ? "  inc %%eax" : "  inc %%rax");
      else if (imm == -1) println(sz <= 4 ? "  dec %%eax" : "  dec %%rax");
      else if (imm != 0) println(sz <= 4 ? "  add $%lld, %%eax" : "  add $%lld, %%rax", (long long)imm);
    } else if (!strcmp(iop, "sub")) {
      if (imm == 1) println(sz <= 4 ? "  dec %%eax" : "  dec %%rax");
      else if (imm == -1) println(sz <= 4 ? "  inc %%eax" : "  inc %%rax");
      else if (imm != 0) println(sz <= 4 ? "  sub $%lld, %%eax" : "  sub $%lld, %%rax", (long long)imm);
    } else if (!strcmp(iop, "imul")) {
      println(sz <= 4 ? "  imul $%lld, %%eax, %%eax" : "  imul $%lld, %%rax, %%rax", (long long)imm);
    } else {
      println(sz <= 4 ? "  %s $%lld, %%eax" : "  %s $%lld, %%rax", iop, (long long)imm);
    }
    store_vreg("%rax", insn->dst);
    return;
  }

  load_vreg(insn->src1, "%rax");
  if (insn->src2 && insn->src2->phys_reg >= 0 && insn->src2->phys_reg < NUM_X86_64_GP_REGS && !insn->src2->is_float) {
    const char *s2 = x86_64_gp_regs[insn->src2->phys_reg];
    println("  %s %s, %s", iop, (sz <= 4) ? x86_reg32(s2) : s2, (sz <= 4) ? "%eax" : "%rax");
  } else {
    load_vreg(insn->src2, "%rdx");
    println("  %s %s, %s", iop, (sz <= 4) ? "%edx" : "%rdx", (sz <= 4) ? "%eax" : "%rax");
  }
  store_vreg("%rax", insn->dst);
}

// Emits signed/unsigned integer division and modulus
static void x86_64_emit_divmod(LLIRInsn *insn, bool is_mod) {
  if (insn->ty && is_flonum(insn->ty)) {
    x86_64_emit_float_binop(insn, "divss", "divsd");
    return;
  }

  load_vreg(insn->src1, "%rax");
  load_vreg(insn->src2, "%rcx");

  if (insn->ty && insn->ty->size <= 4) {
    if (insn->ty->is_unsigned) {
      println("  xor %%edx, %%edx");
      println("  div %%ecx");
    } else {
      println("  cdq");
      println("  idiv %%ecx");
    }
  } else {
    if (insn->ty && insn->ty->is_unsigned) {
      println("  xor %%edx, %%edx");
      println("  div %%rcx");
    } else {
      println("  cqo");
      println("  idiv %%rcx");
    }
  }

  store_vreg(is_mod ? "%rdx" : "%rax", insn->dst);
}

// Emits bitwise shift operations with immediate or variable counts
static void x86_64_emit_shift(LLIRInsn *insn, bool is_shl) {
  int sz = x86_int_size(insn->ty);
  const char *dst_reg = "%rax";
  bool direct_dst = false;
  if (insn->dst && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS && !insn->dst->is_float) {
    dst_reg = x86_64_gp_regs[insn->dst->phys_reg];
    direct_dst = true;
  }
  const char *dreg_sz = (sz <= 4) ? x86_reg32(dst_reg) : dst_reg;

  bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
  if (s2_imm) {
    int64_t imm = insn->src2->def_insn->imm & 63;
    load_vreg(insn->src1, dst_reg);
    if (is_shl) {
      if (imm == 1)
        println("  shl $1, %s", dreg_sz);
      else if (imm > 1)
        println("  shl $%lld, %s", (long long)imm, dreg_sz);
    } else {
      bool uns = insn->ty && insn->ty->is_unsigned;
      const char *op = uns ? "shr" : "sar";
      if (imm == 1)
        println("  %s $1, %s", op, dreg_sz);
      else if (imm > 1)
        println("  %s $%lld, %s", op, (long long)imm, dreg_sz);
    }
    if (!direct_dst)
      store_vreg("%rax", insn->dst);
    return;
  }

  load_vreg(insn->src1, dst_reg);
  load_vreg(insn->src2, "%rcx");

  if (is_shl) {
    println("  shl %%cl, %s", dreg_sz);
  } else {
    if (insn->ty && insn->ty->is_unsigned)
      println("  shr %%cl, %s", dreg_sz);
    else
      println("  sar %%cl, %s", dreg_sz);
  }

  if (!direct_dst)
    store_vreg("%rax", insn->dst);
}

static void x86_64_emit_cmp_result(const char *cc) {
  println("  set%s %%al", cc);
  println("  movzbl %%al, %%eax");
}

// Unified integer comparison helper that sets up flags and computes branch/set cc
static void x86_emit_int_compare(LLIRInsn *insn, const char **out_cc, const char **out_inv_cc) {
  int sz1 = insn->src1 && insn->src1->ty ? insn->src1->ty->size : 8;
  int sz2 = insn->src2 && insn->src2->ty ? insn->src2->ty->size : 8;
  int cmp_sz = MAX(sz1, sz2);
  bool is_imm = insn->is_imm_op;
  int64_t imm = insn->imm;
  bool s1_imm = insn->src1 && insn->src1->def_insn && insn->src1->def_insn->kind == LLIR_IMM;
  bool s2_imm = insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM;
  bool uns = insn->src1 && insn->src1->ty && insn->src1->ty->is_unsigned;

  const char *cc = "e";
  const char *inv_cc = "ne";

  if (!is_imm && s1_imm && s2_imm) {
    int64_t v1 = insn->src1->def_insn->imm;
    int64_t v2 = insn->src2->def_insn->imm;
    if (v1 == 0)
      println("  xor %%eax, %%eax");
    else if (cmp_sz <= 4 || (v1 > 0 && (uint64_t)v1 <= 0xFFFFFFFFULL))
      println("  mov $%u, %%eax", (uint32_t)v1);
    else if ((int64_t)(int32_t)v1 == v1)
      println("  mov $%lld, %%rax", (long long)v1);
    else
      println("  movabs $%lld, %%rax", (long long)v1);

    if (v2 == 0)
      println(cmp_sz <= 4 ? "  test %%eax, %%eax" : "  test %%rax, %%rax");
    else if (cmp_sz <= 4)
      println("  cmp $%lld, %%eax", (long long)(int32_t)v2);
    else
      println("  cmp $%lld, %%rax", (long long)v2);

    switch (insn->kind) {
    case LLIR_CMP_EQ: cc = "e";  inv_cc = "ne"; break;
    case LLIR_CMP_NE: cc = "ne"; inv_cc = "e";  break;
    case LLIR_CMP_LT: cc = uns ? "b" : "l";   inv_cc = uns ? "ae" : "ge"; break;
    case LLIR_CMP_LE: cc = uns ? "be" : "le"; inv_cc = uns ? "a" : "g";   break;
    case LLIR_CMP_GT: cc = uns ? "a" : "g";   inv_cc = uns ? "be" : "le"; break;
    case LLIR_CMP_GE: cc = uns ? "ae" : "ge"; inv_cc = uns ? "b" : "l";   break;
    default: break;
    }
  } else if (is_imm || (s2_imm && (int32_t)insn->src2->def_insn->imm == insn->src2->def_insn->imm)) {
    if (!is_imm)
      imm = insn->src2->def_insn->imm;
    const char *r1 = "%rax";
    if (insn->src1 && insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS && !insn->src1->is_float)
      r1 = x86_64_gp_regs[insn->src1->phys_reg];
    else
      load_vreg(insn->src1, "%rax");

    const char *r1_sz = (cmp_sz <= 4) ? x86_reg32(r1) : r1;
    if (imm == 0)
      println("  test %s, %s", r1_sz, r1_sz);
    else
      println("  cmp $%lld, %s", (long long)imm, r1_sz);

    switch (insn->kind) {
    case LLIR_CMP_EQ: cc = "e";  inv_cc = "ne"; break;
    case LLIR_CMP_NE: cc = "ne"; inv_cc = "e";  break;
    case LLIR_CMP_LT: cc = uns ? "b" : "l";   inv_cc = uns ? "ae" : "ge"; break;
    case LLIR_CMP_LE: cc = uns ? "be" : "le"; inv_cc = uns ? "a" : "g";   break;
    case LLIR_CMP_GT: cc = uns ? "a" : "g";   inv_cc = uns ? "be" : "le"; break;
    case LLIR_CMP_GE: cc = uns ? "ae" : "ge"; inv_cc = uns ? "b" : "l";   break;
    default: break;
    }
  } else {
    const char *r1 = "%rax";
    const char *r2 = "%rdx";
    if (insn->src1 && insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS && !insn->src1->is_float)
      r1 = x86_64_gp_regs[insn->src1->phys_reg];
    else
      load_vreg(insn->src1, "%rax");

    if (insn->src2 && insn->src2->phys_reg >= 0 && insn->src2->phys_reg < NUM_X86_64_GP_REGS && !insn->src2->is_float &&
        insn->src2->phys_reg != (insn->src1 ? insn->src1->phys_reg : -1))
      r2 = x86_64_gp_regs[insn->src2->phys_reg];
    else
      load_vreg(insn->src2, "%rdx");

    const char *r1_sz = (cmp_sz <= 4) ? x86_reg32(r1) : r1;
    const char *r2_sz = (cmp_sz <= 4) ? x86_reg32(r2) : r2;
    println("  cmp %s, %s", r2_sz, r1_sz);

    switch (insn->kind) {
    case LLIR_CMP_EQ: cc = "e";  inv_cc = "ne"; break;
    case LLIR_CMP_NE: cc = "ne"; inv_cc = "e";  break;
    case LLIR_CMP_LT: cc = uns ? "b" : "l";   inv_cc = uns ? "ae" : "ge"; break;
    case LLIR_CMP_LE: cc = uns ? "be" : "le"; inv_cc = uns ? "a" : "g";   break;
    case LLIR_CMP_GT: cc = uns ? "a" : "g";   inv_cc = uns ? "be" : "le"; break;
    case LLIR_CMP_GE: cc = uns ? "ae" : "ge"; inv_cc = uns ? "b" : "l";   break;
    default: break;
    }
  }

  if (out_cc) *out_cc = cc;
  if (out_inv_cc) *out_inv_cc = inv_cc;
}

static void x86_64_emit_int_cmp(LLIRInsn *insn) {
  const char *cc = NULL;
  x86_emit_int_compare(insn, &cc, NULL);
  if (insn->dst && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS && !insn->dst->is_float) {
    const char *dreg = x86_64_gp_regs[insn->dst->phys_reg];
    println("  set%s %s", cc, x86_reg8(dreg));
    println("  movzbl %s, %s", x86_reg8(dreg), x86_reg32(dreg));
    return;
  }
  x86_64_emit_cmp_result(cc);
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_float_cmp(LLIRInsn *insn, const char *cc) {
  load_vreg(insn->src1, "%xmm0");
  load_vreg(insn->src2, "%xmm1");

  if (insn->src1->ty->kind == TY_FLOAT)
    println("  ucomiss %%xmm1, %%xmm0");
  else
    println("  ucomisd %%xmm1, %%xmm0");

  x86_64_emit_cmp_result(cc);
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_float_eq(LLIRInsn *insn, bool ne) {
  load_vreg(insn->src1, "%xmm0");
  load_vreg(insn->src2, "%xmm1");

  if (insn->src1->ty->kind == TY_FLOAT)
    println("  ucomiss %%xmm1, %%xmm0");
  else
    println("  ucomisd %%xmm1, %%xmm0");

  if (ne) {
    println("  setne %%al");
    println("  setp %%dl");
    println("  or %%dl, %%al");
  } else {
    println("  sete %%al");
    println("  setnp %%dl");
    println("  and %%dl, %%al");
  }

  println("  movzbl %%al, %%eax");
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_cmp(LLIRInsn *insn, const char *float_cc) {
  if (insn->src1 && insn->src1->ty && is_flonum(insn->src1->ty)) {
    if (insn->kind == LLIR_CMP_EQ)
      x86_64_emit_float_eq(insn, false);
    else if (insn->kind == LLIR_CMP_NE)
      x86_64_emit_float_eq(insn, true);
    else
      x86_64_emit_float_cmp(insn, float_cc);
    return;
  }

  x86_64_emit_int_cmp(insn);
}

static void x86_64_emit_unary(LLIRInsn *insn, const char *op) {
  load_vreg(insn->src1, "%rax");
  println("  %s %%rax", op);
  store_vreg("%rax", insn->dst);
}

static void x86_64_emit_cast(LLIRInsn *insn) {
  Type *from = insn->src1 ? insn->src1->ty : NULL;
  Type *to = insn->ty;

  if (!from || !to || (from->size == to->size && from->is_unsigned == to->is_unsigned && is_flonum(from) == is_flonum(to))) {
    if (to && is_flonum(to)) {
      load_vreg(insn->src1, "%xmm0");
      store_vreg("%xmm0", insn->dst);
    } else {
      if (insn->src1 && insn->dst && insn->src1 == insn->dst)
        return;
      if (insn->src1 && insn->dst &&
          insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS &&
          insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS &&
          !insn->src1->is_float && !insn->dst->is_float) {
        if (insn->src1->phys_reg != insn->dst->phys_reg)
          println("  movq %s, %s", x86_64_gp_regs[insn->src1->phys_reg], x86_64_gp_regs[insn->dst->phys_reg]);
        return;
      }
      load_vreg(insn->src1, "%rax");
      store_vreg("%rax", insn->dst);
    }
    return;
  }

  if (to->kind == TY_BOOL) {
    if (insn->dst && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS && !insn->dst->is_float) {
      const char *dreg = x86_64_gp_regs[insn->dst->phys_reg];
      const char *sreg = "%rax";
      if (insn->src1 && insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS && !insn->src1->is_float)
        sreg = x86_64_gp_regs[insn->src1->phys_reg];
      else
        load_vreg(insn->src1, "%rax");
      int sz = insn->src1 && insn->src1->ty ? insn->src1->ty->size : 8;
      println(sz <= 4 ? "  test %s, %s" : "  test %s, %s", (sz <= 4) ? x86_reg32(sreg) : sreg, (sz <= 4) ? x86_reg32(sreg) : sreg);
      println("  setne %s", x86_reg8(dreg));
      println("  movzbl %s, %s", x86_reg8(dreg), x86_reg32(dreg));
      return;
    }
    load_vreg(insn->src1, "%rax");
    println("  test %%rax, %%rax");
    x86_64_emit_cmp_result("ne");
    store_vreg("%rax", insn->dst);
    return;
  }

  if (is_flonum(from) && (is_integer(to) || to->kind == TY_PTR)) {
    load_vreg(insn->src1, "%xmm0");
    if (from->kind == TY_FLOAT)
      println("  cvttss2si %%xmm0, %%rax");
    else
      println("  cvttsd2si %%xmm0, %%rax");
    store_vreg("%rax", insn->dst);
    return;
  }

  if ((is_integer(from) || from->kind == TY_PTR) && is_flonum(to)) {
    load_vreg(insn->src1, "%rax");
    if (to->kind == TY_FLOAT)
      println("  cvtsi2ss %%rax, %%xmm0");
    else
      println("  cvtsi2sd %%rax, %%xmm0");
    store_vreg("%xmm0", insn->dst);
    return;
  }

  if (is_flonum(from) && is_flonum(to)) {
    load_vreg(insn->src1, "%xmm0");
    if (from->kind == TY_FLOAT && to->kind == TY_DOUBLE)
      println("  cvtss2sd %%xmm0, %%xmm0");
    else if (from->kind == TY_DOUBLE && to->kind == TY_FLOAT)
      println("  cvtsd2ss %%xmm0, %%xmm0");
    store_vreg("%xmm0", insn->dst);
    return;
  }

  const char *dst_reg = "%rax";
  bool direct_dst = false;
  if (insn->dst && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS && !insn->dst->is_float) {
    dst_reg = x86_64_gp_regs[insn->dst->phys_reg];
    direct_dst = true;
  }

  const char *src_reg = "%rax";
  if (insn->src1 && insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS && !insn->src1->is_float) {
    src_reg = x86_64_gp_regs[insn->src1->phys_reg];
  } else {
    load_vreg(insn->src1, "%rax");
    src_reg = "%rax";
  }

  switch (to->size) {
  case 1:
    if (to->is_unsigned)
      println("  movzbl %s, %s", x86_reg8(src_reg), x86_reg32(dst_reg));
    else
      println("  movsbl %s, %s", x86_reg8(src_reg), x86_reg32(dst_reg));
    break;
  case 2:
    if (to->is_unsigned)
      println("  movzwl %s, %s", x86_reg16(src_reg), x86_reg32(dst_reg));
    else
      println("  movswl %s, %s", x86_reg16(src_reg), x86_reg32(dst_reg));
    break;
  case 4:
    if (to->is_unsigned) {
      println("  movl %s, %s", x86_reg32(src_reg), x86_reg32(dst_reg));
    } else {
      if (src_reg != dst_reg)
        println("  movslq %s, %s", x86_reg32(src_reg), dst_reg);
      else
        println("  cltq");
    }
    break;
  default:
    if (src_reg != dst_reg)
      println("  movq %s, %s", src_reg, dst_reg);
    break;
  }

  if (!direct_dst)
    store_vreg("%rax", insn->dst);
  else if (!strcmp(dst_reg, "%rax"))
    cached_rax_vreg = insn->dst;
  else
    cached_rax_vreg = NULL;
}

static void x86_64_emit_load(LLIRInsn *insn) {
  X86Addr addr;
  x86_get_insn_addr(insn, insn->src1, &addr);

  Type *ty = insn->ty;
  int sz = ty ? ty->size : 8;

  const char *dst_reg = "%rax";
  bool direct_dst = false;
  if (insn->dst && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS && !insn->dst->is_float) {
    dst_reg = x86_64_gp_regs[insn->dst->phys_reg];
    direct_dst = true;
  }

  switch (sz) {
  case 1:
    if (ty && ty->is_unsigned)
      println("  movzbl %s, %s", addr.buf, x86_reg32(dst_reg));
    else
      println("  movsbl %s, %s", addr.buf, x86_reg32(dst_reg));
    break;

  case 2:
    if (ty && ty->is_unsigned)
      println("  movzwl %s, %s", addr.buf, x86_reg32(dst_reg));
    else
      println("  movswl %s, %s", addr.buf, x86_reg32(dst_reg));
    break;

  case 4:
    if (ty && ty->is_unsigned)
      println("  movl %s, %s", addr.buf, x86_reg32(dst_reg));
    else
      println("  movslq %s, %s", addr.buf, dst_reg);
    break;

  default:
    println("  movq %s, %s", addr.buf, dst_reg);
    break;
  }

  if (!direct_dst)
    store_vreg("%rax", insn->dst);
  else if (!strcmp(dst_reg, "%rax"))
    cached_rax_vreg = insn->dst;
  else
    cached_rax_vreg = NULL;
}

static void x86_64_emit_store(LLIRInsn *insn) {
  X86Addr addr;
  if (insn->ty && (insn->ty->kind == TY_STRUCT || insn->ty->kind == TY_UNION)) {
    int src2_offset = insn->src2 ? (insn->src2->spill_offset ? insn->src2->spill_offset : -((insn->src2->id + 1) * 8)) : 0;
    if (insn->ty->size <= 8) {
      load_vreg(insn->src2, "%rcx");
      x86_get_insn_addr(insn, insn->src1, &addr);
      switch (insn->ty->size) {
      case 1: println("  movb %%cl, %s", addr.buf); break;
      case 2: println("  movw %%cx, %s", addr.buf); break;
      case 4: println("  movl %%ecx, %s", addr.buf); break;
      default: println("  movq %%rcx, %s", addr.buf); break;
      }
      return;
    } else if (insn->ty->size <= 16) {
      x86_get_insn_addr(insn, insn->src1, &addr);
      println("  movq %d(%%rbp), %%rcx", src2_offset);
      println("  movq %%rcx, %s", addr.buf);
      if (!strcmp(addr.base_reg, "%rbp")) {
        println("  movq %d(%%rbp), %%rcx", src2_offset + 8);
        println("  movq %%rcx, %d(%%rbp)", addr.offset + 8);
      } else {
        println("  movq %d(%%rbp), %%rcx", src2_offset + 8);
        println("  movq %%rcx, %d(%%rax)", addr.offset + 8);
      }
      return;
    }
  }

  if (insn->is_imm_op || (insn->src2 && insn->src2->def_insn && insn->src2->def_insn->kind == LLIR_IMM &&
      (int32_t)insn->src2->def_insn->imm == insn->src2->def_insn->imm)) {
    int64_t imm = insn->is_imm_op ? insn->imm : insn->src2->def_insn->imm;
    x86_get_insn_addr(insn, insn->src1, &addr);
    switch (insn->ty ? insn->ty->size : 8) {
    case 1:
      println("  movb $%lld, %s", (long long)imm, addr.buf);
      return;
    case 2:
      println("  movw $%lld, %s", (long long)imm, addr.buf);
      return;
    case 4:
      println("  movl $%lld, %s", (long long)imm, addr.buf);
      return;
    default:
      println("  movq $%lld, %s", (long long)imm, addr.buf);
      return;
    }
  }

  if (insn->src2 && insn->src2->phys_reg >= 0 && insn->src2->phys_reg < NUM_X86_64_GP_REGS && !insn->src2->is_float) {
    const char *sreg = x86_64_gp_regs[insn->src2->phys_reg];
    x86_get_insn_addr(insn, insn->src1, &addr);
    switch (insn->ty ? insn->ty->size : 8) {
    case 1:
      println("  movb %s, %s", x86_reg8(sreg), addr.buf);
      return;
    case 2:
      println("  movw %s, %s", x86_reg16(sreg), addr.buf);
      return;
    case 4:
      println("  movl %s, %s", x86_reg32(sreg), addr.buf);
      return;
    default:
      println("  movq %s, %s", sreg, addr.buf);
      return;
    }
  }

  load_vreg(insn->src2, "%rdx");
  x86_get_insn_addr(insn, insn->src1, &addr);

  switch (insn->ty ? insn->ty->size : 8) {
  case 1:
    println("  movb %%dl, %s", addr.buf);
    break;
  case 2:
    println("  movw %%dx, %s", addr.buf);
    break;
  case 4:
    println("  movl %%edx, %s", addr.buf);
    break;
  default:
    println("  movq %%rdx, %s", addr.buf);
    break;
  }
}

static bool label_follows(LLIRInsn *insn, const char *label) {
  if (!insn || !label) return false;
  for (LLIRInsn *nxt = insn->next; nxt; nxt = nxt->next) {
    if (nxt->kind == LLIR_NOP)
      continue;
    if (nxt->kind == LLIR_LABEL)
      return nxt->label && !strcmp(nxt->label, label);
    break;
  }
  return false;
}

static void emit_branch(LLIRInsn *insn, const char *cc, const char *inv_cc) {
  bool has_t = insn->label_true && insn->label_true[0];
  bool has_f = insn->label_false && insn->label_false[0];
  if (has_t && has_f) {
    if (label_follows(insn, insn->label_false)) {
      println("  j%s %s", cc, insn->label_true);
    } else if (label_follows(insn, insn->label_true)) {
      println("  j%s %s", inv_cc, insn->label_false);
    } else {
      println("  j%s %s", cc, insn->label_true);
      println("  jmp %s", insn->label_false);
    }
  } else if (has_t) {
    if (!label_follows(insn, insn->label_true))
      println("  j%s %s", cc, insn->label_true);
  } else if (has_f) {
    if (!label_follows(insn, insn->label_false))
      println("  j%s %s", inv_cc, insn->label_false);
  }
}

static void x86_64_emit_atomic_cas(LLIRInsn *insn) {
  load_vreg(insn->src1, "%r10");
  load_vreg(insn->src2, "%r8");
  load_vreg(insn->src3, "%rdx");

  switch (insn->ty ? insn->ty->size : 8) {
  case 1:
    println("  movb (%%r8), %%al");
    println("  lock cmpxchgb %%dl, (%%r10)");
    println("  sete %%cl");
    println("  movb %%al, (%%r8)");
    break;
  case 2:
    println("  movw (%%r8), %%ax");
    println("  lock cmpxchgw %%dx, (%%r10)");
    println("  sete %%cl");
    println("  movw %%ax, (%%r8)");
    break;
  case 4:
    println("  movl (%%r8), %%eax");
    println("  lock cmpxchgl %%edx, (%%r10)");
    println("  sete %%cl");
    println("  movl %%eax, (%%r8)");
    break;
  default:
    println("  movq (%%r8), %%rax");
    println("  lock cmpxchgq %%rdx, (%%r10)");
    println("  sete %%cl");
    println("  movq %%rax, (%%r8)");
    break;
  }

  println("  movzbl %%cl, %%eax");
  if (insn->dst)
    store_vreg("%rax", insn->dst);
}

static void x86_64_emit_atomic_exch(LLIRInsn *insn) {
  load_vreg(insn->src1, "%r10");
  load_vreg(insn->src2, "%rax");

  switch (insn->ty ? insn->ty->size : 8) {
  case 1:
    println("  xchgb %%al, (%%r10)");
    break;
  case 2:
    println("  xchgw %%ax, (%%r10)");
    break;
  case 4:
    println("  xchgl %%eax, (%%r10)");
    break;
  default:
    println("  xchgq %%rax, (%%r10)");
    break;
  }

  if (insn->dst)
    store_vreg("%rax", insn->dst);
}

static void x86_64_emit_memcpy(LLIRInsn *insn) {
  invalidate_cached_regs();
  int64_t sz = insn->imm;
  if (sz == 1) {
    load_vreg(insn->src2, "%rax");
    load_vreg(insn->src1, "%rdx");
    println("  movb (%%rax), %%cl");
    println("  movb %%cl, (%%rdx)");
  } else if (sz == 2) {
    load_vreg(insn->src2, "%rax");
    load_vreg(insn->src1, "%rdx");
    println("  movw (%%rax), %%cx");
    println("  movw %%cx, (%%rdx)");
  } else if (sz == 4) {
    load_vreg(insn->src2, "%rax");
    load_vreg(insn->src1, "%rdx");
    println("  movl (%%rax), %%ecx");
    println("  movl %%ecx, (%%rdx)");
  } else if (sz == 8) {
    load_vreg(insn->src2, "%rax");
    load_vreg(insn->src1, "%rdx");
    println("  movq (%%rax), %%rcx");
    println("  movq %%rcx, (%%rdx)");
  } else if (sz == 16) {
    load_vreg(insn->src2, "%rax");
    load_vreg(insn->src1, "%rdx");
    println("  movq (%%rax), %%rcx");
    println("  movq %%rcx, (%%rdx)");
    println("  movq 8(%%rax), %%rcx");
    println("  movq %%rcx, 8(%%rdx)");
  } else {
    load_vreg(insn->src1, "%r8");
    load_vreg(insn->src2, "%r9");
    if ((uint64_t)insn->imm <= 0xFFFFFFFFULL)
      println("  mov $%u, %%ecx", (uint32_t)insn->imm);
    else
      println("  mov $%lld, %%rcx", (long long)insn->imm);
    println("1:");
    println("  test %%rcx, %%rcx");
    println("  je 2f");
    println("  movb (%%r9), %%al");
    println("  movb %%al, (%%r8)");
    println("  inc %%r8");
    println("  inc %%r9");
    println("  dec %%rcx");
    println("  jmp 1b");
    println("2:");
  }
}

static void x86_64_emit_memzero(LLIRInsn *insn) {
  invalidate_cached_regs();
  int64_t sz = insn->imm;
  if (sz == 1) {
    load_vreg(insn->src1, "%rax");
    println("  movb $0, (%%rax)");
  } else if (sz == 2) {
    load_vreg(insn->src1, "%rax");
    println("  movw $0, (%%rax)");
  } else if (sz == 4) {
    load_vreg(insn->src1, "%rax");
    println("  movl $0, (%%rax)");
  } else if (sz == 8) {
    load_vreg(insn->src1, "%rax");
    println("  movq $0, (%%rax)");
  } else if (sz == 16) {
    load_vreg(insn->src1, "%rax");
    println("  movq $0, (%%rax)");
    println("  movq $0, 8(%%rax)");
  } else {
    load_vreg(insn->src1, "%r8");
    if ((uint64_t)insn->imm <= 0xFFFFFFFFULL)
      println("  mov $%u, %%ecx", (uint32_t)insn->imm);
    else
      println("  mov $%lld, %%rcx", (long long)insn->imm);
    println("1:");
    println("  test %%rcx, %%rcx");
    println("  je 2f");
    println("  movb $0, (%%r8)");
    println("  inc %%r8");
    println("  dec %%rcx");
    println("  jmp 1b");
    println("2:");
  }
}

// ============================================================================
// Instruction Dispatcher
// ============================================================================
static bool str_case_eq(const char *s1, const char *s2) {
  if (!s1 || !s2) return false;
  while (*s1 && *s2) {
    if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2))
      return false;
    s1++;
    s2++;
  }
  return *s1 == *s2;
}

static void mark_callee_saved(const char *reg) {
  if (!current_fn) return;
  if (!strcmp(reg, "%rbx")) current_fn->callee_saved_mask |= (1 << 0);
  else if (!strcmp(reg, "%r12")) current_fn->callee_saved_mask |= (1 << 1);
  else if (!strcmp(reg, "%r13")) current_fn->callee_saved_mask |= (1 << 2);
  else if (!strcmp(reg, "%r14")) current_fn->callee_saved_mask |= (1 << 3);
  else if (!strcmp(reg, "%r15")) current_fn->callee_saved_mask |= (1 << 4);
  else if (!strcmp(reg, "%rdi")) current_fn->callee_saved_mask |= (1 << 5);
  else if (!strcmp(reg, "%rsi")) current_fn->callee_saved_mask |= (1 << 6);
}

static void x86_64_gen_asm(LLIRInsn *insn) {
  invalidate_cached_regs();

  if (!insn->asm_outputs && !insn->asm_inputs && !insn->asm_clobbers && !insn->asm_labels) {
    if (insn->asm_str && *insn->asm_str) {
      char *p = insn->asm_str;
      while (*p) {
        char *next = strchr(p, '\n');
        if (next) {
          *next = '\0';
          println("  %s", p);
          *next = '\n';
          p = next + 1;
        } else {
          println("  %s", p);
          break;
        }
      }
    }
    return;
  }

#define MAX_ASM_OPERANDS 64
  AsmOperand *ops[MAX_ASM_OPERANDS];
  int num_ops = 0;
  int num_outputs = 0;
  int num_inputs = 0;
  int num_labels = 0;

  for (AsmOperand *op = insn->asm_outputs; op && num_ops < MAX_ASM_OPERANDS; op = op->next) {
    ops[num_ops++] = op;
    num_outputs++;
  }
  for (AsmOperand *op = insn->asm_inputs; op && num_ops < MAX_ASM_OPERANDS; op = op->next) {
    ops[num_ops++] = op;
    num_inputs++;
  }
  for (AsmOperand *op = insn->asm_labels; op && num_ops < MAX_ASM_OPERANDS; op = op->next) {
    ops[num_ops++] = op;
    num_labels++;
  }

  static const char *asm_gp_regs[] = {
    "%rax", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11", "%rbx", "%r12", "%r13", "%r14", "%r15"
  };
#define NUM_ASM_GP 14
  bool reg_used[NUM_ASM_GP] = {0};
  const char *assigned_reg[MAX_ASM_OPERANDS] = {0};
  char assigned_mem[MAX_ASM_OPERANDS][128] = {{0}};

  // 1. Mark clobbered registers
  for (AsmClobber *c = insn->asm_clobbers; c; c = c->next) {
    char *clob = c->clobber;
    if (*clob == '%') clob++;
    for (int r = 0; r < NUM_ASM_GP; r++) {
      const char *rname = asm_gp_regs[r] + 1;
      const char *r32 = x86_reg32(asm_gp_regs[r]) + 1;
      const char *r16 = x86_reg16(asm_gp_regs[r]) + 1;
      const char *r8 = x86_reg8(asm_gp_regs[r]) + 1;
      if (str_case_eq(clob, rname) || str_case_eq(clob, r32) ||
          str_case_eq(clob, r16) || str_case_eq(clob, r8)) {
        reg_used[r] = true;
        mark_callee_saved(asm_gp_regs[r]);
        break;
      }
    }
  }

  // 2. Fixed register constraints
  for (int i = 0; i < num_outputs + num_inputs; i++) {
    char *c = ops[i]->constraint;
    if (!c) continue;
    if (strchr(c, 'a')) { assigned_reg[i] = "%rax"; }
    else if (strchr(c, 'b')) { assigned_reg[i] = "%rbx"; mark_callee_saved("%rbx"); }
    else if (strchr(c, 'c')) { assigned_reg[i] = "%rcx"; }
    else if (strchr(c, 'd')) { assigned_reg[i] = "%rdx"; }
    else if (strchr(c, 'S')) { assigned_reg[i] = "%rsi"; mark_callee_saved("%rsi"); }
    else if (strchr(c, 'D')) { assigned_reg[i] = "%rdi"; mark_callee_saved("%rdi"); }

    if (assigned_reg[i]) {
      for (int r = 0; r < NUM_ASM_GP; r++) {
        if (!strcmp(assigned_reg[i], asm_gp_regs[r]))
          reg_used[r] = true;
      }
    }
  }

  // 3. Matching constraints ("0".."9")
  for (int i = num_outputs; i < num_outputs + num_inputs; i++) {
    char *c = ops[i]->constraint;
    if (c) {
      for (char *p = c; *p; p++) {
        if (isdigit((unsigned char)*p)) {
          int match_idx = *p - '0';
          if (match_idx < num_outputs) {
            if (assigned_reg[match_idx]) {
              assigned_reg[i] = assigned_reg[match_idx];
            } else if (assigned_mem[match_idx][0]) {
              strcpy(assigned_mem[i], assigned_mem[match_idx]);
            }
          }
          break;
        }
      }
    }
  }

  // 4. Memory constraints ("m", "o", "v" without register alternatives)
  for (int i = 0; i < num_outputs + num_inputs; i++) {
    if (assigned_reg[i] || assigned_mem[i][0]) continue;
    char *c = ops[i]->constraint;
    bool is_pure_mem = c && (strchr(c, 'm') || strchr(c, 'o') || strchr(c, 'v')) &&
                       !strchr(c, 'r') && !strchr(c, 'g') && !strchr(c, 'q') && !strchr(c, 'Q') &&
                       !strchr(c, 'a') && !strchr(c, 'b') && !strchr(c, 'c') && !strchr(c, 'd') &&
                       !strchr(c, 'S') && !strchr(c, 'D');
    if (is_pure_mem) {
      if (ops[i]->var) {
        if (ops[i]->var->is_local)
          snprintf(assigned_mem[i], sizeof(assigned_mem[i]), "%d(%%rbp)", ops[i]->var->offset);
        else
          snprintf(assigned_mem[i], sizeof(assigned_mem[i]), "%s(%%rip)", ops[i]->var->name);
      } else if (ops[i]->addr_vreg) {
        const char *areg = "%rax";
        for (int r = 0; r < NUM_ASM_GP; r++) {
          if (!reg_used[r]) {
            areg = asm_gp_regs[r];
            reg_used[r] = true;
            mark_callee_saved(areg);
            break;
          }
        }
        load_vreg(ops[i]->addr_vreg, areg);
        snprintf(assigned_mem[i], sizeof(assigned_mem[i]), "(%s)", areg);
      }
    }
  }

  // 5. Immediate constraints
  for (int i = num_outputs; i < num_outputs + num_inputs; i++) {
    if (assigned_reg[i] || assigned_mem[i][0]) continue;
    char *c = ops[i]->constraint;
    if (c && (strchr(c, 'i') || strchr(c, 'n')) && ops[i]->is_imm_val) {
      snprintf(assigned_mem[i], sizeof(assigned_mem[i]), "$%lld", (long long)ops[i]->imm_val);
    }
  }

  // 6. General register constraints ("r", "g", "q", "Q")
  for (int i = 0; i < num_outputs; i++) {
    if (!assigned_reg[i] && !assigned_mem[i][0]) {
      for (int r = 0; r < NUM_ASM_GP; r++) {
        if (!reg_used[r]) {
          assigned_reg[i] = asm_gp_regs[r];
          reg_used[r] = true;
          mark_callee_saved(assigned_reg[i]);
          break;
        }
      }
    }
  }
  for (int i = num_outputs; i < num_outputs + num_inputs; i++) {
    if (!assigned_reg[i] && !assigned_mem[i][0]) {
      char *c = ops[i]->constraint;
      bool matched = false;
      if (c) {
        for (char *p = c; *p; p++) {
          if (isdigit((unsigned char)*p)) {
            int match_idx = *p - '0';
            if (match_idx < num_outputs && assigned_reg[match_idx]) {
              assigned_reg[i] = assigned_reg[match_idx];
              matched = true;
            }
            break;
          }
        }
      }
      if (!matched) {
        for (int r = 0; r < NUM_ASM_GP; r++) {
          if (!reg_used[r]) {
            assigned_reg[i] = asm_gp_regs[r];
            reg_used[r] = true;
            mark_callee_saved(assigned_reg[i]);
            break;
          }
        }
      }
    }
  }

  // Fallback
  for (int i = 0; i < num_outputs + num_inputs; i++) {
    if (!assigned_reg[i] && !assigned_mem[i][0]) {
      assigned_reg[i] = "%rax";
    }
  }

  // 7. Load inputs and read-write outputs into assigned registers
  for (int i = num_outputs; i < num_outputs + num_inputs; i++) {
    if (assigned_reg[i] && ops[i]->vreg) {
      load_vreg(ops[i]->vreg, assigned_reg[i]);
    }
  }
  for (int i = 0; i < num_outputs; i++) {
    if (assigned_reg[i] && ops[i]->vreg && ops[i]->constraint && strchr(ops[i]->constraint, '+')) {
      load_vreg(ops[i]->vreg, assigned_reg[i]);
    }
  }

  // 8. Format the template string
  char out_buf[8192];
  char *dst = out_buf;
  char *dst_end = out_buf + sizeof(out_buf) - 1;
  const char *src = insn->asm_str ? insn->asm_str : "";

  while (*src && dst < dst_end) {
    if (*src == '%') {
      src++;
      if (*src == '%') {
        *dst++ = '%';
        src++;
        continue;
      }

      char mod = 0;
      if (*src == 'b' || *src == 'h' || *src == 'w' || *src == 'k' ||
          *src == 'q' || *src == 'P' || *src == 'c' || *src == 'l' || *src == 'z') {
        mod = *src++;
      }

      int target_idx = -1;
      if (mod == 'l' && isdigit((unsigned char)*src)) {
        int idx = 0;
        while (isdigit((unsigned char)*src)) {
          idx = idx * 10 + (*src++ - '0');
        }
        if (idx < num_labels)
          target_idx = num_outputs + num_inputs + idx;
        else if (idx < num_ops)
          target_idx = idx;
      } else if (*src == '[') {
        src++;
        const char *name_start = src;
        while (*src && *src != ']') src++;
        int name_len = (int)(src - name_start);
        if (*src == ']') src++;

        for (int i = 0; i < num_ops; i++) {
          if (ops[i]->name && strlen(ops[i]->name) == name_len &&
              !strncmp(ops[i]->name, name_start, name_len)) {
            target_idx = i;
            break;
          }
        }
      } else if (isdigit((unsigned char)*src)) {
        target_idx = 0;
        while (isdigit((unsigned char)*src)) {
          target_idx = target_idx * 10 + (*src++ - '0');
        }
      }

      if (target_idx >= 0 && target_idx < num_ops) {
        AsmOperand *op = ops[target_idx];
        if (op->is_output == 2 || mod == 'l') {
          const char *lbl = op->unique_label ? op->unique_label : (op->label_name ? op->label_name : "");
          dst += snprintf(dst, dst_end - dst, "%s", lbl);
        } else if (mod == 'P' || mod == 'c') {
          if (op->is_imm_val)
            dst += snprintf(dst, dst_end - dst, "%lld", (long long)op->imm_val);
          else if (assigned_mem[target_idx][0])
            dst += snprintf(dst, dst_end - dst, "%s", assigned_mem[target_idx]);
          else if (assigned_reg[target_idx]) {
            const char *r = assigned_reg[target_idx];
            if (*r == '%') r++;
            dst += snprintf(dst, dst_end - dst, "%s", r);
          }
        } else if (mod == 'z') {
          int sz = (op->expr && op->expr->ty) ? op->expr->ty->size : 8;
          char sc = (sz == 1) ? 'b' : (sz == 2) ? 'w' : (sz == 4) ? 'l' : 'q';
          *dst++ = sc;
        } else if (assigned_mem[target_idx][0]) {
          dst += snprintf(dst, dst_end - dst, "%s", assigned_mem[target_idx]);
        } else if (assigned_reg[target_idx]) {
          const char *r64 = assigned_reg[target_idx];
          int sz = (op->expr && op->expr->ty) ? op->expr->ty->size : 8;
          if (mod == 'b') sz = 1;
          else if (mod == 'w') sz = 2;
          else if (mod == 'k') sz = 4;
          else if (mod == 'q') sz = 8;

          if (mod == 'h') {
            if (!strcmp(r64, "%rax")) dst += snprintf(dst, dst_end - dst, "%%ah");
            else if (!strcmp(r64, "%rbx")) dst += snprintf(dst, dst_end - dst, "%%bh");
            else if (!strcmp(r64, "%rcx")) dst += snprintf(dst, dst_end - dst, "%%ch");
            else if (!strcmp(r64, "%rdx")) dst += snprintf(dst, dst_end - dst, "%%dh");
            else dst += snprintf(dst, dst_end - dst, "%s", x86_reg8(r64));
          } else if (sz == 1) {
            dst += snprintf(dst, dst_end - dst, "%s", x86_reg8(r64));
          } else if (sz == 2) {
            dst += snprintf(dst, dst_end - dst, "%s", x86_reg16(r64));
          } else if (sz == 4) {
            dst += snprintf(dst, dst_end - dst, "%s", x86_reg32(r64));
          } else {
            dst += snprintf(dst, dst_end - dst, "%s", r64);
          }
        }
      } else {
        *dst++ = '%';
        if (mod) *dst++ = mod;
      }
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';

  // 9. Output formatted lines
  char *line = out_buf;
  while (*line) {
    char *nl = strchr(line, '\n');
    if (nl) {
      *nl = '\0';
      println("  %s", line);
      *nl = '\n';
      line = nl + 1;
    } else {
      println("  %s", line);
      break;
    }
  }

  // 10. Store outputs into destination vregs
  for (int i = 0; i < num_outputs; i++) {
    if (assigned_reg[i] && ops[i]->vreg) {
      store_vreg(assigned_reg[i], ops[i]->vreg);
    }
  }

  invalidate_cached_regs();
}

static void x86_64_gen_insn(LLIRInsn *insn, FILE *out) {
  if (!insn)
    return;

  (void)out;

  if (insn->dst && current_needs_mat && !current_needs_mat[insn->dst->id]) {
    switch (insn->kind) {
    case LLIR_IMM:
    case LLIR_FIMM:
    case LLIR_LEA:
    case LLIR_MOV:
    case LLIR_CAST:
    case LLIR_ADD:
    case LLIR_SUB:
    case LLIR_MUL:
    case LLIR_DIV:
    case LLIR_MOD:
    case LLIR_AND:
    case LLIR_OR:
    case LLIR_XOR:
    case LLIR_SHL:
    case LLIR_SHR:
    case LLIR_NEG:
    case LLIR_NOT:
    case LLIR_LOGNOT:
    case LLIR_CMP_EQ:
    case LLIR_CMP_NE:
    case LLIR_CMP_LT:
    case LLIR_CMP_LE:
    case LLIR_CMP_GT:
    case LLIR_CMP_GE:
      return;
    default:
      break;
    }
  }

  switch (insn->kind) {
  case LLIR_NOP:
  case LLIR_PHI:
    break;

  case LLIR_LABEL:
    invalidate_cached_regs();
    println("%s:", insn->label ? insn->label : "");
    break;

  case LLIR_JMP: {
    invalidate_cached_regs();
    if (!insn->label || !insn->label[0] || label_follows(insn, insn->label))
      break;
    println("  jmp %s", insn->label);
    break;
  }

  case LLIR_BR_COND: {
    LLIRInsn *def = insn->src1 ? insn->src1->def_insn : NULL;
    invalidate_cached_regs();

    if (def && (!def->src1 || !def->src1->ty || !is_flonum(def->src1->ty)) &&
        (def->kind == LLIR_CMP_EQ || def->kind == LLIR_CMP_NE ||
         def->kind == LLIR_CMP_LT || def->kind == LLIR_CMP_LE ||
         def->kind == LLIR_CMP_GT || def->kind == LLIR_CMP_GE)) {
      const char *cc = NULL;
      const char *inv_cc = NULL;
      x86_emit_int_compare(def, &cc, &inv_cc);
      emit_branch(insn, cc, inv_cc);
      break;
    }

    if (def && def->kind == LLIR_LOGNOT) {
      if (def->src1 && def->src1->phys_reg >= 0 && def->src1->phys_reg < NUM_X86_64_GP_REGS && !def->src1->is_float) {
        const char *r = x86_64_gp_regs[def->src1->phys_reg];
        int sz = def->src1->ty ? def->src1->ty->size : 8;
        println(sz <= 4 ? "  test %s, %s" : "  test %s, %s", (sz <= 4) ? x86_reg32(r) : r, (sz <= 4) ? x86_reg32(r) : r);
      } else {
        load_vreg(def->src1, "%rax");
        int sz = def->src1 && def->src1->ty ? def->src1->ty->size : 8;
        println(sz <= 4 ? "  test %%eax, %%eax" : "  test %%rax, %%rax");
      }
      emit_branch(insn, "e", "ne");
      break;
    }

    if (insn->src1 && insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS && !insn->src1->is_float) {
      const char *r = x86_64_gp_regs[insn->src1->phys_reg];
      int sz = insn->src1->ty ? insn->src1->ty->size : 8;
      println(sz <= 4 ? "  test %s, %s" : "  test %s, %s", (sz <= 4) ? x86_reg32(r) : r, (sz <= 4) ? x86_reg32(r) : r);
      emit_branch(insn, "ne", "e");
      break;
    }

    load_vreg(insn->src1, "%rax");
    int sz = insn->src1 && insn->src1->ty ? insn->src1->ty->size : 8;
    println(sz <= 4 ? "  test %%eax, %%eax" : "  test %%rax, %%rax");
    emit_branch(insn, "ne", "e");
    break;
  }

  case LLIR_IMM:
    x86_emit_load_imm(insn);
    break;

  case LLIR_FIMM:
    x86_emit_load_fimm(insn);
    break;

  case LLIR_LEA:
    if (insn->dst && !insn->dst->is_float && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS) {
      const char *dst = x86_64_gp_regs[insn->dst->phys_reg];
      if (insn->var) {
        if (insn->var->is_local) {
          int total_off = insn->var->offset + (int)insn->imm;
          println("  lea %d(%%rbp), %s", total_off, dst);
        } else {
          if (insn->imm == 0)
            println("  lea %s(%%rip), %s", insn->var->name, dst);
          else
            println("  lea %s%+d(%%rip), %s", insn->var->name, (int)insn->imm, dst);
        }
      } else if (insn->label) {
        if (insn->imm == 0)
          println("  lea %s(%%rip), %s", insn->label, dst);
        else
          println("  lea %s%+d(%%rip), %s", insn->label, (int)insn->imm, dst);
      }
      break;
    }
    if (insn->var) {
      if (insn->var->is_local) {
        int total_off = insn->var->offset + (int)insn->imm;
        println("  lea %d(%%rbp), %%rax", total_off);
      } else {
        if (insn->imm == 0)
          println("  lea %s(%%rip), %%rax", insn->var->name);
        else
          println("  lea %s%+d(%%rip), %%rax", insn->var->name, (int)insn->imm);
      }
    } else if (insn->label) {
      if (insn->imm == 0)
        println("  lea %s(%%rip), %%rax", insn->label);
      else
        println("  lea %s%+d(%%rip), %%rax", insn->label, (int)insn->imm);
    }
    store_vreg("%rax", insn->dst);
    break;

  case LLIR_MOV:
    if (insn->src1 && insn->dst && (insn->src1 == insn->dst ||
        (insn->src1->phys_reg >= 0 && insn->src1->phys_reg == insn->dst->phys_reg) ||
        (insn->src1->spill_offset && insn->src1->spill_offset == insn->dst->spill_offset)))
      break;
    if (insn->src1 && insn->dst &&
        insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS &&
        insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS &&
        !insn->src1->is_float && !insn->dst->is_float) {
      println("  movq %s, %s", x86_64_gp_regs[insn->src1->phys_reg], x86_64_gp_regs[insn->dst->phys_reg]);
      break;
    }
    if (insn->dst && insn->dst->is_float) {
      load_vreg(insn->src1, "%xmm0");
      store_vreg("%xmm0", insn->dst);
    } else {
      load_vreg(insn->src1, "%rax");
      store_vreg("%rax", insn->dst);
    }
    break;

  case LLIR_CAST:
    x86_64_emit_cast(insn);
    break;

  case LLIR_LOAD:
    x86_64_emit_load(insn);
    break;

  case LLIR_STORE:
    x86_64_emit_store(insn);
    break;

  case LLIR_ADD:
    x86_64_emit_binop_imm(insn, "add", "addss", "addsd", true);
    break;

  case LLIR_SUB:
    x86_64_emit_binop_imm(insn, "sub", "subss", "subsd", false);
    break;

  case LLIR_MUL:
    x86_64_emit_binop_imm(insn, "imul", "mulss", "mulsd", true);
    break;

  case LLIR_DIV:
    x86_64_emit_divmod(insn, false);
    break;

  case LLIR_MOD:
    x86_64_emit_divmod(insn, true);
    break;

  case LLIR_AND:
    x86_64_emit_binop_imm(insn, "and", NULL, NULL, true);
    break;

  case LLIR_OR:
    x86_64_emit_binop_imm(insn, "or", NULL, NULL, true);
    break;

  case LLIR_XOR:
    x86_64_emit_binop_imm(insn, "xor", NULL, NULL, true);
    break;

  case LLIR_SHL:
    x86_64_emit_shift(insn, true);
    break;

  case LLIR_SHR:
    x86_64_emit_shift(insn, false);
    break;

  case LLIR_NEG:
    x86_64_emit_unary(insn, "neg");
    break;

  case LLIR_NOT:
    x86_64_emit_unary(insn, "not");
    break;

  case LLIR_LOGNOT:
    if (insn->dst && insn->dst->phys_reg >= 0 && insn->dst->phys_reg < NUM_X86_64_GP_REGS && !insn->dst->is_float) {
      const char *dreg = x86_64_gp_regs[insn->dst->phys_reg];
      const char *sreg = "%rax";
      if (insn->src1 && insn->src1->phys_reg >= 0 && insn->src1->phys_reg < NUM_X86_64_GP_REGS && !insn->src1->is_float)
        sreg = x86_64_gp_regs[insn->src1->phys_reg];
      else
        load_vreg(insn->src1, "%rax");
      int sz = insn->src1 && insn->src1->ty ? insn->src1->ty->size : 8;
      println(sz <= 4 ? "  test %s, %s" : "  test %s, %s", (sz <= 4) ? x86_reg32(sreg) : sreg, (sz <= 4) ? x86_reg32(sreg) : sreg);
      println("  sete %s", x86_reg8(dreg));
      println("  movzbl %s, %s", x86_reg8(dreg), x86_reg32(dreg));
      break;
    }
    load_vreg(insn->src1, "%rax");
    println("  test %%rax, %%rax");
    x86_64_emit_cmp_result("e");
    store_vreg("%rax", insn->dst);
    break;

  case LLIR_CMP_EQ:
    x86_64_emit_cmp(insn, "e");
    break;

  case LLIR_CMP_NE:
    x86_64_emit_cmp(insn, "ne");
    break;

  case LLIR_CMP_LT:
    x86_64_emit_cmp(insn, "b");
    break;

  case LLIR_CMP_LE:
    x86_64_emit_cmp(insn, "be");
    break;

  case LLIR_CMP_GT:
    x86_64_emit_cmp(insn, "a");
    break;

  case LLIR_CMP_GE:
    x86_64_emit_cmp(insn, "ae");
    break;

  case LLIR_RET: {
    invalidate_cached_regs();
    if (insn->src1) {
      if (insn->ty && is_flonum(insn->ty)) {
        load_vreg(insn->src1, "%xmm0");
      } else {
        load_vreg(insn->src1, "%rax");
        if (insn->ty &&
            (insn->ty->kind == TY_STRUCT ||
             insn->ty->kind == TY_UNION)) {
          ABI *fn_abi =
              (current_fn && current_fn->abi) ? current_fn->abi : current_abi;

          if (fn_abi && fn_abi->emit_return)
            fn_abi->emit_return(current_fn, insn->ty, output_file);
        }
      }
    }

    LLIRInsn *nxt = insn->next;
    while (nxt && (nxt->kind == LLIR_NOP || nxt->kind == LLIR_LABEL))
      nxt = nxt->next;
    if (nxt)
      println("  jmp .L.return.%s", current_fn->name);
    break;
  }

  case LLIR_CALL: {
    invalidate_cached_regs();
    ABI *callee_abi = insn->call_abi ? insn->call_abi : ((current_fn && current_fn->abi) ? current_fn->abi : current_abi);
    if (callee_abi && callee_abi->emit_call) {
      callee_abi->emit_call(insn, output_file);
    }
    break;
  }

  case LLIR_ASM:
    x86_64_gen_asm(insn);
    break;

  case LLIR_ALLOCA:
    load_vreg(insn->src1, "%rax");
    println("  add $15, %%rax");
    println("  and $-16, %%rax");
    println("  sub %%rax, %%rsp");
    println("  mov %%rsp, %%rax");
    store_vreg("%rax", insn->dst);
    break;

  case LLIR_CAS:
    x86_64_emit_atomic_cas(insn);
    break;

  case LLIR_EXCH:
    x86_64_emit_atomic_exch(insn);
    break;

  case LLIR_MEMCPY:
    x86_64_emit_memcpy(insn);
    break;

  case LLIR_MEMZERO:
    x86_64_emit_memzero(insn);
    break;

  default:
    break;
  }
}

static void x86_64_gen_expr(LLIRInsn *insn, FILE *out) {
  x86_64_gen_insn(insn, out);
}

// ============================================================================
// Program Text Emission
// ============================================================================
static void emit_text(LLIRProg *prog, FILE *out) {
  (void)out;

  if (!prog || !prog->fns)
    return;

  for (int i = 0; i < prog->num_fns; i++) {
    LLIRFunction *fn = prog->fns[i];
    Obj *fn_obj = fn->fn_obj;
    if (!fn_obj || !fn_obj->is_function || !fn_obj->is_definition || !fn_obj->is_live)
      continue;

    if (current_objfmt && current_objfmt->emit_fn_decl)
      current_objfmt->emit_fn_decl(fn_obj, output_file);
    else if (!fn_obj->is_static)
      println("  .globl %s", fn->name);

    if (opt_ffunction_sections) {
      if (current_objfmt == &objfmt_coff)
        println("  .section .text$%s,\"xr\"", fn->name);
      else if (current_objfmt == &objfmt_macho)
        println("  .section __TEXT,__text,%s", fn->name);
      else
        println("  .section .text.%s,\"ax\",@progbits", fn->name);
    } else {
      println("  .text");
    }

    if (current_objfmt && current_objfmt->emit_fn_type)
      current_objfmt->emit_fn_type(fn_obj, output_file);

    println("%s:", fn->name);

    current_fn = fn_obj;

    ABI *fn_abi = fn->abi ? fn->abi : get_fn_abi(fn_obj);

    if (fn_abi && fn_abi->emit_prologue)
      fn_abi->emit_prologue(fn_obj, output_file);

    compute_materialization(fn);
    invalidate_cached_regs();

    // Assemble LLIR instructions directly from LLIRFunction
    for (LLIRInsn *insn = fn->head; insn; insn = insn->next) {
      x86_64_gen_insn(insn, output_file);
    }

    if (strcmp(fn->name, "main") == 0)
      println("  xor %%eax, %%eax");

    if (fn_abi && fn_abi->emit_epilogue)
      fn_abi->emit_epilogue(fn_obj, output_file);
  }
}

static void x86_64_init(FILE *out) {
  output_file = out;
  depth = 0;
}

static const int x86_64_callee_saved_gp_ids[] = { 0, 1, 2, 3, 4 };
static const int x86_64_scratch_gp_ids[] = { 5 };
static const RegAllocPool x86_64_reg_pool = {
  .num_gp_regs = 5,
  .gp_regs = x86_64_callee_saved_gp_ids,
  .num_scratch_gp_regs = 1,
  .scratch_gp_regs = x86_64_scratch_gp_ids,
  .num_fp_regs = 0,
  .fp_regs = NULL,
  .spill_base_offset = 0,
  .spill_align = 16,
};

static void x86_64_codegen(LLIRProg *prog, FILE *out) {
  output_file = out;
  depth = 0;

  if (opt_g) {
    File **files = get_input_files();

    for (int i = 0; files[i]; i++) {
      char *name = strdup(files[i]->name);

      for (char *p = name; *p; p++) {
        if (*p == '\\')
          *p = '/';
      }

      println("  .file %d \"%s\"", files[i]->file_no, name);
      free(name);
    }
  }

  // Reset local offsets before the ABI assigns them
  for (Obj *fn = prog->globals; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    for (Obj *v = fn->locals; v; v = v->next)
      v->offset = 0;

    for (Obj *v = fn->params; v; v = v->next)
      v->offset = 0;
  }

  // ABI-specific local/parameter layout
  for (Obj *fn = prog->globals; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition)
      continue;

    const ABI *fn_abi = get_fn_abi(fn);
    if (fn_abi && fn_abi->assign_lvar_offsets)
      fn_abi->assign_lvar_offsets(fn);
  }

  // Allocate stack spill offsets for all LLIR virtual registers
  for (int i = 0; i < prog->num_fns; i++) {
    LLIRFunction *fn = prog->fns[i];
    if (!fn)
      continue;

    Obj *fn_obj = fn->fn_obj;
    const ABI *fn_abi = fn->abi ? fn->abi : (fn_obj ? get_fn_abi(fn_obj) : NULL);
    RegAllocPool pool = x86_64_reg_pool;
    if (fn_obj && fn_abi && fn_abi->get_spill_base) {
      pool.spill_base_offset = fn_abi->get_spill_base(fn_obj);
    }
    regalloc_function((IRFunction *)fn, &pool);
  }

  emit_data(prog->globals, out);
  emit_text(prog, out);
}

static void x86_64_codegen_llir(LLIRProg *prog, FILE *out) {
  if (prog)
    x86_64_codegen(prog, out);
}

Codegen codegen_x86_64 = {
  .name = "x86_64",
  .description = "x86-64 code generator",
  .default_abi_name = "sysv64",
  .init = x86_64_init,
  .codegen_llir = x86_64_codegen_llir,
  .emit_data = emit_data,
  .emit_text = emit_text,
  .gen_insn = x86_64_gen_insn,
  .gen_expr = x86_64_gen_expr,
};
