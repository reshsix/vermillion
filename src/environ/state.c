/*
This file is part of vermillion.

Vermillion is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Vermillion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <general/types.h>
#include <general/mem.h>

#include <environ/state.h>

[[gnu::naked, gnu::return_twice]]
extern void *
state_save(state *st)
{
    (void)st;

    #if defined(CONFIG_ARCH_ARM)

    /* Call parameters: st -> r0 */

    /* Saving low callee-saved registers in st->gpr[0 -> 3] */
    asm volatile ("stmia r0!, {r4-r7}");
    /* Saving high callee-saved registers in st->gpr[4 -> 10] */
    asm volatile ("mov r1,  r8\n" "mov r2,  r9\n" "mov r3, r10\n" \
                  "mov r4, r11\n" "mov r5, r12\n" "mov r6, r13\n" \
                  "mov r7, r14\n");
    asm volatile ("stmia r0!, {r1-r7}");
    /* Restoring low registers */
    asm volatile ("sub r0, r0, #44");
    asm volatile ("ldmia r0!, {r4-r7}");
    /* Setting r1 to st->gpr[12] */
    asm volatile ("mov r1, r0");
    asm volatile ("add r1, r1, #32");
    /* Saving cpsr in st->gpr[12] */
    asm volatile ("mrs r2, cpsr");
    asm volatile ("str r2, [r1]");
    /* Setting r1 to st->gpr[11] */
    asm volatile ("sub r1, r1, #4");
    /* Setting r0 to NULL for first return */
    asm volatile ("mov r0, $0");
    /* Saving program counter in st->gpr[11] */
    asm volatile ("str pc, [r1]");
    /* Return with NULL or the value from state_load */
    asm volatile ("bx lr");
    /* Again if coming from state_load */
    asm volatile ("bx lr");

    #elif defined(CONFIG_ARCH_I686)

    /* Call parameters st -> [esp + 4] */

    /* Moving st to edx */
    asm volatile ("movl 4(%esp), %edx");
    /* Saving all callee-saved registers */
    asm volatile ("movl %ebx, (%edx)");
    asm volatile ("movl %esi, 4(%edx)");
    asm volatile ("movl %edi, 8(%edx)");
    asm volatile ("movl %ebp, 12(%edx)");
    asm volatile ("movl %esp, 16(%edx)");
    /* Saving return address */
    asm volatile ("movl (%esp), %eax");
    asm volatile ("movl %eax, 24(%edx)");
    /* Setting eax to NULL for first return */
    asm volatile ("xor %eax, %eax");
    /* Saving program counter */
    asm volatile ("movl $_ret, %ecx");
    asm volatile ("movl %ecx, 20(%edx)");
    asm volatile ("_ret:");
    /* Return with NULL or the value from state_load */
    asm volatile ("ret");

    #endif

    /* Supressing compiler warning */
    return NULL;
}

[[gnu::naked, noreturn]]
extern void
state_load(state *st, void *ret)
{
    (void)st, (void)ret;

    #if defined(CONFIG_ARCH_ARM)

    /* Call parameters: st -> r0, ret -> r1 */

    /* Loading cpsr */
    asm volatile ("add r0, r0, #48");
    asm volatile ("ldr r2, [r0]");
    asm volatile ("msr cpsr, r2");
    /* Loading high callee-saved registers */
    asm volatile ("sub r0, r0, #32");
    asm volatile ("ldmia r0!, {r2-r7}");
    asm volatile ("mov r8,  r2\n" "mov r9,  r3\n" "mov r10, r4\n" \
                  "mov r11, r5\n" "mov r12, r6\n" "mov r13, r7\n");
    asm volatile ("ldr r2, [r0]");
    asm volatile ("mov r14, r2");
    /* Loading low callee-saved registers */
    asm volatile ("sub r0, r0, #40");
    asm volatile ("ldmia r0!, {r4-r7}");
    /* Setting r2 to program counter */
    asm volatile ("add r0, r0, #28");
    asm volatile ("ldr r2, [r0]");
    /* Setting r0 to ret */
    asm volatile ("mov r0, r1");
    /* Jumping to address */
    asm volatile ("mov pc, r2");

    #elif defined(CONFIG_ARCH_I686)

    /* Call parameters st -> [esp + 4], ret -> [esp + 8] */

    /* Moving st to edx and ret to ecx */
    asm volatile ("movl 4(%esp), %edx");
    asm volatile ("movl 8(%esp), %ecx");
    /* Loading all callee-saved registers */
    asm volatile ("movl (%edx),   %ebx");
    asm volatile ("movl 4(%edx),  %esi");
    asm volatile ("movl 8(%edx),  %edi");
    asm volatile ("movl 12(%edx), %ebp");
    asm volatile ("movl 16(%edx), %esp");
    /* Setting return address */
    asm volatile ("movl 24(%edx), %eax");
    asm volatile ("movl %eax, (%esp)");
    /* Loading address to jump */
    asm volatile ("movl 20(%edx), %eax");
    asm volatile ("movl %eax, %edx");
    /* Setting eax to ret */
    asm volatile ("movl %ecx, %eax");
    /* Jumping to address */
    asm volatile ("jmp *%edx");

    #endif

    /* Supressing compiler warning */
    while (true);
}
