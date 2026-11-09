/*I recognize this code could be betterif we had a table with every instruction and how to decode them
But I realized this to late, and for the purpose of the exercise,its good enough
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define GET_3BIT(val, dis)    ((val >> dis) & 0x07)
#define GET_2BIT(val, dis)    ((val >> dis) & 0x03)
#define GET_1BIT(val, dis)      ((val >> dis) & 0x01)
#define GET_OPCODE(val, dis) (val >> dis)

#define GET_FIELD(val, len, dis) ((val >> dis) & (0xFF >> (8 - len)))

uint8_t MOV = 0x22;         // MOV opcode: 0010 0010
uint8_t MOV_IMD = 0x0B;     // MOV IMMEDIATE opcode: 1011
uint8_t MOV_IMD_MEM = 0x63; // MOV IMMEDIATE TO MEMORY: 0110 0011
uint8_t MOV_ACC_REG = 0x28; // MOV ACC TO REG: 0010 1000
uint8_t MAX_VAL = 0xFF;

static const char *regs8[] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

static const char *regs16[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

static const char *rm_mod[] = {
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
};

typedef struct
{
    uint8_t byte, dis, b_val;     // The byte where it is, and displacement from the 0 bit   
} Field;


typedef struct
{
    char op_val[8];
    uint8_t len;
    Field op, w, d, s, mod, reg, rm, imd;
    bool acc, acc_dest;
} Instruction;

int n = 3;
Instruction insts[32] = {
    {   // MOV Register
        .len = 2,
        .op.dis = 2,
        .op.b_val = 0x22,
        .op_val = "mov",

        .w.dis = 0,
        .w.byte = 0,
        
        .d.dis = 1,
        .d.byte = 0,
        
        .mod.dis = 6,
        .mod.byte = 1,

        .reg.byte = 1,
        .reg.dis = 3,

        .rm.byte = 1,
        .rm.dis = 0,

        .imd.byte = 0xFF
    },
    {   // MOV immediate to reg / mem
        .len = 2,
        .op.dis = 1,
        .op.b_val = 0x63,
        .op_val = "mov",

        .w.dis = 0,
        .w.byte = 0,
        
        .d.dis = 0xFF,
        
        .mod.byte = 1,
        .mod.dis = 6,

        .reg.byte = 0xFF,

        .rm.byte = 1,
        .rm.dis = 0,

        .imd.byte = 4
    },
    {   // MOV immediate to reg 
        .len = 1,
        .op.dis = 4,
        .op.b_val = 0x0B,
        .op_val = "mov",

        .w.byte = 0,
        .w.dis = 3,
        
        .d.byte = 0xFF,
        
        .mod.byte = 0xFF,

        .reg.byte = 0,
        .reg.dis = 0,

        .rm.byte = 0xFF,

        .imd.byte = 1
    },
    {   // MOV memory to accumulator
        .len = 1,
        .op.dis = 7,
        .op.b_val = 0x50,
        .op_val = "mov",

        .w.byte = 0,
        .w.dis = 0,
        
        .d.byte = 0xFF,
        
        .mod.byte = 0xFF,

        // Not exactly true, but true enough
        .reg.byte = 0,
        .reg.dis = 2,

        .rm.byte = 0xFF,

        .imd.byte = 1,

        .acc = true,
        .acc_dest = true
    },
    {   // MOV immediate to reg 
        .len = 1,
        .op.dis = 4,
        .op.b_val = 0x0B,
        .op_val = "mov",

        .w.byte = 0,
        .w.dis = 3,
        
        .d.byte = 0xFF,
        
        .mod.byte = 0xFF,

        .reg.byte = 0,
        .reg.dis = 0,

        .rm.byte = 0xFF,

        .imd.byte = 1,

        .acc = true,
    }
};

uint8_t get_field(uint8_t *raw, Field f, uint8_t len){
    if (f.byte == MAX_VAL)
        return MAX_VAL;
    return GET_FIELD(raw[f.byte], len, f.dis);
}

void decode_register(uint8_t w, uint8_t breg, char *res){
    strcpy(res, w ? regs16[breg] : regs8[breg]);
}

// Gets the RM register + displacement
void get_rm(uint8_t *raw, Field rm, uint8_t mod, uint8_t w, char *t_rm){
    uint8_t b_rm, data8;
    uint16_t data16;
    char tmp[32];

    b_rm = GET_3BIT(raw[rm.byte], rm.dis);
    if (mod == 0x03){
        decode_register(w, b_rm, t_rm);
    }else{
        if (mod == 0x00 && b_rm == 0x06){
            data16 = ((uint16_t)raw[3] << 8) | raw[2];
            sprintf(t_rm, "[%i]", data16);
        }else{
            strcpy(t_rm, rm_mod[b_rm]);

            char sign = '+';
            int8_t sdata8 = 0;
            int16_t sdata16 = 0;
            if (mod == 0x01)
                sdata8 = (int8_t) raw[2];
            else if (mod == 0x02)
                sdata16 = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
            
            if (sdata16 < 0 || sdata8 < 0){
                sign = '-';
                sdata16 = abs(sdata16);
                sdata8 = abs(sdata8);
            }
            
            if (sdata8 || sdata16){
                sprintf(tmp, "[%s %c %i]", t_rm, sign, sdata8 ? sdata8 : sdata16);
            }
            else
                sprintf(tmp, "[%s]", t_rm);
            
            strcpy(t_rm, tmp);
        }
    }
}

void get_reg(uint8_t *raw, uint8_t w, Field reg, char *t_reg){

    uint8_t b_reg = GET_FIELD(raw[reg.byte], 3, reg.dis);
    strcpy(t_reg, w ? regs16[b_reg] : regs8[b_reg]);
}

int main(int argc, char *argv[]){
    if (argc != 2){
        printf("Usage: decode <filename>");
        return 0;
    }

    FILE *fsource = fopen(argv[1], "rb");
    if (fsource == NULL){
        printf("Error while opening file %s", argv[1]);
        return 1;
    }

    printf("bits 16\n\n");
    
    uint8_t w, d, s, mod;
    int8_t sdata8;
    int16_t sdata16;

    uint8_t mem[1028 * 1028];
    uint8_t raw[6];

    char t_reg[32];
    char t_rm[32];

    char t_op[8];
    char t1[32];
    char t2[32];

    int im = 0, n_m, i =0;
    
    while(fread(&mem[im], sizeof(uint8_t), 1, fsource)){
        im++;
    }
    
    n_m = im;
    im = 0;
    while (im < n_m){
        memcpy(raw, &mem[im], 6);
        while (i < n){
            Instruction inst = insts[i];
            i++;

            uint8_t total_size = inst.len;

            if ((raw[0] >> inst.op.dis) != inst.op.b_val)
                continue;
            i = 0;      
            strcpy(t_op, inst.op_val);

            w = get_field(raw, inst.w, 1);
            d = get_field(raw, inst.d, 1);
            s = get_field(raw, inst.s, 1);
            mod = get_field(raw, inst.mod, 2);

            if (mod < 3)
                total_size += mod;

            if (inst.reg.byte != MAX_VAL){
                get_reg(raw, w, inst.reg, d == 0 ? t2 : t1);
                //printf("DEBUG %s %i\n", t1,d);
            }
            
            if (inst.rm.byte != MAX_VAL){
                get_rm(raw, inst.rm, mod, w,  d == 1 ? t2 : t1);
            }

            if (inst.imd.byte != MAX_VAL){
                if (w == 1)
                    sprintf(t2, "%s%i", inst.rm.byte != MAX_VAL ? "word " : "", (int16_t)(((uint16_t)raw[inst.imd.byte + 1] << 8) | raw[inst.imd.byte]));
                else
                    sprintf(t2, "%s%i", inst.rm.byte != MAX_VAL ? "byte " : "", (int8_t) raw[inst.imd.byte]);
                total_size += w + 1;
            }

            if (inst.acc){
                if (w == 1)
                    sprintf(inst.acc_dest ? t1 : t2, "%i", (int16_t)(((uint16_t)raw[inst.imd.byte + 1] << 8) | raw[inst.imd.byte]));
                else
                    sprintf(inst.acc_dest ? t1 : t2, "%i", (int8_t) raw[inst.imd.byte]);
                
               
                total_size += w + 1;
            }

            printf("%s %s, %s\n", t_op, t1, t2);
            im += total_size;
            break;
        }
        if (i == n){
            printf("Unrecognized instruction: %x\n", raw[0]);
            break;
        }
    }

    fclose(fsource);
    return 0;
}
