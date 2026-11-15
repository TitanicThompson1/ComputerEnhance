

#include "listing_0074_platform_metrics.c"
#include "../listings/listing_0065_haversine_formula.cpp"
#include "../HW2/json_parser.c"

int main(int argc, char** argv){
    if(argc != 2){
        printf("Usage: %s <datafile>", argv[0]);
        return 1;
    }

    u64 cpu_freq = EstimateCPUFrequency(), t1, t2, t3, t4;
    JSONField* json_data;
    size_t n;

    t1 = ReadCPUTimer();
    int err = parse_json_file(argv[1], &json_data, &n);
    t2 = ReadCPUTimer();
    
    if(err){
        printf("Something wrong happened while parsing the JSON file\n");
        return 1;
    }

    if(n != 1 || strcmp(json_data[0].key, "pairs") || json_data[0].ele.type != ARR){
        printf("JSON data has invalid structure\n");
        return 1;
    }

    JSONElement* pair_list = (JSONElement*)json_data[0].ele.val;    
    double sum = 0;
    t3 = ReadCPUTimer();
    for (size_t i = 0; i < json_data[0].ele.n; i++){
        if(pair_list[i].type != ARR_OBJ){
            printf("Invalid type for pair %li", i);
            return 1;
        }

        JSONField* cur_pair = (JSONField*)pair_list[i].val;
        sum += ReferenceHaversine(get_number(cur_pair[0].ele), get_number(cur_pair[1].ele), get_number(cur_pair[2].ele), get_number(cur_pair[3].ele), EarthRadius);
    }
    t4 = ReadCPUTimer();
    printf("Medium haversine distance: %.16f\n", sum/json_data[0].ele.n);

    double total_cycles = t4-t1;
    printf("Total time: %fms (CPU freq %lu)\n", total_cycles/cpu_freq, cpu_freq);
    printf("Parse: %ld (%.2f%%)\n",t2-t1, (t2-t1)/total_cycles*100);
    printf("Misc: %ld (%.2f%%)\n",t3-t2, (t3-t2)/total_cycles*100);
    printf("Sum: %ld (%.2f%%)\n",t4-t3, (t4-t3)/total_cycles*100);
}