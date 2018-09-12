/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author: Stanislaw Skowronek
 */

//#include <linux/module.h>
//#include <linux/sched.h>
//#include <linux/slab.h>
//#include <asm/unaligned.h>

#define ATOM_DEBUG

//#include "atom.h"
//#include "atom-names.h"
//#include "atom-bits.h"
//#include "amdgpu.h"

#define ATOM_COND_ABOVE     0
#define ATOM_COND_ABOVEOREQUAL  1
#define ATOM_COND_ALWAYS    2
#define ATOM_COND_BELOW     3
#define ATOM_COND_BELOWOREQUAL  4
#define ATOM_COND_EQUAL     5
#define ATOM_COND_NOTEQUAL  6

#define ATOM_PORT_ATI   0
#define ATOM_PORT_PCI   1
#define ATOM_PORT_SYSIO 2

#define ATOM_UNIT_MICROSEC  0
#define ATOM_UNIT_MILLISEC  1

#define PLL_INDEX   2
#define PLL_DATA    3

#include <push.h>
#define uint8_t UINT8
#define uint16_t UINT16
#define uint32_t UINT32
#define uint64_t UINT64

#define bool BOOLEAN

typedef struct {
    struct atom_context *ctx;
    uint32_t *ps, *ws;
    int ps_shift;
    uint16_t start;
    unsigned last_jump;
    unsigned long last_jump_jiffies;
    bool abort;
} atom_exec_context;

int amdgpu_atom_debug = 0;
static int amdgpu_atom_execute_table_locked(struct atom_context *ctx, int index, uint32_t * params);
int amdgpu_atom_execute_table(struct atom_context *ctx, int index, uint32_t * params);

static uint32_t atom_arg_mask[8] =
    { 0xFFFFFFFF, 0xFFFF, 0xFFFF00, 0xFFFF0000, 0xFF, 0xFF00, 0xFF0000,
0xFF000000 };
static int atom_arg_shift[8] = { 0, 0, 8, 16, 0, 8, 16, 24 };

static int atom_dst_to_src[8][4] = {
    /* translate destination alignment field to the source alignment encoding */
    {0, 0, 0, 0},
    {1, 2, 3, 0},
    {1, 2, 3, 0},
    {1, 2, 3, 0},
    {4, 5, 6, 7},
    {4, 5, 6, 7},
    {4, 5, 6, 7},
    {4, 5, 6, 7},
};
static int atom_def_dst[8] = { 0, 0, 1, 2, 0, 1, 2, 3 };

static int debug_depth = 0;
#ifdef ATOM_DEBUG
static void debug_print_spaces(int n)
{
    while (n--)
        Log(L"   ");
}

//#define Log(L...) do if (amdgpu_atom_debug) { printk(KERN_DEBUG __VA_ARGS__); } while (0)
//#define Log(L...) do if (amdgpu_atom_debug) { printk(KERN_DEBUG); debug_print_spaces(debug_depth); printk(__VA_ARGS__); } while (0)
#else
//#define Log(L...) do { } while (0)
//#define Log(L...) do { } while (0)
#endif


#define ATOM_IIO_NOP        0
#define ATOM_IIO_START      1
#define ATOM_IIO_READ       2
#define ATOM_IIO_WRITE      3
#define ATOM_IIO_CLEAR      4
#define ATOM_IIO_SET        5
#define ATOM_IIO_MOVE_INDEX 6
#define ATOM_IIO_MOVE_ATTR  7
#define ATOM_IIO_MOVE_DATA  8
#define ATOM_IIO_END    9

uint8_t get_u8(void *bios, int ptr)
{
    return ((unsigned char *)bios)[ptr];
}
#define U8(ptr) get_u8(ctx->ctx->bios, (ptr))
#define CU8(ptr) get_u8(ctx->bios, (ptr))
uint16_t get_u16(void *bios, int ptr)
{
    return get_u8(bios, ptr) | (((uint16_t)get_u8(bios, ptr + 1)) << 8);
}
#define U16(ptr) get_u16(ctx->ctx->bios, (ptr))
#define CU16(ptr) get_u16(ctx->bios, (ptr))
uint32_t get_u32(void *bios, int ptr)
{
    return get_u16(bios, ptr) | (((uint32_t)get_u16(bios, ptr + 2)) << 16);
}
#define U32(ptr) get_u32(ctx->ctx->bios, (ptr))
#define CU32(ptr) get_u32(ctx->bios, (ptr))
#define CSTR(ptr) (((char *)(ctx->bios))+(ptr))
struct card_info {
    struct drm_device *dev;
    void(*reg_write)(struct card_info *, uint32_t, uint32_t);   /*  filled by driver */
    uint32_t(*reg_read)(struct card_info *, uint32_t);          /*  filled by driver */
    void(*ioreg_write)(struct card_info *, uint32_t, uint32_t);   /*  filled by driver */
    uint32_t(*ioreg_read)(struct card_info *, uint32_t);          /*  filled by driver */
    void(*mc_write)(struct card_info *, uint32_t, uint32_t);   /*  filled by driver */
    uint32_t(*mc_read)(struct card_info *, uint32_t);          /*  filled by driver */
    void(*pll_write)(struct card_info *, uint32_t, uint32_t);   /*  filled by driver */
    uint32_t(*pll_read)(struct card_info *, uint32_t);          /*  filled by driver */
};
struct atom_context {
        struct card_info *card;
    //  struct mutex mutex;
    uint8_t  *bios;
    uint32_t cmd_table, data_table;
    uint16_t *iio;

    uint16_t data_block;
    uint32_t fb_base;
    uint32_t divmul[2];
    uint16_t io_attr;
    uint16_t reg_block;
    uint8_t shift;
    int cs_equal, cs_above;
    int io_mode;
    uint32_t *scratch;
    int scratch_size_bytes;
};


static uint32_t atom_iio_execute(struct atom_context *ctx, int base,
                 uint32_t index, uint32_t data)
{
    uint32_t temp = 0xCDCDCDCD;

    while (1)
        switch (CU8(base)) {
        case ATOM_IIO_NOP:
            base++;
            break;
        case ATOM_IIO_READ:
            temp = ctx->card->ioreg_read(ctx->card, CU16(base + 1));
            base += 3;
            break;
        case ATOM_IIO_WRITE:
            ctx->card->ioreg_write(ctx->card, CU16(base + 1), temp);
            base += 3;
            break;
        case ATOM_IIO_CLEAR:
            temp &=
                ~((0xFFFFFFFF >> (32 - CU8(base + 1))) <<
                  CU8(base + 2));
            base += 3;
            break;
        case ATOM_IIO_SET:
            temp |=
                (0xFFFFFFFF >> (32 - CU8(base + 1))) << CU8(base +
                                    2);
            base += 3;
            break;
        case ATOM_IIO_MOVE_INDEX:
            temp &=
                ~((0xFFFFFFFF >> (32 - CU8(base + 1))) <<
                  CU8(base + 3));
            temp |=
                ((index >> CU8(base + 2)) &
                 (0xFFFFFFFF >> (32 - CU8(base + 1)))) << CU8(base +
                                      3);
            base += 4;
            break;
        case ATOM_IIO_MOVE_DATA:
            temp &=
                ~((0xFFFFFFFF >> (32 - CU8(base + 1))) <<
                  CU8(base + 3));
            temp |=
                ((data >> CU8(base + 2)) &
                 (0xFFFFFFFF >> (32 - CU8(base + 1)))) << CU8(base +
                                      3);
            base += 4;
            break;
        case ATOM_IIO_MOVE_ATTR:
            temp &=
                ~((0xFFFFFFFF >> (32 - CU8(base + 1))) <<
                  CU8(base + 3));
            temp |=
                ((ctx->
                  io_attr >> CU8(base + 2)) & (0xFFFFFFFF >> (32 -
                                      CU8
                                      (base
                                       +
                                       1))))
                << CU8(base + 3);
            base += 4;
            break;
        case ATOM_IIO_END:
            return temp;
        default:
            Log(L"Unknown IIO opcode\n");
            return 0;
        }
}
#define ATOM_ARG_REG        0
#define ATOM_ARG_PS     1
#define ATOM_ARG_WS     2
#define ATOM_ARG_FB     3
#define ATOM_ARG_ID     4
#define ATOM_ARG_IMM        5
#define ATOM_ARG_PLL        6
#define ATOM_ARG_MC 7

#define ATOM_IO_MM      0
#define ATOM_IO_PCI     1
#define ATOM_IO_SYSIO       2
#define ATOM_IO_IIO 0x80

#define ATOM_WS_QUOTIENT    0x40
#define ATOM_WS_REMAINDER   0x41
#define ATOM_WS_DATAPTR     0x42
#define ATOM_WS_SHIFT       0x43
#define ATOM_WS_OR_MASK     0x44
#define ATOM_WS_AND_MASK    0x45
#define ATOM_WS_FB_WINDOW   0x46
#define ATOM_WS_ATTRIBUTES  0x47
#define ATOM_WS_REGPTR 0x48

#define ATOM_SRC_DWORD      0
#define ATOM_SRC_WORD0      1
#define ATOM_SRC_WORD8      2
#define ATOM_SRC_WORD16     3
#define ATOM_SRC_BYTE0      4
#define ATOM_SRC_BYTE8      5
#define ATOM_SRC_BYTE16     6
#define ATOM_SRC_BYTE24 7


static uint32_t atom_get_src_int(atom_exec_context *ctx, uint8_t attr,
                 int *ptr, uint32_t *saved, int print)
{
    uint32_t idx, val = 0xCDCDCDCD, align, arg;
    struct atom_context *gctx = ctx->ctx;
    arg = attr & 7;
    align = (attr >> 3) & 7;
    switch (arg) {
    case ATOM_ARG_REG:
        idx = U16(*ptr);
        (*ptr) += 2;
        if (print)
            Log(L"REG[0x%04X]", idx);
        idx += gctx->reg_block;
        switch (gctx->io_mode) {
        case ATOM_IO_MM:
            val = gctx->card->reg_read(gctx->card, idx);
            break;
        case ATOM_IO_PCI:
            Log(L"PCI registers are not implemented\n");
            return 0;
        case ATOM_IO_SYSIO:
            Log(L"SYSIO registers are not implemented\n");
            return 0;
        default:
            if (!(gctx->io_mode & 0x80)) {
                Log(L"Bad IO mode\n");
                return 0;
            }
            if (!gctx->iio[gctx->io_mode & 0x7F]) {
                Log(L"Undefined indirect IO read method %d\n",
                    gctx->io_mode & 0x7F);
                return 0;
            }
            val =
                atom_iio_execute(gctx,
                         gctx->iio[gctx->io_mode & 0x7F],
                         idx, 0);
        }
        break;
    case ATOM_ARG_PS:
        idx = U8(*ptr);
        (*ptr)++;
        /* get_unaligned_le32 avoids unaligned accesses from atombios
         * tables, noticed on a DEC Alpha. */
        //val = get_unaligned_le32((u32 *)&ctx->ps[idx]);
        Log(L"ATOMFIX: Code needs fixing");
        if (print)
            Log(L"PS[0x%02X,0x%04X]", idx, val);
        break;
    case ATOM_ARG_WS:
        idx = U8(*ptr);
        (*ptr)++;
        if (print)
            Log(L"WS[0x%02X]", idx);
        switch (idx) {
        case ATOM_WS_QUOTIENT:
            val = gctx->divmul[0];
            break;
        case ATOM_WS_REMAINDER:
            val = gctx->divmul[1];
            break;
        case ATOM_WS_DATAPTR:
            val = gctx->data_block;
            break;
        case ATOM_WS_SHIFT:
            val = gctx->shift;
            break;
        case ATOM_WS_OR_MASK:
            val = 1 << gctx->shift;
            break;
        case ATOM_WS_AND_MASK:
            val = ~(1 << gctx->shift);
            break;
        case ATOM_WS_FB_WINDOW:
            val = gctx->fb_base;
            break;
        case ATOM_WS_ATTRIBUTES:
            val = gctx->io_attr;
            break;
        case ATOM_WS_REGPTR:
            val = gctx->reg_block;
            break;
        default:
            val = ctx->ws[idx];
        }
        break;
    case ATOM_ARG_ID:
        idx = U16(*ptr);
        (*ptr) += 2;
        if (print) {
            if (gctx->data_block)
                Log(L"ID[0x%04X+%04X]", idx, gctx->data_block);
            else
                Log(L"ID[0x%04X]", idx);
        }
        val = U32(idx + gctx->data_block);
        break;
    case ATOM_ARG_FB:
        idx = U8(*ptr);
        (*ptr)++;
        if ((gctx->fb_base + (idx * 4)) > gctx->scratch_size_bytes) {
            Log(L"ATOM: fb read beyond scratch region: %d vs. %d\n",
                  gctx->fb_base + (idx * 4), gctx->scratch_size_bytes);
            val = 0;
        } else
            val = gctx->scratch[(gctx->fb_base / 4) + idx];
        if (print)
            Log(L"FB[0x%02X]", idx);
        break;
    case ATOM_ARG_IMM:
        switch (align) {
        case ATOM_SRC_DWORD:
            val = U32(*ptr);
            (*ptr) += 4;
            if (print)
                Log(L"IMM 0x%08X\n", val);
            return val;
        case ATOM_SRC_WORD0:
        case ATOM_SRC_WORD8:
        case ATOM_SRC_WORD16:
            val = U16(*ptr);
            (*ptr) += 2;
            if (print)
                Log(L"IMM 0x%04X\n", val);
            return val;
        case ATOM_SRC_BYTE0:
        case ATOM_SRC_BYTE8:
        case ATOM_SRC_BYTE16:
        case ATOM_SRC_BYTE24:
            val = U8(*ptr);
            (*ptr)++;
            if (print)
                Log(L"IMM 0x%02X\n", val);
            return val;
        }
        return 0;
    case ATOM_ARG_PLL:
        idx = U8(*ptr);
        (*ptr)++;
        if (print)
            Log(L"PLL[0x%02X]", idx);
        val = gctx->card->pll_read(gctx->card, idx);
        break;
    case ATOM_ARG_MC:
        idx = U8(*ptr);
        (*ptr)++;
        if (print)
            Log(L"MC[0x%02X]", idx);
        val = gctx->card->mc_read(gctx->card, idx);
        break;
    }
    if (saved)
        *saved = val;
    val &= atom_arg_mask[align];
    val >>= atom_arg_shift[align];
    if (print)
        switch (align) {
        case ATOM_SRC_DWORD:
            Log(L".[31:0] -> 0x%08X\n", val);
            break;
        case ATOM_SRC_WORD0:
            Log(L".[15:0] -> 0x%04X\n", val);
            break;
        case ATOM_SRC_WORD8:
            Log(L".[23:8] -> 0x%04X\n", val);
            break;
        case ATOM_SRC_WORD16:
            Log(L".[31:16] -> 0x%04X\n", val);
            break;
        case ATOM_SRC_BYTE0:
            Log(L".[7:0] -> 0x%02X\n", val);
            break;
        case ATOM_SRC_BYTE8:
            Log(L".[15:8] -> 0x%02X\n", val);
            break;
        case ATOM_SRC_BYTE16:
            Log(L".[23:16] -> 0x%02X\n", val);
            break;
        case ATOM_SRC_BYTE24:
            Log(L".[31:24] -> 0x%02X\n", val);
            break;
        }
    return val;
}

static void atom_skip_src_int(atom_exec_context *ctx, uint8_t attr, int *ptr)
{
    uint32_t align = (attr >> 3) & 7, arg = attr & 7;
    switch (arg) {
    case ATOM_ARG_REG:
    case ATOM_ARG_ID:
        (*ptr) += 2;
        break;
    case ATOM_ARG_PLL:
    case ATOM_ARG_MC:
    case ATOM_ARG_PS:
    case ATOM_ARG_WS:
    case ATOM_ARG_FB:
        (*ptr)++;
        break;
    case ATOM_ARG_IMM:
        switch (align) {
        case ATOM_SRC_DWORD:
            (*ptr) += 4;
            return;
        case ATOM_SRC_WORD0:
        case ATOM_SRC_WORD8:
        case ATOM_SRC_WORD16:
            (*ptr) += 2;
            return;
        case ATOM_SRC_BYTE0:
        case ATOM_SRC_BYTE8:
        case ATOM_SRC_BYTE16:
        case ATOM_SRC_BYTE24:
            (*ptr)++;
            return;
        }
        return;
    }
}

static uint32_t atom_get_src(atom_exec_context *ctx, uint8_t attr, int *ptr)
{
    return atom_get_src_int(ctx, attr, ptr, NULL, 1);
}

static uint32_t atom_get_src_direct(atom_exec_context *ctx, uint8_t align, int *ptr)
{
    uint32_t val = 0xCDCDCDCD;

    switch (align) {
    case ATOM_SRC_DWORD:
        val = U32(*ptr);
        (*ptr) += 4;
        break;
    case ATOM_SRC_WORD0:
    case ATOM_SRC_WORD8:
    case ATOM_SRC_WORD16:
        val = U16(*ptr);
        (*ptr) += 2;
        break;
    case ATOM_SRC_BYTE0:
    case ATOM_SRC_BYTE8:
    case ATOM_SRC_BYTE16:
    case ATOM_SRC_BYTE24:
        val = U8(*ptr);
        (*ptr)++;
        break;
    }
    return val;
}

static uint32_t atom_get_dst(atom_exec_context *ctx, int arg, uint8_t attr,
                 int *ptr, uint32_t *saved, int print)
{
    return atom_get_src_int(ctx,
                arg | atom_dst_to_src[(attr >> 3) &
                              7][(attr >> 6) & 3] << 3,
                ptr, saved, print);
}

static void atom_skip_dst(atom_exec_context *ctx, int arg, uint8_t attr, int *ptr)
{
    atom_skip_src_int(ctx,
              arg | atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) &
                                 3] << 3, ptr);
}

static void atom_put_dst(atom_exec_context *ctx, int arg, uint8_t attr,
             int *ptr, uint32_t val, uint32_t saved)
{
    uint32_t align =
        atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3], old_val =
        val, idx;
    struct atom_context *gctx = ctx->ctx;
    old_val &= atom_arg_mask[align] >> atom_arg_shift[align];
    val <<= atom_arg_shift[align];
    val &= atom_arg_mask[align];
    saved &= ~atom_arg_mask[align];
    val |= saved;
    switch (arg) {
    case ATOM_ARG_REG:
        idx = U16(*ptr);
        (*ptr) += 2;
        Log(L"REG[0x%04X]", idx);
        idx += gctx->reg_block;
        switch (gctx->io_mode) {
        case ATOM_IO_MM:
            if (idx == 0)
                gctx->card->reg_write(gctx->card, idx,
                              val << 2);
            else
                gctx->card->reg_write(gctx->card, idx, val);
            break;
        case ATOM_IO_PCI:
            Log(L"PCI registers are not implemented\n");
            return;
        case ATOM_IO_SYSIO:
            Log(L"SYSIO registers are not implemented\n");
            return;
        default:
            if (!(gctx->io_mode & 0x80)) {
                Log(L"Bad IO mode\n");
                return;
            }
            if (!gctx->iio[gctx->io_mode & 0xFF]) {
                Log(L"Undefined indirect IO write method %d\n",
                    gctx->io_mode & 0x7F);
                return;
            }
            atom_iio_execute(gctx, gctx->iio[gctx->io_mode & 0xFF],
                     idx, val);
        }
        break;
    case ATOM_ARG_PS:
        idx = U8(*ptr);
        (*ptr)++;
        Log(L"PS[0x%02X]", idx);
        //ctx->ps[idx] = cpu_to_le32(val);
        Log(L"ATOMFIX: Code needs fixing");
        break;
    case ATOM_ARG_WS:
        idx = U8(*ptr);
        (*ptr)++;
        Log(L"WS[0x%02X]", idx);
        switch (idx) {
        case ATOM_WS_QUOTIENT:
            gctx->divmul[0] = val;
            break;
        case ATOM_WS_REMAINDER:
            gctx->divmul[1] = val;
            break;
        case ATOM_WS_DATAPTR:
            gctx->data_block = val;
            break;
        case ATOM_WS_SHIFT:
            gctx->shift = val;
            break;
        case ATOM_WS_OR_MASK:
        case ATOM_WS_AND_MASK:
            break;
        case ATOM_WS_FB_WINDOW:
            gctx->fb_base = val;
            break;
        case ATOM_WS_ATTRIBUTES:
            gctx->io_attr = val;
            break;
        case ATOM_WS_REGPTR:
            gctx->reg_block = val;
            break;
        default:
            ctx->ws[idx] = val;
        }
        break;
    case ATOM_ARG_FB:
        idx = U8(*ptr);
        (*ptr)++;
        if ((gctx->fb_base + (idx * 4)) > gctx->scratch_size_bytes) {
            Log(L"ATOM: fb write beyond scratch region: %d vs. %d\n",
                  gctx->fb_base + (idx * 4), gctx->scratch_size_bytes);
        } else
            gctx->scratch[(gctx->fb_base / 4) + idx] = val;
        Log(L"FB[0x%02X]", idx);
        break;
    case ATOM_ARG_PLL:
        idx = U8(*ptr);
        (*ptr)++;
        Log(L"PLL[0x%02X]", idx);
        gctx->card->pll_write(gctx->card, idx, val);
        break;
    case ATOM_ARG_MC:
        idx = U8(*ptr);
        (*ptr)++;
        Log(L"MC[0x%02X]", idx);
        gctx->card->mc_write(gctx->card, idx, val);
        return;
    }
    switch (align) {
    case ATOM_SRC_DWORD:
        Log(L".[31:0] <- 0x%08X\n", old_val);
        break;
    case ATOM_SRC_WORD0:
        Log(L".[15:0] <- 0x%04X\n", old_val);
        break;
    case ATOM_SRC_WORD8:
        Log(L".[23:8] <- 0x%04X\n", old_val);
        break;
    case ATOM_SRC_WORD16:
        Log(L".[31:16] <- 0x%04X\n", old_val);
        break;
    case ATOM_SRC_BYTE0:
        Log(L".[7:0] <- 0x%02X\n", old_val);
        break;
    case ATOM_SRC_BYTE8:
        Log(L".[15:8] <- 0x%02X\n", old_val);
        break;
    case ATOM_SRC_BYTE16:
        Log(L".[23:16] <- 0x%02X\n", old_val);
        break;
    case ATOM_SRC_BYTE24:
        Log(L".[31:24] <- 0x%02X\n", old_val);
        break;
    }
}

static void atom_op_add(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src, saved;
    int dptr = *ptr;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    Log(L"   src: ");
    src = atom_get_src(ctx, attr, ptr);
    dst += src;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_and(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src, saved;
    int dptr = *ptr;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    Log(L"   src: ");
    src = atom_get_src(ctx, attr, ptr);
    dst &= src;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_beep(atom_exec_context *ctx, int *ptr, int arg)
{
    Log(L"ATOM BIOS beeped!\n");
}
#define ATOM_TABLE_NAMES_CNT 74

static char *atom_table_names[ATOM_TABLE_NAMES_CNT] = {
    "ASIC_Init", "GetDisplaySurfaceSize", "ASIC_RegistersInit",
    "VRAM_BlockVenderDetection", "SetClocksRatio", "MemoryControllerInit",
    "GPIO_PinInit", "MemoryParamAdjust", "DVOEncoderControl",
    "GPIOPinControl", "SetEngineClock", "SetMemoryClock", "SetPixelClock",
    "DynamicClockGating", "ResetMemoryDLL", "ResetMemoryDevice",
    "MemoryPLLInit", "EnableMemorySelfRefresh", "AdjustMemoryController",
    "EnableASIC_StaticPwrMgt", "ASIC_StaticPwrMgtStatusChange",
    "DAC_LoadDetection", "TMDS2EncoderControl", "LCD1OutputControl",
    "DAC1EncoderControl", "DAC2EncoderControl", "DVOOutputControl",
    "CV1OutputControl", "SetCRTC_DPM_State", "TVEncoderControl",
    "TMDS1EncoderControl", "LVDSEncoderControl", "TV1OutputControl",
    "EnableScaler", "BlankCRTC", "EnableCRTC", "GetPixelClock",
    "EnableVGA_Render", "EnableVGA_Access", "SetCRTC_Timing",
    "SetCRTC_OverScan", "SetCRTC_Replication", "SelectCRTC_Source",
    "EnableGraphSurfaces", "UpdateCRTC_DoubleBufferRegisters",
    "LUT_AutoFill", "EnableHW_IconCursor", "GetMemoryClock",
    "GetEngineClock", "SetCRTC_UsingDTDTiming", "TVBootUpStdPinDetection",
    "DFP2OutputControl", "VRAM_BlockDetectionByStrap", "MemoryCleanUp",
    "ReadEDIDFromHWAssistedI2C", "WriteOneByteToHWAssistedI2C",
    "ReadHWAssistedI2CStatus", "SpeedFanControl", "PowerConnectorDetection",
    "MC_Synchronization", "ComputeMemoryEnginePLL", "MemoryRefreshConversion",
    "VRAM_GetCurrentInfoBlock", "DynamicMemorySettings", "MemoryTraining",
    "EnableLVDS_SS", "DFP1OutputControl", "SetVoltage", "CRT1OutputControl",
    "CRT2OutputControl", "SetupHWAssistedI2CStatus", "ClockSource",
    "MemoryDeviceInit", "EnableYUV",
};

#define true 1
static void atom_op_calltable(atom_exec_context *ctx, int *ptr, int arg)
{
    int idx = U8((*ptr)++);
    int r = 0;

    if (idx < ATOM_TABLE_NAMES_CNT)
        Log(L"   table: %d (%s)\n", idx, atom_table_names[idx]);
    else
        Log(L"   table: %d\n", idx);
    if (U16(ctx->ctx->cmd_table + 4 + 2 * idx))
        r = amdgpu_atom_execute_table_locked(ctx->ctx, idx, ctx->ps + ctx->ps_shift);
    if (r) {
        ctx->abort = true;
    }
}

static void atom_op_clear(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t saved;
    int dptr = *ptr;
    attr &= 0x38;
    attr |= atom_def_dst[attr >> 3] << 6;
    atom_get_dst(ctx, arg, attr, ptr, &saved, 0);
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, 0, saved);
}

static void atom_op_compare(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src;
    Log(L"   src1: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
    Log(L"   src2: ");
    src = atom_get_src(ctx, attr, ptr);
    ctx->ctx->cs_equal = (dst == src);
    ctx->ctx->cs_above = (dst > src);
    Log(L"   result: %s %s\n", ctx->ctx->cs_equal ? "EQ" : "NE",
           ctx->ctx->cs_above ? "GT" : "LE");
}

static void atom_op_delay(atom_exec_context *ctx, int *ptr, int arg)
{
    unsigned count = U8((*ptr)++);
    Log(L"   count: %d\n", count);
    if (arg == ATOM_UNIT_MICROSEC)
        //udelay(count);
        Log(L"ATOMFIX: Code needs fixing");
    else if (/*!drm_can_sleep()*/1)
        //mdelay(count);
        Log(L"ATOMFIX: Code needs fixing");
    else
        //msleep(count);
        Log(L"ATOMFIX: Code needs fixing");
}

static void atom_op_div(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src;
    Log(L"   src1: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
    Log(L"   src2: ");
    src = atom_get_src(ctx, attr, ptr);
    if (src != 0) {
        ctx->ctx->divmul[0] = dst / src;
        ctx->ctx->divmul[1] = dst % src;
    } else {
        ctx->ctx->divmul[0] = 0;
        ctx->ctx->divmul[1] = 0;
    }
}

static void atom_op_div32(atom_exec_context *ctx, int *ptr, int arg)
{
    uint64_t val64;
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src;
    Log(L"   src1: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
    Log(L"   src2: ");
    src = atom_get_src(ctx, attr, ptr);
    if (src != 0) {
        val64 = dst;
        val64 |= ((uint64_t)ctx->ctx->divmul[1]) << 32;
        //do_div(val64, src);
        Log(L"ATOMFIX: Code needs fixing");
        //ctx->ctx->divmul[0] = lower_32_bits(val64);
        //ctx->ctx->divmul[1] = upper_32_bits(val64);
    } else {
        ctx->ctx->divmul[0] = 0;
        ctx->ctx->divmul[1] = 0;
    }
}

static void atom_op_eot(atom_exec_context *ctx, int *ptr, int arg)
{
    /* functionally, a nop */
}

static void atom_op_jump(atom_exec_context *ctx, int *ptr, int arg)
{
    int execute = 0, target = U16(*ptr);
    //unsigned long cjiffies;

    (*ptr) += 2;
    switch (arg) {
    case ATOM_COND_ABOVE:
        execute = ctx->ctx->cs_above;
        break;
    case ATOM_COND_ABOVEOREQUAL:
        execute = ctx->ctx->cs_above || ctx->ctx->cs_equal;
        break;
    case ATOM_COND_ALWAYS:
        execute = 1;
        break;
    case ATOM_COND_BELOW:
        execute = !(ctx->ctx->cs_above || ctx->ctx->cs_equal);
        break;
    case ATOM_COND_BELOWOREQUAL:
        execute = !ctx->ctx->cs_above;
        break;
    case ATOM_COND_EQUAL:
        execute = ctx->ctx->cs_equal;
        break;
    case ATOM_COND_NOTEQUAL:
        execute = !ctx->ctx->cs_equal;
        break;
    }
    if (arg != ATOM_COND_ALWAYS)
        Log(L"   taken: %s\n", execute ? "yes" : "no");
    Log(L"   target: 0x%04X\n", target);
    if (execute) {
        if (ctx->last_jump == (ctx->start + target)) {
            //cjiffies = jiffies;
            Log(L"ATOMFIX: Code needs fixing");
            //if (time_after(cjiffies, ctx->last_jump_jiffies)) {
            //  cjiffies -= ctx->last_jump_jiffies;
            //  if ((jiffies_to_msecs(cjiffies) > 5000)) {
            //      Log(L"atombios stuck in loop for more than 5secs aborting\n");
            //      ctx->abort = true;
            //  }
            //} else {
            //  /* jiffies wrap around we will just wait a little longer */
            //  //ctx->last_jump_jiffies = jiffies;
            //  Log(L"ATOMFIX: Code needs fixing");
            //}
        } else {
            ctx->last_jump = ctx->start + target;
            //ctx->last_jump_jiffies = jiffies;
            Log(L"ATOMFIX: Code needs fixing");
        }
        *ptr = ctx->start + target;
    }
}

static void atom_op_mask(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, mask, src, saved;
    int dptr = *ptr;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    mask = atom_get_src_direct(ctx, ((attr >> 3) & 7), ptr);
    Log(L"   mask: 0x%08x", mask);
    Log(L"   src: ");
    src = atom_get_src(ctx, attr, ptr);
    dst &= mask;
    dst |= src;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_move(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t src, saved;
    int dptr = *ptr;
    if (((attr >> 3) & 7) != ATOM_SRC_DWORD)
        atom_get_dst(ctx, arg, attr, ptr, &saved, 0);
    else {
        atom_skip_dst(ctx, arg, attr, ptr);
        saved = 0xCDCDCDCD;
    }
    Log(L"   src: ");
    src = atom_get_src(ctx, attr, ptr);
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, src, saved);
}

static void atom_op_mul(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src;
    Log(L"   src1: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
    Log(L"   src2: ");
    src = atom_get_src(ctx, attr, ptr);
    ctx->ctx->divmul[0] = dst * src;
}

static void atom_op_mul32(atom_exec_context *ctx, int *ptr, int arg)
{
    uint64_t val64;
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src;
    Log(L"   src1: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
    Log(L"   src2: ");
    src = atom_get_src(ctx, attr, ptr);
    val64 = (uint64_t)dst * (uint64_t)src;
    //ctx->ctx->divmul[0] = lower_32_bits(val64);
    //ctx->ctx->divmul[1] = upper_32_bits(val64);
    Log(L"ATOMFIX: Code needs fixing");
}

static void atom_op_nop(atom_exec_context *ctx, int *ptr, int arg)
{
    /* nothing */
}

static void atom_op_or(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src, saved;
    int dptr = *ptr;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    Log(L"   src: ");
    src = atom_get_src(ctx, attr, ptr);
    dst |= src;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_postcard(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t val = U8((*ptr)++);
    Log(L"POST card output: 0x%02X\n", val);
}

static void atom_op_repeat(atom_exec_context *ctx, int *ptr, int arg)
{
    Log(L"unimplemented!\n");
}

static void atom_op_restorereg(atom_exec_context *ctx, int *ptr, int arg)
{
    Log(L"unimplemented!\n");
}

static void atom_op_savereg(atom_exec_context *ctx, int *ptr, int arg)
{
    Log(L"unimplemented!\n");
}

static void atom_op_setdatablock(atom_exec_context *ctx, int *ptr, int arg)
{
    int idx = U8(*ptr);
    (*ptr)++;
    Log(L"   block: %d\n", idx);
    if (!idx)
        ctx->ctx->data_block = 0;
    else if (idx == 255)
        ctx->ctx->data_block = ctx->start;
    else
        ctx->ctx->data_block = U16(ctx->ctx->data_table + 4 + 2 * idx);
    Log(L"   base: 0x%04X\n", ctx->ctx->data_block);
}

static void atom_op_setfbbase(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    Log(L"   fb_base: ");
    ctx->ctx->fb_base = atom_get_src(ctx, attr, ptr);
}
#define ATOM_IO_NAMES_CNT 5
static char *atom_io_names[ATOM_IO_NAMES_CNT] = {
    "MM", "PLL", "MC", "PCIE", "PCIE PORT",
};


static void atom_op_setport(atom_exec_context *ctx, int *ptr, int arg)
{
    int port;
    switch (arg) {
    case ATOM_PORT_ATI:
        port = U16(*ptr);
        if (port < ATOM_IO_NAMES_CNT)
            Log(L"   port: %d (%s)\n", port, atom_io_names[port]);
        else
            Log(L"   port: %d\n", port);
        if (!port)
            ctx->ctx->io_mode = ATOM_IO_MM;
        else
            ctx->ctx->io_mode = ATOM_IO_IIO | port;
        (*ptr) += 2;
        break;
    case ATOM_PORT_PCI:
        ctx->ctx->io_mode = ATOM_IO_PCI;
        (*ptr)++;
        break;
    case ATOM_PORT_SYSIO:
        ctx->ctx->io_mode = ATOM_IO_SYSIO;
        (*ptr)++;
        break;
    }
}

static void atom_op_setregblock(atom_exec_context *ctx, int *ptr, int arg)
{
    ctx->ctx->reg_block = U16(*ptr);
    (*ptr) += 2;
    Log(L"   base: 0x%04X\n", ctx->ctx->reg_block);
}

static void atom_op_shift_left(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++), shift;
    uint32_t saved, dst;
    int dptr = *ptr;
    attr &= 0x38;
    attr |= atom_def_dst[attr >> 3] << 6;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    shift = atom_get_src_direct(ctx, ATOM_SRC_BYTE0, ptr);
    Log(L"   shift: %d\n", shift);
    dst <<= shift;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_shift_right(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++), shift;
    uint32_t saved, dst;
    int dptr = *ptr;
    attr &= 0x38;
    attr |= atom_def_dst[attr >> 3] << 6;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    shift = atom_get_src_direct(ctx, ATOM_SRC_BYTE0, ptr);
    Log(L"   shift: %d\n", shift);
    dst >>= shift;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_shl(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++), shift;
    uint32_t saved, dst;
    int dptr = *ptr;
    uint32_t dst_align = atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3];
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    /* op needs to full dst value */
    dst = saved;
    shift = atom_get_src(ctx, attr, ptr);
    Log(L"   shift: %d\n", shift);
    dst <<= shift;
    dst &= atom_arg_mask[dst_align];
    dst >>= atom_arg_shift[dst_align];
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_shr(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++), shift;
    uint32_t saved, dst;
    int dptr = *ptr;
    uint32_t dst_align = atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3];
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    /* op needs to full dst value */
    dst = saved;
    shift = atom_get_src(ctx, attr, ptr);
    Log(L"   shift: %d\n", shift);
    dst >>= shift;
    dst &= atom_arg_mask[dst_align];
    dst >>= atom_arg_shift[dst_align];
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_sub(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src, saved;
    int dptr = *ptr;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    Log(L"   src: ");
    src = atom_get_src(ctx, attr, ptr);
    dst -= src;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}
#define ATOM_CASE_MAGIC     0x63
#define ATOM_CASE_END   0x5A5A


static void atom_op_switch(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t src, val, target;
    Log(L"   switch: ");
    src = atom_get_src(ctx, attr, ptr);
    while (U16(*ptr) != ATOM_CASE_END)
        if (U8(*ptr) == ATOM_CASE_MAGIC) {
            (*ptr)++;
            Log(L"   case: ");
            val =
                atom_get_src(ctx, (attr & 0x38) | ATOM_ARG_IMM,
                     ptr);
            target = U16(*ptr);
            if (val == src) {
                Log(L"   target: %04X\n", target);
                *ptr = ctx->start + target;
                return;
            }
            (*ptr) += 2;
        } else {
            Log(L"Bad case\n");
            return;
        }
    (*ptr) += 2;
}

static void atom_op_test(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src;
    Log(L"   src1: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
    Log(L"   src2: ");
    src = atom_get_src(ctx, attr, ptr);
    ctx->ctx->cs_equal = ((dst & src) == 0);
    Log(L"   result: %s\n", ctx->ctx->cs_equal ? "EQ" : "NE");
}

static void atom_op_xor(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t attr = U8((*ptr)++);
    uint32_t dst, src, saved;
    int dptr = *ptr;
    Log(L"   dst: ");
    dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
    Log(L"   src: ");
    src = atom_get_src(ctx, attr, ptr);
    dst ^= src;
    Log(L"   dst: ");
    atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}

static void atom_op_debug(atom_exec_context *ctx, int *ptr, int arg)
{
    uint8_t val = U8((*ptr)++);
    Log(L"DEBUG output: 0x%02X\n", val);
}

static void atom_op_processds(atom_exec_context *ctx, int *ptr, int arg)
{
    uint16_t val = U16(*ptr);
    (*ptr) += val + 2;
    Log(L"PROCESSDS output: 0x%02X\n", val);
}
#define ATOM_OP_CNT     123
#define ATOM_OP_EOT 91


static struct {
    void (*func) (atom_exec_context *, int *, int);
    int arg;
} opcode_table[ATOM_OP_CNT] = {
    {
    NULL, 0}, {
    atom_op_move, ATOM_ARG_REG}, {
    atom_op_move, ATOM_ARG_PS}, {
    atom_op_move, ATOM_ARG_WS}, {
    atom_op_move, ATOM_ARG_FB}, {
    atom_op_move, ATOM_ARG_PLL}, {
    atom_op_move, ATOM_ARG_MC}, {
    atom_op_and, ATOM_ARG_REG}, {
    atom_op_and, ATOM_ARG_PS}, {
    atom_op_and, ATOM_ARG_WS}, {
    atom_op_and, ATOM_ARG_FB}, {
    atom_op_and, ATOM_ARG_PLL}, {
    atom_op_and, ATOM_ARG_MC}, {
    atom_op_or, ATOM_ARG_REG}, {
    atom_op_or, ATOM_ARG_PS}, {
    atom_op_or, ATOM_ARG_WS}, {
    atom_op_or, ATOM_ARG_FB}, {
    atom_op_or, ATOM_ARG_PLL}, {
    atom_op_or, ATOM_ARG_MC}, {
    atom_op_shift_left, ATOM_ARG_REG}, {
    atom_op_shift_left, ATOM_ARG_PS}, {
    atom_op_shift_left, ATOM_ARG_WS}, {
    atom_op_shift_left, ATOM_ARG_FB}, {
    atom_op_shift_left, ATOM_ARG_PLL}, {
    atom_op_shift_left, ATOM_ARG_MC}, {
    atom_op_shift_right, ATOM_ARG_REG}, {
    atom_op_shift_right, ATOM_ARG_PS}, {
    atom_op_shift_right, ATOM_ARG_WS}, {
    atom_op_shift_right, ATOM_ARG_FB}, {
    atom_op_shift_right, ATOM_ARG_PLL}, {
    atom_op_shift_right, ATOM_ARG_MC}, {
    atom_op_mul, ATOM_ARG_REG}, {
    atom_op_mul, ATOM_ARG_PS}, {
    atom_op_mul, ATOM_ARG_WS}, {
    atom_op_mul, ATOM_ARG_FB}, {
    atom_op_mul, ATOM_ARG_PLL}, {
    atom_op_mul, ATOM_ARG_MC}, {
    atom_op_div, ATOM_ARG_REG}, {
    atom_op_div, ATOM_ARG_PS}, {
    atom_op_div, ATOM_ARG_WS}, {
    atom_op_div, ATOM_ARG_FB}, {
    atom_op_div, ATOM_ARG_PLL}, {
    atom_op_div, ATOM_ARG_MC}, {
    atom_op_add, ATOM_ARG_REG}, {
    atom_op_add, ATOM_ARG_PS}, {
    atom_op_add, ATOM_ARG_WS}, {
    atom_op_add, ATOM_ARG_FB}, {
    atom_op_add, ATOM_ARG_PLL}, {
    atom_op_add, ATOM_ARG_MC}, {
    atom_op_sub, ATOM_ARG_REG}, {
    atom_op_sub, ATOM_ARG_PS}, {
    atom_op_sub, ATOM_ARG_WS}, {
    atom_op_sub, ATOM_ARG_FB}, {
    atom_op_sub, ATOM_ARG_PLL}, {
    atom_op_sub, ATOM_ARG_MC}, {
    atom_op_setport, ATOM_PORT_ATI}, {
    atom_op_setport, ATOM_PORT_PCI}, {
    atom_op_setport, ATOM_PORT_SYSIO}, {
    atom_op_setregblock, 0}, {
    atom_op_setfbbase, 0}, {
    atom_op_compare, ATOM_ARG_REG}, {
    atom_op_compare, ATOM_ARG_PS}, {
    atom_op_compare, ATOM_ARG_WS}, {
    atom_op_compare, ATOM_ARG_FB}, {
    atom_op_compare, ATOM_ARG_PLL}, {
    atom_op_compare, ATOM_ARG_MC}, {
    atom_op_switch, 0}, {
    atom_op_jump, ATOM_COND_ALWAYS}, {
    atom_op_jump, ATOM_COND_EQUAL}, {
    atom_op_jump, ATOM_COND_BELOW}, {
    atom_op_jump, ATOM_COND_ABOVE}, {
    atom_op_jump, ATOM_COND_BELOWOREQUAL}, {
    atom_op_jump, ATOM_COND_ABOVEOREQUAL}, {
    atom_op_jump, ATOM_COND_NOTEQUAL}, {
    atom_op_test, ATOM_ARG_REG}, {
    atom_op_test, ATOM_ARG_PS}, {
    atom_op_test, ATOM_ARG_WS}, {
    atom_op_test, ATOM_ARG_FB}, {
    atom_op_test, ATOM_ARG_PLL}, {
    atom_op_test, ATOM_ARG_MC}, {
    atom_op_delay, ATOM_UNIT_MILLISEC}, {
    atom_op_delay, ATOM_UNIT_MICROSEC}, {
    atom_op_calltable, 0}, {
    atom_op_repeat, 0}, {
    atom_op_clear, ATOM_ARG_REG}, {
    atom_op_clear, ATOM_ARG_PS}, {
    atom_op_clear, ATOM_ARG_WS}, {
    atom_op_clear, ATOM_ARG_FB}, {
    atom_op_clear, ATOM_ARG_PLL}, {
    atom_op_clear, ATOM_ARG_MC}, {
    atom_op_nop, 0}, {
    atom_op_eot, 0}, {
    atom_op_mask, ATOM_ARG_REG}, {
    atom_op_mask, ATOM_ARG_PS}, {
    atom_op_mask, ATOM_ARG_WS}, {
    atom_op_mask, ATOM_ARG_FB}, {
    atom_op_mask, ATOM_ARG_PLL}, {
    atom_op_mask, ATOM_ARG_MC}, {
    atom_op_postcard, 0}, {
    atom_op_beep, 0}, {
    atom_op_savereg, 0}, {
    atom_op_restorereg, 0}, {
    atom_op_setdatablock, 0}, {
    atom_op_xor, ATOM_ARG_REG}, {
    atom_op_xor, ATOM_ARG_PS}, {
    atom_op_xor, ATOM_ARG_WS}, {
    atom_op_xor, ATOM_ARG_FB}, {
    atom_op_xor, ATOM_ARG_PLL}, {
    atom_op_xor, ATOM_ARG_MC}, {
    atom_op_shl, ATOM_ARG_REG}, {
    atom_op_shl, ATOM_ARG_PS}, {
    atom_op_shl, ATOM_ARG_WS}, {
    atom_op_shl, ATOM_ARG_FB}, {
    atom_op_shl, ATOM_ARG_PLL}, {
    atom_op_shl, ATOM_ARG_MC}, {
    atom_op_shr, ATOM_ARG_REG}, {
    atom_op_shr, ATOM_ARG_PS}, {
    atom_op_shr, ATOM_ARG_WS}, {
    atom_op_shr, ATOM_ARG_FB}, {
    atom_op_shr, ATOM_ARG_PLL}, {
    atom_op_shr, ATOM_ARG_MC}, {
    atom_op_debug, 0}, {
    atom_op_processds, 0}, /*{
    atom_op_mul32, ATOM_ARG_PS}, {
    atom_op_mul32, ATOM_ARG_WS}, {
    atom_op_div32, ATOM_ARG_PS}, {
    atom_op_div32, ATOM_ARG_WS},*/
};
#define EINVAL 1
#define ATOM_CT_SIZE_PTR    0
#define ATOM_CT_WS_PTR      4
#define ATOM_CT_PS_PTR      5
#define ATOM_CT_PS_MASK     0x7F
#define ATOM_CT_CODE_PTR    6

#define false 0
#define ATOM_OP_NAMES_CNT 123
static char *atom_op_names[ATOM_OP_NAMES_CNT] = {
    "RESERVED", "MOVE_REG", "MOVE_PS", "MOVE_WS", "MOVE_FB", "MOVE_PLL",
    "MOVE_MC", "AND_REG", "AND_PS", "AND_WS", "AND_FB", "AND_PLL", "AND_MC",
    "OR_REG", "OR_PS", "OR_WS", "OR_FB", "OR_PLL", "OR_MC", "SHIFT_LEFT_REG",
    "SHIFT_LEFT_PS", "SHIFT_LEFT_WS", "SHIFT_LEFT_FB", "SHIFT_LEFT_PLL",
    "SHIFT_LEFT_MC", "SHIFT_RIGHT_REG", "SHIFT_RIGHT_PS", "SHIFT_RIGHT_WS",
    "SHIFT_RIGHT_FB", "SHIFT_RIGHT_PLL", "SHIFT_RIGHT_MC", "MUL_REG",
    "MUL_PS", "MUL_WS", "MUL_FB", "MUL_PLL", "MUL_MC", "DIV_REG", "DIV_PS",
    "DIV_WS", "DIV_FB", "DIV_PLL", "DIV_MC", "ADD_REG", "ADD_PS", "ADD_WS",
    "ADD_FB", "ADD_PLL", "ADD_MC", "SUB_REG", "SUB_PS", "SUB_WS", "SUB_FB",
    "SUB_PLL", "SUB_MC", "SET_ATI_PORT", "SET_PCI_PORT", "SET_SYS_IO_PORT",
    "SET_REG_BLOCK", "SET_FB_BASE", "COMPARE_REG", "COMPARE_PS",
    "COMPARE_WS", "COMPARE_FB", "COMPARE_PLL", "COMPARE_MC", "SWITCH",
    "JUMP", "JUMP_EQUAL", "JUMP_BELOW", "JUMP_ABOVE", "JUMP_BELOW_OR_EQUAL",
    "JUMP_ABOVE_OR_EQUAL", "JUMP_NOT_EQUAL", "TEST_REG", "TEST_PS", "TEST_WS",
    "TEST_FB", "TEST_PLL", "TEST_MC", "DELAY_MILLISEC", "DELAY_MICROSEC",
    "CALL_TABLE", "REPEAT", "CLEAR_REG", "CLEAR_PS", "CLEAR_WS", "CLEAR_FB",
    "CLEAR_PLL", "CLEAR_MC", "NOP", "EOT", "MASK_REG", "MASK_PS", "MASK_WS",
    "MASK_FB", "MASK_PLL", "MASK_MC", "POST_CARD", "BEEP", "SAVE_REG",
    "RESTORE_REG", "SET_DATA_BLOCK", "XOR_REG", "XOR_PS", "XOR_WS", "XOR_FB",
    "XOR_PLL", "XOR_MC", "SHL_REG", "SHL_PS", "SHL_WS", "SHL_FB", "SHL_PLL",
    "SHL_MC", "SHR_REG", "SHR_PS", "SHR_WS", "SHR_FB", "SHR_PLL", "SHR_MC",
    "DEBUG", "CTB_DS",
};

static int amdgpu_atom_execute_table_locked(struct atom_context *ctx, int index, uint32_t * params)
{
    int base = CU16(ctx->cmd_table + 4 + 2 * index);
    int len, ws, ps, ptr;
    unsigned char op;
    atom_exec_context ectx;
    int ret = 0;

    if (!base)
        return -EINVAL;

    len = CU16(base + ATOM_CT_SIZE_PTR);
    ws = CU8(base + ATOM_CT_WS_PTR);
    ps = CU8(base + ATOM_CT_PS_PTR) & ATOM_CT_PS_MASK;
    ptr = base + ATOM_CT_CODE_PTR;

    Log(L">> execute %04X (len %d, WS %d, PS %d)\n", base, len, ws, ps);

    ectx.ctx = ctx;
    ectx.ps_shift = ps / 4;
    ectx.start = base;
    ectx.ps = params;
    ectx.abort = false;
    ectx.last_jump = 0;
    if (ws)
        //ectx.ws = kcalloc(4, ws, GFP_KERNEL);
        Log(L"ATOMFIX: Code needs fixing");
    else
        ectx.ws = NULL;

    debug_depth++;
    while (1) {
        op = CU8(ptr++);
        if (op < ATOM_OP_NAMES_CNT)
            Log(L"%s @ 0x%04X\n", atom_op_names[op], ptr - 1);
        else
            Log(L"[%d] @ 0x%04X\n", op, ptr - 1);
        if (ectx.abort) {
            Log(L"atombios stuck executing %04X (len %d, WS %d, PS %d) @ 0x%04X\n",
                base, len, ws, ps, ptr - 1);
            ret = -EINVAL;
            goto free;
        }

        if (op < ATOM_OP_CNT && op > 0)
            opcode_table[op].func(&ectx, &ptr,
                          opcode_table[op].arg);
        else
            break;

        if (op == ATOM_OP_EOT)
            break;
    }
    debug_depth--;
    Log(L"<<\n");

free:
    if (ws)
        //kfree(ectx.ws);
        Log(L"ATOMFIX: Code needs fixing");
    return ret;
}

int amdgpu_atom_execute_table(struct atom_context *ctx, int index, uint32_t * params)
{
    int r;

    //mutex_lock(&ctx->mutex);
    Log(L"ATOMFIX: Code needs fixing");
    /* reset data block */
    ctx->data_block = 0;
    /* reset reg block */
    ctx->reg_block = 0;
    /* reset fb window */
    ctx->fb_base = 0;
    /* reset io mode */
    ctx->io_mode = ATOM_IO_MM;
    /* reset divmul */
    ctx->divmul[0] = 0;
    ctx->divmul[1] = 0;
    r = amdgpu_atom_execute_table_locked(ctx, index, params);
    //mutex_unlock(&ctx->mutex);
    Log(L"ATOMFIX: Code needs fixing");
    return r;
}

static int atom_iio_len[] = { 1, 2, 3, 3, 3, 3, 4, 4, 4, 3 };

static void atom_index_iio(struct atom_context *ctx, int base)
{
    ctx->iio = Memory_Allocate(2 * 256);
    Log(L"ATOMFIX: Code needs fixing");
    if (!ctx->iio)
        return;
    while (CU8(base) == ATOM_IIO_START) {
        ctx->iio[CU8(base + 1)] = base + 2;
        base += 2;
        while (CU8(base) != ATOM_IIO_END)
            base += atom_iio_len[CU8(base)];
        base += 3;
    }
}
#define u16 UINT16
#define ATOM_BIOS_MAGIC     0xAA55
#define ATOM_ATI_MAGIC_PTR  0x30
#define ATOM_ATI_MAGIC      " 761295520"
#define ATOM_ROM_TABLE_PTR  0x48

#define ATOM_ROM_MAGIC      "ATOM"
#define ATOM_ROM_MAGIC_PTR  4


#define ATOM_ROM_MSG_PTR    0x10
#define ATOM_ROM_CMD_PTR    0x1E
#define ATOM_ROM_DATA_PTR   0x20

#define ATOM_DATA_FWI_PTR   0xC
#define ATOM_DATA_IIO_PTR   0x32

#define ATOM_ROM_PART_NUMBER_PTR    0x6E
extern void* gpubios;
struct atom_context AtomCtx;

VOID UTF8ToWchar(
    WCHAR* WcharStringDestination,
    ULONG WcharStringMaxWCharCount,
    CHAR* UTF8StringSource,
    ULONG UTF8StringByteCount
    );

struct atom_context *amdgpu_atom_parse(struct card_info *card, void *bios)
{
    int base;
    /*struct atom_context *ctx =
        kzalloc(sizeof(struct atom_context), GFP_KERNEL);*/
    AtomCtx.bios = gpubios;
    struct atom_context *ctx;
    ctx = &AtomCtx;

    char *str;
    u16 idx;

    if (!ctx)
        return NULL;

    ctx->card = card;
    ctx->bios = bios;

    if (CU16(0) != ATOM_BIOS_MAGIC) {
        Log(L"Invalid BIOS magic\n");
        //kfree(ctx);
        Log(L"ATOMFIX: Code needs fixing");
        return NULL;
    }
    if (strncmp
        (CSTR(ATOM_ATI_MAGIC_PTR), ATOM_ATI_MAGIC,
         strlen(ATOM_ATI_MAGIC))) {
        Log(L"Invalid ATI magic\n");
        //kfree(ctx);
        Log(L"ATOMFIX: Code needs fixing");
        return NULL;
    }

    base = CU16(ATOM_ROM_TABLE_PTR);
    if (strncmp
        (CSTR(base + ATOM_ROM_MAGIC_PTR), ATOM_ROM_MAGIC,
         strlen(ATOM_ROM_MAGIC))) {
        Log(L"Invalid ATOM magic\n");
        //kfree(ctx);
        Log(L"ATOMFIX: Code needs fixing");
        return NULL;
    }

    ctx->cmd_table = CU16(base + ATOM_ROM_CMD_PTR);
    ctx->data_table = CU16(base + ATOM_ROM_DATA_PTR);
    atom_index_iio(ctx, CU16(ctx->data_table + ATOM_DATA_IIO_PTR) + 4);
    if (!ctx->iio) {
        //amdgpu_atom_destroy(ctx);
        Log(L"ATOMFIX: Code needs fixing");
        return NULL;
    }

    idx = CU16(ATOM_ROM_PART_NUMBER_PTR);
    if (idx == 0)
        idx = 0x80;

    str = CSTR(idx);

    str = CSTR(CU16(base + ATOM_ROM_MSG_PTR));
      while (*str && ((*str == '\n') || (*str == '\r')))
          str++;
      char bios_name[70];
      /* name string isn't always 0 terminated */
       for (int i = 0; i < 63; i++) {
           bios_name[i] = str[i];
           char lol = '-';
           if (bios_name[i] == ' ' && str[i+1] == ' ') 
           {
               bios_name[i] = 0;
               break;
           }
       }

      wchar_t help[260];
      int len = strlen(bios_name);
      if (len > 63)
          len = 63;
      UTF8ToWchar(help, 260, str, len);
      Log(L"ATOM BIOS: %s\n", help);

    return ctx;
}


VOID AtomBios_Initialize()
{
    amdgpu_atom_parse(0, gpubios);
}
#define ATOM_CMD_INIT       0
#define ATOM_CMD_SETSCLK    0x0A
#define ATOM_CMD_SETMCLK    0x0B
#define ATOM_CMD_SETPCLK    0x0C
#define ATOM_CMD_SPDFANCNTL 0x39


int amdgpu_atom_asic_init(struct atom_context *ctx)
{
    int hwi = CU16(ctx->data_table + ATOM_DATA_FWI_PTR);
    uint32_t ps[16];
    int ret;

    memset(ps, 0, 64);

    //ps[0] = cpu_to_le32(CU32(hwi + ATOM_FWI_DEFSCLK_PTR));
    //ps[1] = cpu_to_le32(CU32(hwi + ATOM_FWI_DEFMCLK_PTR));
    Log(L"ATOMFIX: Code needs fixing");
    if (!ps[0] || !ps[1])
        return 1;

    if (!CU16(ctx->cmd_table + 4 + 2 * ATOM_CMD_INIT))
        return 1;
    ret = amdgpu_atom_execute_table(ctx, ATOM_CMD_INIT, ps);
    if (ret)
        return ret;

    memset(ps, 0, 64);

    return ret;
}

void amdgpu_atom_destroy(struct atom_context *ctx)
{
    //kfree(ctx->iio);
    //kfree(ctx);
    Log(L"ATOMFIX: Code needs fixing");
}

bool amdgpu_atom_parse_data_header(struct atom_context *ctx, int index,
                uint16_t * size, uint8_t * frev, uint8_t * crev,
                uint16_t * data_start)
{
    int offset = index * 2 + 4;
    int idx = CU16(ctx->data_table + offset);
    u16 *mdt = (u16 *)(ctx->bios + ctx->data_table + 4);

    if (!mdt[index])
        return false;

    if (size)
        *size = CU16(idx);
    if (frev)
        *frev = CU8(idx + 2);
    if (crev)
        *crev = CU8(idx + 3);
    *data_start = idx;
    return true;
}

bool amdgpu_atom_parse_cmd_header(struct atom_context *ctx, int index, uint8_t * frev,
               uint8_t * crev)
{
    int offset = index * 2 + 4;
    int idx = CU16(ctx->cmd_table + offset);
    u16 *mct = (u16 *)(ctx->bios + ctx->cmd_table + 4);

    if (!mct[index])
        return false;

    if (frev)
        *frev = CU8(idx + 2);
    if (crev)
        *crev = CU8(idx + 3);
    return true;
}

