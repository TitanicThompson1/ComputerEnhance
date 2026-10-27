#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define GET_3BIT(val, dis)    ((val >> dis) & 0x07)
#define GET_2BIT(val, dis)    ((val >> dis) & 0x03)
#define GET_1BIT(val, dis)      ((val >> dis) & 0x01)
#define GET_OPCODE(val, dis) ((val >> dis) & 0x3F)

uint16_t MOV = 0x22;        // MOV opcode: 0010 0010
uint16_t MOV_IMD = 0x0B;     // MOV IMMEDIATE opcode: 1011

static const char *DIRECT_MEM_ACCESS = "DIR";

static const char *regs8[] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

static const char *regs16[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

static const char *rm_mod[] = {
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
};

void get_register_name(uint8_t w, uint8_t breg, char *res){
    strcpy(res, w ? regs16[breg] : regs8[breg]);
}

void get_rm(uint8_t rm, char *res){
    strcpy(res, rm_mod[rm]);
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

    FILE *fdest = fopen("res.txt", "w");
    if (fdest == NULL){
        printf("Error while creating res file");
        return 1;
    }

    fwrite("bits 16\n\n", 1, 9, fdest);
    
    uint8_t op, w, d, mod, b_rm, data8;
    uint16_t data16;
    char t_reg[3];
    char t_rm[16];
    char t_offset[10] = {'\0'};
    bool direct = false;

    while (fread(&op, sizeof(op), 1, fsource)){
        data8 = 0;
        data16 = 0;
        direct = false;

        printf("\n%x ", op);

        if (GET_OPCODE(op, 2) == MOV){
            w = GET_1BIT(op, 0);
            d = GET_1BIT(op, 1);

            fread(&op, sizeof(op), 1, fsource);

            mod = GET_2BIT(op, 6);

            get_register_name(w, GET_3BIT(op, 3), t_reg);

            if (mod == 0x03){
                get_register_name(w, GET_3BIT(op, 0), t_rm);
            }else{
                b_rm = GET_3BIT(op, 0);
                get_rm(b_rm, t_rm);
                if (mod == 0x00 && b_rm == 0x06){
                    fread(&data16, sizeof(data16), 1, fsource);
                    direct = true;
                }else if (mod == 0x01){
                    fread(&data8, sizeof(data8), 1, fsource);
                }else if (mod == 0x02){
                    fread(&data16, sizeof(data16), 1, fsource);
                }
            }

            char s_rm[16];

            if (mod == 0x03){
                strcpy(s_rm, t_rm);
            }else if (direct){
                sprintf(s_rm, "[%i]", data16);
            }else if (data8 != 0 || data16 != 0){
                sprintf(s_rm, "[%s + %i]", t_rm,  data8 ? data8 : data16);
            }else{
                sprintf(s_rm, "[%s]", t_rm);
            }

            char *t1 = d == 1 ? t_reg : s_rm;
            char *t2 = d == 1 ? s_rm : t_reg;

            fprintf(fdest, "mov %s, %s\n", t1, t2);
            

            
        }else if (GET_OPCODE(op, 4) == MOV_IMD){
            uint8_t w = GET_1BIT(op, 3);
            get_register_name(w, GET_3BIT(op, 0), t_reg);

            if (w == 1){
                fread(&data16, sizeof(data16), 1, fsource);
                get_register_name(w, GET_3BIT(op, 0), t_reg);
                fprintf(fdest, "mov %s, %i\n", t_reg, data16);
            }else{
                fread(&data8, sizeof(data8), 1, fsource);
                get_register_name(w, GET_3BIT(op, 0), t_reg);
                fprintf(fdest, "mov %s, %i\n", t_reg, data8);
            }
        }
    }

    fclose(fsource);
    fclose(fdest);
    return 0;
}
