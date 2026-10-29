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

static const char *regs8[] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

static const char *regs16[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

static const char *rm_mod[] = {
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
};

void decode_register(uint8_t w, uint8_t breg, char *res){
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

    printf("bits 16\n\n");
    
    uint8_t op, w, d, mod, b_rm, b_reg, data8;
    int8_t sdata8;
    uint16_t data16;
    int16_t sdata16;

    char t_reg[32];
    char t_rm[32];
    char tmp[32];

    char t_op[8];
    char t1[32];
    char t2[32];

    while (fread(&op, sizeof(op), 1, fsource)){
        sdata8 = 0;
        data8 = 0;

        sdata16 = 0;
        data16 = 0;
        
        // Parsing
        if (GET_OPCODE(op, 2) == MOV){
            strcpy(t_op, "mov");
            w = GET_1BIT(op, 0);
            d = GET_1BIT(op, 1);

            fread(&op, sizeof(op), 1, fsource);

            mod = GET_2BIT(op, 6);

            b_reg = GET_3BIT(op, 3);            
            decode_register(w, b_reg, t_reg);

            b_rm = GET_3BIT(op, 0);
            if (mod == 0x03){
                decode_register(w, b_rm, t_rm);
            }else{
                get_rm(b_rm, t_rm);
                if (mod == 0x00 && b_rm == 0x06){
                    fread(&data16, sizeof(data16), 1, fsource);
                    sprintf(t_rm, "[%i]", data16);
                }else{
                    if (mod == 0x01)
                        fread(&sdata8, sizeof(sdata8), 1, fsource);
                    else if (mod == 0x02)
                        fread(&sdata16, sizeof(sdata16), 1, fsource);

                    if (sdata8 || sdata16){
                        sprintf(tmp, "[%s + %i]", t_rm, sdata8 ? sdata8 : sdata16);
                    }
                    else
                        sprintf(tmp, "[%s]", t_rm);
                    }
                    strcpy(t_rm, tmp);
                }

            if (d){
                strcpy(t1, t_reg);
                strcpy(t2, t_rm);
            }else{
                strcpy(t1, t_rm);
                strcpy(t2, t_reg);
            }
        }else if (GET_OPCODE(op, 4) == MOV_IMD){
            strcpy(t_op, "mov");
            uint8_t w = GET_1BIT(op, 3);
            b_reg = GET_3BIT(op, 0);
            decode_register(w, b_reg, t1);

            if (w == 1)
                fread(&data16, sizeof(data16), 1, fsource);
            else
                fread(&data8, sizeof(data8), 1, fsource);
            
            sprintf(t2, "%i", data8 ? data8 : data16);
        }

        printf("%s %s, %s\n", t_op, t1, t2);
    }

    fclose(fsource);
    return 0;
}
