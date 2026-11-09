#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CODE 65536
#define MAX_INS  MAX_CODE
#define MAX_TEXT 128

typedef struct {
    int address;
    int length;
    int target;
    int is_branch;
    char text[MAX_TEXT];
} Instruction;

static uint8_t code[MAX_CODE];
static int code_size;
static int ip;

static Instruction instructions[MAX_INS];
static int instruction_count;

static int labels[MAX_CODE];

/* --------------------------------------------------------- */
/* Reading bytes                                             */
/* --------------------------------------------------------- */

static uint8_t read8(void)
{
    if (ip >= code_size) {
        fprintf(stderr, "Unexpected end of file\n");
        exit(1);
    }

    return code[ip++];
}

static uint16_t read16(void)
{
    uint8_t low = read8();
    uint8_t high = read8();

    return (uint16_t)((high << 8) | low);
}

static int8_t read_s8(void)
{
    return (int8_t)read8();
}

static int16_t read_s16(void)
{
    return (int16_t)read16();
}

/* --------------------------------------------------------- */
/* Registers                                                 */
/* --------------------------------------------------------- */

static const char *reg8[8] = {
    "al", "cl", "dl", "bl",
    "ah", "ch", "dh", "bh"
};

static const char *reg16[8] = {
    "ax", "cx", "dx", "bx",
    "sp", "bp", "si", "di"
};

static const char *ea_table[8] = {
    "bx+si", "bx+di", "bp+si", "bp+di",
    "si", "di", "bp", "bx"
};

/* --------------------------------------------------------- */
/* ModR/M decoding                                           */
/* --------------------------------------------------------- */

typedef struct {
    int mod;
    int reg;
    int rm;
} ModRM;

static ModRM read_modrm(void)
{
    uint8_t byte = read8();

    ModRM m;

    m.mod = (byte >> 6) & 3;
    m.reg = (byte >> 3) & 7;
    m.rm  = byte & 7;

    return m;
}

static void decode_rm(ModRM m, int w, char *out)
{
    if (m.mod == 3) {
        snprintf(out, MAX_TEXT, "%s",
                 w ? reg16[m.rm] : reg8[m.rm]);
        return;
    }

    if (m.mod == 0 && m.rm == 6) {
        uint16_t address = read16();

        snprintf(out, MAX_TEXT, "[0x%04X]", address);
        return;
    }

    char address[64];

    snprintf(address, sizeof(address), "%s", ea_table[m.rm]);

    if (m.mod == 1) {
        int8_t displacement = read_s8();

        if (displacement > 0) {
            snprintf(out, MAX_TEXT, "[%s+%d]",
                     address, displacement);
        } else if (displacement < 0) {
            snprintf(out, MAX_TEXT, "[%s-%d]",
                     address, -displacement);
        } else {
            snprintf(out, MAX_TEXT, "[%s]", address);
        }

        return;
    }

    if (m.mod == 2) {
        int16_t displacement = read_s16();

        if (displacement > 0) {
            snprintf(out, MAX_TEXT, "[%s+%d]",
                     address, displacement);
        } else if (displacement < 0) {
            snprintf(out, MAX_TEXT, "[%s-%d]",
                     address, -(int)displacement);
        } else {
            snprintf(out, MAX_TEXT, "[%s]", address);
        }

        return;
    }

    snprintf(out, MAX_TEXT, "[%s]", address);
}

/* --------------------------------------------------------- */
/* Label and branch helpers                                  */
/* --------------------------------------------------------- */

static int relative_target(int displacement)
{
    return (ip + displacement) & 0xFFFF;
}

static void set_branch(Instruction *ins, int target,
                       const char *mnemonic)
{
    ins->is_branch = 1;
    ins->target = target;

    snprintf(ins->text, sizeof(ins->text),
             "%s label_%04X", mnemonic, target);
}

/* --------------------------------------------------------- */
/* MOV                                                      */
/* --------------------------------------------------------- */

static void decode_mov(uint8_t opcode, Instruction *ins)
{
    char rm[ MAX_TEXT ];

    if (opcode >= 0xB0 && opcode <= 0xBF) {
        int w = (opcode >> 3) & 1;
        int reg = opcode & 7;

        if (w) {
            uint16_t imm = read16();

            snprintf(ins->text, sizeof(ins->text),
                     "mov %s, 0x%04X", reg16[reg], imm);
        } else {
            uint8_t imm = read8();

            snprintf(ins->text, sizeof(ins->text),
                     "mov %s, 0x%02X", reg8[reg], imm);
        }

        return;
    }

    if (opcode >= 0x88 && opcode <= 0x8B) {
        int d = (opcode >> 1) & 1;
        int w = opcode & 1;

        ModRM m = read_modrm();

        decode_rm(m, w, rm);

        const char *reg = w ? reg16[m.reg] : reg8[m.reg];

        if (d) {
            snprintf(ins->text, sizeof(ins->text),
                     "mov %s, %s", reg, rm);
        } else {
            snprintf(ins->text, sizeof(ins->text),
                     "mov %s, %s", rm, reg);
        }

        return;
    }

    if (opcode == 0xC6 || opcode == 0xC7) {
        int w = opcode & 1;

        ModRM m = read_modrm();
        decode_rm(m, w, rm);

        if (w) {
            uint16_t imm = read16();

            snprintf(ins->text, sizeof(ins->text),
                     "mov %s, 0x%04X", rm, imm);
        } else {
            uint8_t imm = read8();

            snprintf(ins->text, sizeof(ins->text),
                     "mov %s, 0x%02X", rm, imm);
        }

        return;
    }

    if (opcode >= 0xA0 && opcode <= 0xA3) {
        int w = opcode & 1;
        int d = (opcode >> 1) & 1;

        uint16_t address = read16();

        if (w && d) {
            snprintf(ins->text, sizeof(ins->text),
                     "mov [0x%04X], ax", address);
        } else if (w) {
            snprintf(ins->text, sizeof(ins->text),
                     "mov ax, [0x%04X]", address);
        } else if (d) {
            snprintf(ins->text, sizeof(ins->text),
                     "mov [0x%04X], al", address);
        } else {
            snprintf(ins->text, sizeof(ins->text),
                     "mov al, [0x%04X]", address);
        }

        return;
    }

    snprintf(ins->text, sizeof(ins->text),
             "db 0x%02X ; unsupported mov", opcode);
}

/* --------------------------------------------------------- */
/* ADD, SUB, CMP                                             */
/* --------------------------------------------------------- */

static void decode_arithmetic(uint8_t opcode, Instruction *ins)
{
    char rm[MAX_TEXT];

    const char *mnemonic = NULL;

    if (opcode >= 0x00 && opcode <= 0x03) {
        mnemonic = "add";
    } else if (opcode >= 0x28 && opcode <= 0x2B) {
        mnemonic = "sub";
    } else if (opcode >= 0x38 && opcode <= 0x3B) {
        mnemonic = "cmp";
    }

    if (mnemonic) {
        int d = (opcode >> 1) & 1;
        int w = opcode & 1;

        ModRM m = read_modrm();
        decode_rm(m, w, rm);

        const char *reg = w ? reg16[m.reg] : reg8[m.reg];

        if (d) {
            snprintf(ins->text, sizeof(ins->text),
                     "%s %s, %s", mnemonic, reg, rm);
        } else {
            snprintf(ins->text, sizeof(ins->text),
                     "%s %s, %s", mnemonic, rm, reg);
        }

        return;
    }

    /* Accumulator immediate forms */

    if (opcode == 0x04 || opcode == 0x05 ||
        opcode == 0x2C || opcode == 0x2D ||
        opcode == 0x3C || opcode == 0x3D) {

        int w = opcode & 1;

        if (opcode == 0x04 || opcode == 0x05) {
            mnemonic = "add";
        } else if (opcode == 0x2C || opcode == 0x2D) {
            mnemonic = "sub";
        } else {
            mnemonic = "cmp";
        }

        if (w) {
            uint16_t imm = read16();

            snprintf(ins->text, sizeof(ins->text),
                     "%s ax, 0x%04X", mnemonic, imm);
        } else {
            uint8_t imm = read8();

            snprintf(ins->text, sizeof(ins->text),
                     "%s al, 0x%02X", mnemonic, imm);
        }

        return;
    }

    /* Group 1: 80, 81, 83 */

    if (opcode == 0x80 || opcode == 0x81 || opcode == 0x83) {
        int w = opcode != 0x80;

        ModRM m = read_modrm();
        decode_rm(m, w, rm);

        switch (m.reg) {
            case 0: mnemonic = "add"; break;
            case 5: mnemonic = "sub"; break;
            case 7: mnemonic = "cmp"; break;
            default:
                snprintf(ins->text, sizeof(ins->text),
                         "db 0x%02X ; unsupported arithmetic group",
                         opcode);
                return;
        }

        if (opcode == 0x80) {
            uint8_t imm = read8();

            snprintf(ins->text, sizeof(ins->text),
                     "%s %s, 0x%02X", mnemonic, rm, imm);
        } else if (opcode == 0x81) {
            uint16_t imm = read16();

            snprintf(ins->text, sizeof(ins->text),
                     "%s %s, 0x%04X", mnemonic, rm, imm);
        } else {
            int8_t imm = read_s8();

            snprintf(ins->text, sizeof(ins->text),
                     "%s %s, %d", mnemonic, rm, imm);
        }

        return;
    }

    snprintf(ins->text, sizeof(ins->text),
             "db 0x%02X ; unsupported arithmetic", opcode);
}

/* --------------------------------------------------------- */
/* JMP                                                      */
/* --------------------------------------------------------- */

static void decode_jmp(uint8_t opcode, Instruction *ins)
{
    if (opcode == 0xEB) {
        int8_t displacement = read_s8();
        int target = relative_target(displacement);

        set_branch(ins, target, "jmp");
        return;
    }

    if (opcode == 0xE9) {
        int16_t displacement = read_s16();
        int target = relative_target(displacement);

        set_branch(ins, target, "jmp");
        return;
    }

    if (opcode == 0xEA) {
        uint16_t offset = read16();
        uint16_t segment = read16();

        snprintf(ins->text, sizeof(ins->text),
                 "jmp 0x%04X:0x%04X", segment, offset);
        return;
    }

    if (opcode == 0xFF) {
        ModRM m = read_modrm();

        if (m.reg == 4 || m.reg == 5) {
            char rm[MAX_TEXT];

            decode_rm(m, 1, rm);

            if (m.reg == 4) {
                snprintf(ins->text, sizeof(ins->text),
                         "jmp %s", rm);
            } else {
                snprintf(ins->text, sizeof(ins->text),
                         "jmp far %s", rm);
            }

            return;
        }
    }

    snprintf(ins->text, sizeof(ins->text),
             "db 0x%02X ; unsupported jmp", opcode);
}

/* --------------------------------------------------------- */
/* Conditional jumps                                         */
/* --------------------------------------------------------- */

static void decode_jcc(uint8_t opcode, Instruction *ins)
{
    static const char *jcc_names[16] = {
        "jo",   "jno",
        "jb",   "jnb",
        "je",   "jne",
        "jbe",  "ja",
        "js",   "jns",
        "jp",   "jnp",
        "jl",   "jnl",
        "jle",  "jg"
    };

    int8_t displacement = read_s8();
    int target = relative_target(displacement);

    set_branch(ins, target, jcc_names[opcode - 0x70]);
}

/* --------------------------------------------------------- */
/* LOOP, LOOPZ, LOOPNZ, JCXZ                                 */
/* --------------------------------------------------------- */

static void decode_loop(uint8_t opcode, Instruction *ins)
{
    static const char *loop_names[4] = {
        "loopnz",
        "loopz",
        "loop",
        "jcxz"
    };

    int8_t displacement = read_s8();
    int target = relative_target(displacement);

    set_branch(ins, target, loop_names[opcode - 0xE0]);
}

/* --------------------------------------------------------- */
/* Instruction dispatcher                                    */
/* --------------------------------------------------------- */

static void decode_instruction(Instruction *ins)
{
    int start = ip;
    uint8_t opcode = read8();

    ins->address = start;
    ins->is_branch = 0;
    ins->target = -1;

    if ((opcode >= 0xB0 && opcode <= 0xBF) ||
        (opcode >= 0x88 && opcode <= 0x8B) ||
        opcode == 0xC6 || opcode == 0xC7 ||
        (opcode >= 0xA0 && opcode <= 0xA3)) {

        decode_mov(opcode, ins);
    }
    else if ((opcode >= 0x00 && opcode <= 0x03) ||
             (opcode >= 0x28 && opcode <= 0x2B) ||
             (opcode >= 0x38 && opcode <= 0x3B) ||
             opcode == 0x04 || opcode == 0x05 ||
             opcode == 0x2C || opcode == 0x2D ||
             opcode == 0x3C || opcode == 0x3D ||
             opcode == 0x80 || opcode == 0x81 ||
             opcode == 0x83) {

        decode_arithmetic(opcode, ins);
    }
    else if (opcode == 0xEB || opcode == 0xE9 ||
             opcode == 0xEA || opcode == 0xFF) {

        decode_jmp(opcode, ins);
    }
    else if (opcode >= 0x70 && opcode <= 0x7F) {
        decode_jcc(opcode, ins);
    }
    else if (opcode >= 0xE0 && opcode <= 0xE3) {
        decode_loop(opcode, ins);
    }
    else {
        snprintf(ins->text, sizeof(ins->text),
                 "db 0x%02X ; unknown opcode", opcode);
    }

    ins->length = ip - start;
}

/* --------------------------------------------------------- */
/* Main                                                      */
/* --------------------------------------------------------- */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.bin\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");

    if (!file) {
        perror("fopen");
        return 1;
    }

    code_size = (int)fread(code, 1, MAX_CODE, file);
    fclose(file);

    if (code_size == 0) {
        fprintf(stderr, "Input file is empty\n");
        return 1;
    }

    /* First pass: decode instructions and collect branch targets */

    ip = 0;

    while (ip < code_size && instruction_count < MAX_INS) {
        Instruction *ins = &instructions[instruction_count];

        decode_instruction(ins);

        if (ins->is_branch &&
            ins->target >= 0 &&
            ins->target < code_size) {

            labels[ins->target] = 1;
        }

        instruction_count++;
    }

    /* Second pass: print assembly and labels */

    printf("bits 16\n\n");

    for (int i = 0; i < instruction_count; i++) {
        Instruction *ins = &instructions[i];

        if (ins->address >= 0 &&
            ins->address < code_size &&
            labels[ins->address]) {

            printf("label_%04X:\n", ins->address);
        }

        printf("    %s\n", ins->text);
    }

    return 0;
}