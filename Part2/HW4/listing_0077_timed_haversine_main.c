

#include "../HW3/listing_0074_platform_metrics.c"
#include "../listings/listing_0065_haversine_formula.cpp"
#include "../HW2/json_parser.c"
#include "../HW4/profiler.c"

int main(int argc, char** argv){
    if(argc != 2){
        printf("Usage: %s <datafile>", argv[0]);
        return 1;
    }

    StartProfile();

    JSONField* json_data;
    size_t n;

    FILE *fsource = fopen(argv[1], "r");
    if (fsource == NULL){
        printf("Error while opening file %s\n", argv[1]);
        return 1;
    }
    start_time_function("Parse JSON");
    int err = parse_json(fsource, &json_data, &n);
    end_time_function("Parse JSON");

    if(err){
        printf("Something wrong happened while parsing the JSON file\n");
        return 1;
    }

    if(n != 1 || strcmp(json_data[0].key, "pairs") || json_data[0].ele.type != ARR){
        printf("JSON data has invalid structure\n");
        return 1;
    }

    start_time_function("Sum");
    JSONElement* pair_list = (JSONElement*)json_data[0].ele.val;    
    double sum = 0;
    printf("Number of pairs: %ld\n", json_data[0].ele.n);
    for (size_t i = 0; i < json_data[0].ele.n; i++){
        if(pair_list[i].type != ARR_OBJ){
            printf("Invalid type for pair %li", i);
            return 1;
        }

        JSONField* cur_pair = (JSONField*)pair_list[i].val;
        sum += ReferenceHaversine(get_number(cur_pair[0].ele), get_number(cur_pair[1].ele), get_number(cur_pair[2].ele), get_number(cur_pair[3].ele), EarthRadius);
    }
    end_time_function("Sum");

    start_time_function("Free");
    free_json_object(json_data, n);
    fclose(fsource);
    end_time_function("Free");


    printf("Medium haversine distance: %.16f\n", sum/json_data[0].ele.n);

    PrintProfile();

    return 0;
}