#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define BUFF_LEN 1056784

const char LBRACKET = '{';
const char RBRACKET = '}';
const char SEMICOLON = ':';
const char LSBRACKET = '[';
const char RSBRACKET = ']';

static uint8_t STR=0, NUMBER=1, ARR=2, ARR_OBJ=3;

typedef struct 
{
    uint8_t type;       // 0-> string, 1->number, 2->array list, 3-> array json objects
    void* val;
    size_t n;
} JSONElement;

typedef struct 
{
    char* key;
    JSONElement ele;
} JSONField;

typedef struct
{
    FILE* fsource;
    char buffer[BUFF_LEN];
    size_t idx, n;
} FileBuffer;

bool is_digit(const char d){
    return d >= '0' && d <= '9';
}

double get_number(JSONElement ele){
    return *((double*)ele.val);
}

void load_buffer(FileBuffer* fb){
    size_t bytes_read;
    bytes_read = fread(fb->buffer, sizeof(char), BUFF_LEN - 1 , fb->fsource);
    (fb->buffer)[bytes_read] = '\0';
    fb->n = bytes_read + 1;
    fb->idx = 0;
}

int peek_char(FileBuffer* fb, char* c){
    if (fb->idx == BUFF_LEN - 1){
        load_buffer(fb);
    }
    if(fb->idx == fb->n){
        printf("Reached EOF");
        return 1;
    }
    *c = fb->buffer[fb->idx];
    return 0;
}

int get_char(FileBuffer* fb, char* c){
    if (fb->idx == BUFF_LEN){
        load_buffer(fb);
    }
    if(fb->idx == fb->n){
        printf("Reached EOF");
        return 1;
    }

    *c = fb->buffer[fb->idx];
    fb->idx++;
    return 0;
}

int get_string(FileBuffer* fb, char** out){
    size_t i=0, len=256;
    char* str = malloc(len * sizeof(char));
    if (str == NULL) return 1;
    do{
        if(get_char(fb, &str[i])) return 1;
        if (i==len){
            len *= 2;
            char* temp = realloc(str, len);
            if (temp == NULL) return 1;
            str = temp;
        }
    }while(str[i] != '"' && ++i);
    str[i] = '\0';
    
    *out = malloc(i * sizeof(char));
    strcpy(*out, str);    
    return 0; 
}

int parse_number(FileBuffer* fb, double* out){
    size_t i=0, MAX=1027;
    char number[MAX+1]; //Only accept number up to 1027 digits 
    char cur_c;
    while(true){
        if(peek_char(fb, &cur_c)) return 1;
        if(is_digit(cur_c) || cur_c == '-'){
            if(get_char(fb, &number[i++])) return 1;
        }else 
            break;
    }

    if (cur_c == '.'){
        if(get_char(fb, &number[i++])) return 1;
        while(true){
            if(peek_char(fb, &cur_c)) return 1;
            if(is_digit(cur_c)){
                if(get_char(fb, &number[i++])) return 1;
            }else 
                break;
        }
    }

    number[i] = '\0'; 
    *out = atof(number);    
    return 0;
}

int skip_whitespaces_newline(FileBuffer* fb){
    char c;
    while (true)
    {
        if(peek_char(fb, &c)) return 1;
        if(c == ' ' || c == '\n'){
            if(get_char(fb, &c)) return 1;
        }
        else
            break;
    }
    return 0;
}

int parse_json_list(FileBuffer* fb, JSONElement** out, size_t* n_out);
int parse_json_object(FileBuffer* fb, JSONField** out, size_t* n_out);

void print_json_object(JSONField* fields, size_t n, size_t identation);
void print_json_list(JSONElement* eles, size_t n);

void free_json_element(JSONElement ele);
void free_json_object(JSONField* fields, size_t n);
void free_json_object_inner(JSONField* fields, size_t n);

void free_json_object_inner(JSONField* fields, size_t n){
    for(size_t i = 0; i < n; i++){
        JSONField f = fields[i];
        free_json_element(f.ele);
        free(f.key);
    }
}

void free_json_element(JSONElement ele){
    if(ele.type < ARR){
        free(ele.val);
    }else if (ele.type == ARR){
        JSONElement* children = (JSONElement*)ele.val;
        for (size_t i=0; i< ele.n; i++){
            free_json_element(children[i]);
        }
        free(children);
    }else if(ele.type == ARR_OBJ){
        JSONField* children = (JSONField*)ele.val;
        free_json_object_inner(children, ele.n);
        free(children);
    }
}

void free_json_object(JSONField* fields, size_t n){
    free_json_object_inner(fields, n);
    free(fields);
}


void print_json_list(JSONElement* eles, size_t n){
    for (size_t i = 0; i < n; i++){
        JSONElement ele = eles[i];
        if(ele.type == STR){
            printf("'%s', ", (char*)ele.val);
        }else if (ele.type == NUMBER){
            printf("%f, ", *(double*)(ele.val));
        }else if (ele.type == ARR){
            printf("[");
            print_json_list((JSONElement*)ele.val, ele.n);
            printf("], ");
        }else if (ele.type == ARR_OBJ){
            printf("{");
            print_json_object((JSONField*)ele.val, ele.n, 0);
            printf("}, ");
        }
    }
}

void print_json_object(JSONField* fields, size_t n, size_t identation){
    char iden[identation+1];
    for (size_t i = 0; i < identation; i++){
        iden[i] = '\t';
    }
    iden[identation] = '\0';

    for (size_t i = 0; i < n; i++){
        JSONField f = fields[i];
        printf("%s%s: ", iden, f.key);
        if(f.ele.type == STR){
            printf("'%s'\n", (char*)f.ele.val);
        }else if (f.ele.type == NUMBER){
            printf("%f\n", *(double*)(f.ele.val));
        }else if (f.ele.type == ARR){
            printf("[");
            print_json_list((JSONElement*)f.ele.val, f.ele.n);
            printf("]\n");
        }else if (f.ele.type == ARR_OBJ){
            printf("{\n");
            print_json_object((JSONField*)f.ele.val, f.ele.n, ++identation);
            printf("\n}");
        }
    } 
}

int parse_json_string(FileBuffer* fb, bool accept_empty, char** out){
    get_string(fb, out);
    if(sizeof(*out) == 0 && !accept_empty) return 1;
    return 0; 
}


int parse_json_element(FileBuffer* fb, JSONElement* out){
    if(skip_whitespaces_newline(fb)) return 1;
    
    char cur_c; 
    if (peek_char(fb, &cur_c)) return 1;

    if (cur_c == '"'){
        if (get_char(fb, &cur_c)) return 1;
        if(parse_json_string(fb, true, (char **)&(out->val))) return 1;
        out->type = STR;
    }else if (is_digit(cur_c) || cur_c == '-'){
        out->val = malloc(sizeof(double)); 
        if(parse_number(fb, out->val)) return 1;
        out->type = NUMBER;
    }else if (cur_c == LSBRACKET){
        if(get_char(fb, &cur_c)) return 1;
        if(parse_json_list(fb, (JSONElement**)&(out->val), &out->n)) return 1;
        out->type = ARR;
    }else if (cur_c == LBRACKET){
        if(parse_json_object(fb, (JSONField**)&(out->val), &out->n)) return 1;
        out->type = ARR_OBJ;
    }else{
        printf("Invalid JSON schema at %li, got '%c'\n", fb->idx, cur_c);
        return 1;
    }
    return 0;
}

int parse_json_list(FileBuffer* fb, JSONElement** out, size_t* n_out){
    size_t size_out = 64;
    *out = (JSONElement*)malloc(size_out * sizeof(JSONElement));
    if(*out == NULL){
        printf("Error while allocating memory");
        return 1;
    }
    
    char cur_c;
    *n_out = 0;
    bool has_trailing_comma=false, first=true;
    if (peek_char(fb, &cur_c)) return 1;

    while(cur_c != RSBRACKET){
        if(!has_trailing_comma && !first){
            printf("Error in JSON schema: missing ',' at %li, got '%c'\n", fb->idx, cur_c);
            return 1;
        }
        if (*n_out == size_out - 1){
            size_out *= 2;
            JSONElement* temp = (JSONElement*)realloc(*out, size_out * sizeof(JSONElement));
            if (temp == NULL){
                printf("Error while reallocating memory");
                return 1;
            }
            *out = temp;
        }

        JSONElement ele;
        if(skip_whitespaces_newline(fb)) return 1;

        if(parse_json_element(fb, &ele)) return 1;
        (*out)[(*n_out)++] = ele;
        first = false;

        if(skip_whitespaces_newline(fb)) return 1;
        if(get_char(fb, &cur_c)) return 1;
        if(cur_c == ','){
            has_trailing_comma = true;
            if(skip_whitespaces_newline(fb)) return 1;
        }
        else
            has_trailing_comma = false;
    }
    if(*n_out == 0){
        if(get_char(fb, &cur_c)) return 1;
    }
    if(has_trailing_comma && !first){
        printf("Error in JSON schema: extra ',' at %li, got '%c'\n", fb->idx, cur_c);
        return 1;
    }
    return 0;
}

int parse_json_object(FileBuffer* fb, JSONField** out, size_t* n_out){
    size_t size_out = 16;
    *out = (JSONField*)malloc(size_out * sizeof(JSONField));
    if(*out == NULL){
        printf("Error while allocating memory\n");
        return 1;
    }

    if(skip_whitespaces_newline(fb)) return 1;
    char cur_c;
    if (get_char(fb, &cur_c)) return 1;

    if (cur_c != LBRACKET){
        printf("Invalid JSON schema at idx %li, got %c\n", fb->idx, cur_c);
        return 1;
    }
    if(skip_whitespaces_newline(fb)) return 1;
    if (get_char(fb, &cur_c)) return 1;
    *n_out = 0;
    bool has_trailing_comma=false, first=true;
    while(cur_c != RBRACKET){
        if(!has_trailing_comma && !first){
            printf("Error in JSON schema: missing ',' at %li, got '%c'\n", fb->idx, cur_c);
            return 1;
        }
        
        if (cur_c != '"'){
            printf("Error in JSON schema: missing '\"' at %li, got %c\n", fb->idx, cur_c);
            return 1;
        }
        JSONField f;
        if(parse_json_string(fb, false, &f.key)) return 1;
        if(skip_whitespaces_newline(fb)) return 1;
        if (get_char(fb, &cur_c)) return 1;

        if (cur_c != SEMICOLON){
            printf("Invalid JSON schema at %li, got %c\n", fb->idx, cur_c);
            return 1;
        }
        if(skip_whitespaces_newline(fb)) return 1;
        
        if(parse_json_element(fb, &f.ele)) return 1;
        (*out)[(*n_out)++] = f;
        first = false;
        if(skip_whitespaces_newline(fb)) return 1;
        if(get_char(fb, &cur_c)) return 1;

        if (cur_c == ','){
            has_trailing_comma = true;
            if(skip_whitespaces_newline(fb)) return 1;
            if(get_char(fb, &cur_c)) return 1;
        }
        else
            has_trailing_comma = false;
    }

    if(has_trailing_comma && !first){
        printf("Error in JSON schema: extra ',' at %li, got %c", fb->idx, cur_c);
        exit(1);
    }
    return 0;
}

int parse_json_file(char* filename, JSONField** out_res, size_t* out_n){

    FILE *fsource = fopen(filename, "r");
    if (fsource == NULL){
        printf("Error while opening file %s\n", filename);
        return 1;
    }

    FileBuffer fb = { .fsource=fsource };
    load_buffer(&fb);
    
    return parse_json_object(&fb, out_res, out_n);
}
/*
int main(int argc, char **argv){
    if (argc != 2){
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *fsource = fopen(argv[1], "r");
    if (fsource == NULL){
        printf("Error while opening file %s\n", argv[1]);
        return 1;
    }

    FileBuffer fb = { .fsource=fsource };
    load_buffer(&fb);
    
    JSONField* res;
    size_t size_json;
    int err = parse_json_object(&fb, &res, &size_json);
    if(err){
        printf("Something wrong happened while parsing the file");        
    }else{
        printf("Successfully parsed JSON file!\n");
        print_json_object(res, size_json, 0);
    }
    free_json_object(res, size_json);
    return err;
}
*/