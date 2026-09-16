#include "test.h"

// Top-level assembly statement
asm(".globl toplevel_asm_fn\n"
    "toplevel_asm_fn:\n\t"
    "mov $77, %rax\n\t"
    "ret\n");

int toplevel_asm_fn(void);

int asm_fn1(void) {
  int res;
  asm("mov $50, %0" : "=r"(res));
  return res;
}

int asm_fn2(void) {
  int res;
  asm inline volatile("mov $55, %0" : "=r"(res));
  return res;
}

int asm_fn3(void) {
  int res;
  __asm__ __volatile__("mov {$}60, %0" : "=r"(res));
  return res;
}

unsigned char _unused_decl(const long *Base, long Offset);
extern __inline__ __attribute__((__gnu_inline__, __always_inline__))
unsigned char _unused_decl(const long *Base, long Offset) {
  __asm__ ("btq %[Offset],%[Base]\n\tsetc %[old]"
           : [old] "=qm" (Offset)
           : [Offset] "r" (Offset), [Base] "m" (*Base));
  return 0;
}

static inline void _unused_static_inline(int x) {
  __asm__ ("mov %[val], %%eax" : : [val] "r" (x));
}

static inline int _used_inline(int x) {
  return x + 10;
}

static int test_output_r(void) {
  int a = 0;
  asm("mov $123, %0" : "=r"(a));
  return a;
}

static int test_multi_outputs(void) {
  int a = 0, b = 0;
  asm("mov $10, %0\n\tmov $20, %1" : "=r"(a), "=r"(b));
  return a + b;
}

static int test_input_r(int x, int y) {
  int res = 0;
  asm("mov %1, %0\n\tadd %2, %0" : "=r"(res) : "r"(x), "r"(y));
  return res;
}

static int test_rw_reg(int x) {
  asm("add $100, %0" : "+r"(x));
  return x;
}

static int test_matching_constraint(int a, int b) {
  int c;
  asm("add %2, %0" : "=r"(c) : "0"(a), "r"(b));
  return c;
}

static int test_specific_regs(int x) {
  int a_out, c_out, d_out;
  asm("mov %3, %%eax\n\tmov %3, %%ecx\n\tmov %3, %%edx"
      : "=a"(a_out), "=c"(c_out), "=d"(d_out)
      : "r"(x));
  return a_out + c_out + d_out;
}

static int test_memory_operand(void) {
  int val = 40;
  asm("addl $10, %0" : "+m"(val));
  return val;
}

static int test_mem_read(int *ptr) {
  int out = 0;
  asm("movl %1, %0" : "=r"(out) : "m"(*ptr));
  return out;
}

static int test_imm(int x) {
  asm("add %1, %0" : "+r"(x) : "i"(50));
  return x;
}

static int test_imm_P(int x) {
  asm("add $%P1, %0" : "+r"(x) : "i"(30));
  return x;
}

static int test_modifiers(unsigned long long val) {
  unsigned char b = 0;
  unsigned short w = 0;
  unsigned int k = 0;
  unsigned long long q = 0;
  asm("mov %q1, %0" : "=r"(q) : "r"(val));
  asm("mov %k1, %k0" : "=r"(k) : "r"(val));
  asm("mov %w1, %w0" : "=r"(w) : "r"(val));
  asm("mov %b1, %b0" : "=r"(b) : "r"(val));
  return (q == val) && (k == (unsigned int)val) && (w == (unsigned short)val) && (b == (unsigned char)val);
}

static int test_high_byte(int val) {
  int res = 0;
  asm("mov %h1, %b0" : "=r"(res) : "a"(val));
  return res & 0xff;
}

static int test_named_operands(int a, int b) {
  int result;
  asm("mov %[first], %[out]\n\tsub %[second], %[out]"
      : [out] "=r"(result)
      : [first] "r"(a), [second] "r"(b));
  return result;
}

static int test_clobbers(int a, int b) {
  int res;
  asm volatile (
    "mov %1, %%eax\n\t"
    "mov %2, %%ecx\n\t"
    "add %%ecx, %%eax\n\t"
    "mov %%eax, %0"
    : "=r"(res)
    : "r"(a), "r"(b)
    : "rax", "rcx", "memory", "cc"
  );
  return res;
}

static int test_asm_goto(int cond) {
  asm goto ("cmp $0, %0\n\tjne %l0" : : "r"(cond) : : taken);
  return 100;
taken:
  return 200;
}

static int test_asm_goto_named(int cond) {
  asm goto ("cmp $0, %0\n\tje %l[zero_target]\n\tjmp %l[nonzero_target]"
            : : "r"(cond) : : [zero_target] is_zero, [nonzero_target] is_nonzero);
  return 0;
is_zero:
  return 10;
is_nonzero:
  return 20;
}

static int test_bt_setc(long long base, long long offset) {
  unsigned char old = 0;
  asm ("btq %[Offset],%[Base]\n\tsetc %[old]"
       : [old] "=qm" (old)
       : [Offset] "r" (offset), [Base] "m" (base)
       : "cc");
  return old;
}

int main() {
  ASSERT(77, toplevel_asm_fn());
  ASSERT(50, asm_fn1());
  ASSERT(55, asm_fn2());
  ASSERT(60, asm_fn3());
  ASSERT(15, _used_inline(5));

  ASSERT(123, test_output_r());
  ASSERT(30, test_multi_outputs());
  ASSERT(35, test_input_r(15, 20));
  ASSERT(125, test_rw_reg(25));
  ASSERT(50, test_matching_constraint(20, 30));
  ASSERT(15, test_specific_regs(5));
  ASSERT(50, test_memory_operand());

  int v = 99;
  ASSERT(99, test_mem_read(&v));

  ASSERT(70, test_imm(20));
  ASSERT(45, test_imm_P(15));
  ASSERT(1, test_modifiers(0x123456789abcdef0ULL));
  ASSERT(0x12, test_high_byte(0x1234));
  ASSERT(30, test_named_operands(50, 20));
  ASSERT(77, test_clobbers(33, 44));

  ASSERT(100, test_asm_goto(0));
  ASSERT(200, test_asm_goto(1));
  ASSERT(10, test_asm_goto_named(0));
  ASSERT(20, test_asm_goto_named(5));

  ASSERT(1, test_bt_setc(1LL << 5, 5));
  ASSERT(0, test_bt_setc(1LL << 5, 4));

  printf("OK\n");
  return 0;
}
