#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GET_REG(val)    ((val >> 11) & 0x07)
#define GET_RM(val)     ((val >> 8) & 0x07)
#define GET_D(val)      ((val >> 1) & 0x01)
#define GET_W(val)      (val & 0x01)
#define GET_OPCODE(val) ((val >> 2) & 0x3F)

uint16_t MOV = 0x22;   // MOV opcode: 0010 0010

void get_register_name(uint8_t w, uint8_t breg, char *res){
    static const char *regs8[] = {
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
    };

    static const char *regs16[] = {
        "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
    };

    strcpy(res, w ? regs16[breg] : regs8[breg]);
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
    
    uint16_t op;
    char t_reg[3];
    char t_rm[3];

    while (fread(&op, sizeof(op), 1, fsource)){
        printf("%x", op);
        if (GET_OPCODE(op) != MOV){
            printf("Wrong OPCODE!\n");
            continue;
        }

        uint8_t w = GET_W(op);

        get_register_name(w, GET_REG(op), t_reg);
        get_register_name(w, GET_RM(op), t_rm);
        
        if (GET_D(op)){
            fprintf(fdest, "mov %s, %s\n", t_reg, t_rm);
        }else {
            fprintf(fdest, "mov %s, %s\n", t_rm, t_reg);
        }
    }

    fclose(fsource);
    fclose(fdest);
    return 0;
}
