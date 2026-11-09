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

uint8_t MOV = 0x22;         // MOV opcode: 0010 0010
uint8_t MOV_IMD = 0x0B;     // MOV IMMEDIATE opcode: 1011
uint8_t MOV_IMD_MEM = 0x63; // MOV IMMEDIATE TO MEMORY: 0110 0011
uint8_t MOV_ACC_REG = 0x28; // MOV ACC TO REG: 0010 1000

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

// Gets the RM register + displacement
void get_rm(FILE * fsource, uint8_t mod, uint8_t op, uint8_t w, char *t_rm){
    uint8_t b_rm, data8;
    uint16_t data16;
    char tmp[32];

    b_rm = GET_3BIT(op, 0);
    if (mod == 0x03){
        decode_register(w, b_rm, t_rm);
    }else{
        if (mod == 0x00 && b_rm == 0x06){
            fread(&data16, sizeof(data16), 1, fsource);
            sprintf(t_rm, "[%i]", data16);
        }else{
            strcpy(t_rm, rm_mod[b_rm]);
            char sign = '+';
            int8_t sdata8 = 0;
            int16_t sdata16 = 0;
            if (mod == 0x01)
                fread(&sdata8, sizeof(sdata8), 1, fsource);
            else if (mod == 0x02)
                fread(&sdata16, sizeof(sdata16), 1, fsource);
            
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

void get_inst_body(FILE *fsource, uint8_t op, char *t1, char * t2){
    uint8_t w = GET_1BIT(op, 0);
    uint8_t d = GET_1BIT(op, 1);

    char t_reg[16], t_rm[16];

    fread(&op, sizeof(op), 1, fsource);

    uint8_t mod = GET_2BIT(op, 6);

    uint8_t b_reg = GET_3BIT(op, 3);            
    decode_register(w, b_reg, t_reg);

    get_rm(fsource, mod, op, w, t_rm);

    if (d){
        strcpy(t1, t_reg);
        strcpy(t2, t_rm);
    }else{
        strcpy(t1, t_rm);
        strcpy(t2, t_reg);
    }
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
            //printf("DEBUG %x\n", op);
            strcpy(t_op, "mov");
            get_inst_body(fsource, op, t1, t2);

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
        }else if(GET_OPCODE(op, 1) == MOV_IMD_MEM){
            w = GET_1BIT(op, 0);

            fread(&op, sizeof(op), 1, fsource);

            mod = GET_2BIT(op, 6);

            get_rm(fsource, mod, op, w, t1);

            if (w == 1){
                fread(&data16, sizeof(data16), 1, fsource);
                sprintf(t2, "word %i", data16);
            }
            else{
                fread(&data8, sizeof(data8), 1, fsource);
                sprintf(t2, "byte %i", data8);
            }
        }else if (GET_OPCODE(op, 2) == MOV_ACC_REG){
            w = GET_1BIT(op, 0);
            d = GET_1BIT(op, 1);

            if (w == 1)
                strcpy(t_reg, "ax");
            else 
                strcpy(t_reg, "al");

            fread(&data16, sizeof(data16), 1, fsource);
            sprintf(t_rm, "[%i]", data8 ? data8 : data16);

            if (d){
                strcpy(t1, t_rm);
                strcpy(t2, t_reg);
            }else{
                strcpy(t1, t_reg);
                strcpy(t2, t_rm);
            }
        } else if (GET_OPCODE(op, 2))
        else{
            printf("Unrecognized code: %x\n", op);
            continue;
        }

        printf("%s %s, %s\n", t_op, t1, t2);
    }

    fclose(fsource);
    return 0;
}
