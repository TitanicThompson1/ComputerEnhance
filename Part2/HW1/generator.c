#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

double get_random_number(double min, double max){
    return min + ((double)rand() / RAND_MAX) * (max - min);
}

double random_triangular(double min, double max, double mode) {
    double u = (double)rand() / RAND_MAX;
    double f = (mode - min) / (max - min);

    if (u < f) {
        return min + sqrt(u * (max - min) * (mode - min));
    }

    return max - sqrt((1.0 - u) * (max - min) * (max - mode));
}

int main(int argc, char** argv){
    if (argc != 4){
        printf("Usage: %s <uniform/cluster> <seed> <number_examples>", argv[0]);
        return 1;
    }

    FILE *fdest = fopen("haversine_samples.json", "w");
    if (fdest == NULL){
        printf("Error while creating res file");
        return 1;
    }

    srand(atoi(argv[2]));

    char preamble[] = "{\"pairs\":[\n";
    fwrite(preamble, sizeof(char), sizeof(preamble) - 1, fdest);

    bool uniform = argv[1][0] == 'u';

    char pairs[256];
    int n = atoi(argv[3]), line_size;
    for (int i = 0; i < n; i++){
        double x0, x1, y0, y1;
        if (uniform){
            x0 = get_random_number(-180, 180);
            x1 = get_random_number(-180, 180);
            y0 = get_random_number(-90, 90);
            y1 = get_random_number(-90, 90);         
        }else{
            x0 = random_triangular(-180, 180, i % 180);
            x1 = random_triangular(-180, 180, i % 180);
            y0 = random_triangular(-90, 90, i % 90);
            y1 = random_triangular(-90, 90, i % 90);         
        }

        line_size = sprintf(pairs, "{\"x0\":%.16f, \"y0\":%.16f, \"x1\":%.16f, \"y1\":%.16f}", x0, y0, x1, y1);
        if (i != n -1){
            sprintf(&pairs[line_size], ",\n");
            line_size += 2;
        }
        else{
            sprintf(&pairs[line_size], "\n");
            line_size += 1;
        }

        fwrite(pairs, 1, line_size, fdest);
    }    
    
    fwrite("]}", sizeof(char), 2, fdest);

    fclose(fdest);
    return 0;
}