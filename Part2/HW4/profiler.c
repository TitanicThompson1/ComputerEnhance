/*
*/
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../HW3/listing_0074_platform_metrics.c"

typedef uint64_t u64;

typedef struct 
{
    u64 start, elapsed;
    char label[128];        
}Profile;

typedef struct 
{
    Profile pfs[256];
    u64 start, end;
    int idx;
}Profiler;

static Profiler GlobalProfiler;

static void start_time_function(char* function_name){
    int i = 0;
    for (i = 0; i < GlobalProfiler.idx; i++) {
        if (strcmp(GlobalProfiler.pfs[i].label, function_name) == 0) break;
    }   
    if (i == GlobalProfiler.idx){
        GlobalProfiler.pfs[i] = (Profile){.start = 0, .elapsed=0};
        strcpy(GlobalProfiler.pfs[i].label, function_name);
        GlobalProfiler.idx++;
    }

    GlobalProfiler.pfs[i].start = ReadCPUTimer();
}

static void end_time_function(char* function_name){
    int i=0;
    for (i = 0; i < GlobalProfiler.idx; i++) {
        if (strcmp(GlobalProfiler.pfs[i].label, function_name) == 0) break;
    }   
    if (i==GlobalProfiler.idx){
        printf("Couldn't find metric for function %s", function_name);        
        return;
    }

    GlobalProfiler.pfs[i].elapsed += ReadCPUTimer() - GlobalProfiler.pfs[i].start;
    GlobalProfiler.pfs[i].start = 0;
}

static void StartProfile(){
    GlobalProfiler.start = ReadCPUTimer();
}

static void PrintProfile(){
    if (GlobalProfiler.idx == 0) return;

    GlobalProfiler.end = ReadCPUTimer();
    u64 total = GlobalProfiler.end - GlobalProfiler.start;
    u64 cpuf = EstimateCPUFrequency();

    printf("Total time: %fms/%lucycles (CPU freq %lu)\n", (double)total / cpuf, total, cpuf);
    for (int i = 0; i < GlobalProfiler.idx; i++){
        Profile fp = GlobalProfiler.pfs[i];
        printf("%s: %lu (%.2f%%)\n", fp.label, fp.elapsed, fp.elapsed/(double)total*100);
    }
}

#define StartTimeFunction start_time_function(__func__)

#define EndTimeFunction end_time_function(__func__)