/*  gcc main.c -o main.exe ; & 'c:\Users\Kuba\Documents\GitHub\1brc in C\main.exe'

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TABLE_SIZE (1<<19)

size_t djb2_hash(const char* pStr){
    size_t hash_value = 5381;
    char c;
    while((c = *pStr++)){
        hash_value = ((hash_value << 5) + hash_value) + c;
    }
    return hash_value % TABLE_SIZE;
}

struct brc{
    const char* pCity;
    double min;
    double max;
    double sum;
    int count;
};

struct brc* db[TABLE_SIZE] = {0};

void hash_map_add(struct brc* entry){
    size_t hash = djb2_hash(entry->pCity);
    if(db[hash]){
        printf("hash_map_add collision at %llu", hash);
        exit(EXIT_FAILURE);
    }
    db[hash] = entry;
}
/*
void hash_map_update(const char* pCity, struct brc* entry){
    size_t hash = djb2_hash(pCity);
    if(db[hash]){
        db[hash] = entry;
    }else{
        printf("hash_map_update  cant update at %llu", hash);
        exit(EXIT_FAILURE);
    }
}
*/
struct brc* hash_map_get(const char* pCity){
    //Return pointer or NULL
    size_t hash = djb2_hash(pCity);
    if(db[hash] && !strcmp(db[hash]->pCity, pCity)){
        return db[hash];
    }else{
        return NULL;
    }
}

void print_brc(struct brc* entry){
    if(entry){
        printf("'%s' min:%.2f, max:%.2f, avg:%.2f, sum:%.1f, count:%u\n", 
                entry->pCity, entry->min, entry->max, entry->sum/entry->count,entry->sum, entry->count);
    }else{
        printf("print_brc  null ptr");
        exit(EXIT_FAILURE);
    }
}

void hash_map_dump(){
    int c = 0 ;
    for (size_t i = 0; i < TABLE_SIZE; i++){
        if(db[i]){
            c++;
            print_brc(db[i]);
        }
    }
    printf("Count: %d\n", c);
}

double parser(const char* pStr){
    if(!pStr){
        printf("parser error");
        exit(EXIT_FAILURE);
    }
    double r = 1.0F;
    if(*pStr == '-'){
        r = -1.0F;
        pStr++;
    }
    if(pStr[1]=='.'){
        return (((double)pStr[0] + (double)pStr[2] / 10.0) - 1.1 * '0') * r;
    }else{
        return ((double)((pStr[0]) * 10 + pStr[1]) + (double)pStr[3] / 10.0 - 11.1 * '0') * r;
    }
}

int main(int argc, char const *argv[])
{
    printf("---START---\n");
    //FILE *file = fopen("R:\\100M.txt", "r");
    FILE *file = fopen("C:\\Users\\Kuba\\Documents\\Py\\1brc\\100M.txt", "r");
    if (!file){
        printf("File error");
        return EXIT_FAILURE;
    }
    clock_t start = clock();
    struct brc pBrcT[500];
    int BrcTI = 1;
    char line[64] = {0};
    unsigned int line_count = 0;

    while (fgets(line, 64, file)){
        ++line_count;
        if(line_count%10000000==0) printf("%u\n", line_count/10000000);
        
        char *pos = strchr(line, ';');
        *pos = '\0';
        //double measurement = strtod(pos + 1, NULL);
        double measurement = parser(pos + 1);

        struct brc *pBrc;
        if((pBrc = hash_map_get((const char *)&line))){
            pBrc->count++;
            pBrc->max = measurement>pBrc->max ? measurement : pBrc->max;
            pBrc->min = measurement<pBrc->min ? measurement : pBrc->min;
            pBrc->sum += measurement;
        }else{
            pBrcT[BrcTI].count=1;
            pBrcT[BrcTI].max = measurement;
            pBrcT[BrcTI].min = measurement;
            pBrcT[BrcTI].sum = measurement;
            pBrcT[BrcTI].pCity = strdup(line); //todo free()
            hash_map_add(&pBrcT[BrcTI]);
            BrcTI++;
        }
        if (BrcTI >= 500) {
            //feat of chatgpt
            fprintf(stderr, "Error: pBrcT array is full.\n");
            break;
        }
    }
    hash_map_dump();
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n-------End program--------", time_spent);

    fclose(file);
    return 0;
}