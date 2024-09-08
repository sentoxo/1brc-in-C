/*  gcc main.c -o main.exe ; & 'c:\Users\Kuba\Documents\GitHub\1brc in C\main.exe'
    3.18s best on linux using fgets()
    2.71s best on crude mmap() on linux
    2.54s best on custom copy() instead mecpy/memset
    25.99s best for 1G
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define TABLE_SIZE (1<<12)

struct brc{
    const char* pCity;
    double min, max, sum;
    int count;
};

struct brc* db[TABLE_SIZE] = {0};

size_t djb2_hash(char* pStr, unsigned int* pLen){
    size_t hash_value = 5381;
    char c;
    *pLen = 0;
    while((c = *pStr++) != ';'){
        *pLen = *pLen + 1;
        hash_value = ((hash_value << 5) + hash_value) + c;
    }
    *(pStr-1) = '\0';
    return hash_value % TABLE_SIZE;
}

size_t djb2_hash_add(const char* pStr){
    size_t hash_value = 5381;
    char c;
    while((c = *pStr++)){
        hash_value = ((hash_value << 5) + hash_value) + c;
    }
    return hash_value % TABLE_SIZE;
}

void hash_map_add(struct brc* entry){
    size_t hash = djb2_hash_add(entry->pCity);
    while(db[hash]){
        hash = (hash+1) % TABLE_SIZE;
    }
    db[hash] = entry;
}

struct brc* hash_map_get(const char* pCity, unsigned int* pLen){
    //Return pointer or NULL
    size_t hash = djb2_hash((char*)pCity, pLen);
    while(db[hash] && strcmp(db[hash]->pCity, pCity)){
        hash = (hash+1) % TABLE_SIZE;
    }
    if(db[hash]){
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
        fprintf(stderr, "print_brc  null ptr");
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
    /*
    if(!pStr){
        fprintf(stderr, "parser error");
        exit(EXIT_FAILURE);
    }*/
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

void copy(char* dest, const char* src, size_t len){
    size_t i = 0;
    for(; i < len; i++){
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

int main(int argc, char const *argv[])
{
    printf("---START---\n");
    int fd = open("1G.txt", O_RDONLY, S_IRUSR | S_IWUSR);
    struct stat sb;
    if (fstat(fd, &sb) == -1){
        fprintf(stderr, "File error\n");
        return EXIT_FAILURE;
    }
    char* nmap_file = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    
    size_t counter2 = 0;

    clock_t start = clock();
    struct brc pBrcT[500];
    int BrcTI = 1;
    char line[50] = {0};
    unsigned int line_count = 0;
    size_t iL = 0;
    while (iL < sb.st_size){
        for ( ;iL < sb.st_size; iL++){
            if (nmap_file[iL] == '\n'){
                iL++;
                break;
            }
        }
        //memset(&line, 0, 50);
        //memcpy(&line, &nmap_file[counter2], iL - counter2);
        copy((char*)&line, &nmap_file[counter2], iL - counter2);
        line[iL  - counter2 - 1U] = '\0';
        counter2 = iL ;

        //++line_count;
        //if(line_count%10000000==0) printf("%u\n", line_count/10000000);
        
        unsigned int len;
        struct brc *pBrc;
        if((pBrc = hash_map_get((const char *)&line, &len))){
            double measurement = parser((const char*)&line[len+1]);
            pBrc->count++;
            pBrc->max = measurement>pBrc->max ? measurement : pBrc->max;
            pBrc->min = measurement<pBrc->min ? measurement : pBrc->min;
            pBrc->sum += measurement;
        }else{
            double measurement = parser((const char*)&line[len+1]);
            pBrcT[BrcTI].count=1;
            pBrcT[BrcTI].max = measurement;
            pBrcT[BrcTI].min = measurement;
            pBrcT[BrcTI].sum = measurement;
            pBrcT[BrcTI].pCity = strdup(line); //todo free()
            hash_map_add(&pBrcT[BrcTI]);
            BrcTI++;
        }
        if (BrcTI >= 500) {
            fprintf(stderr, "Error: pBrcT array is full.\n");
            break;
        }
    }
    hash_map_dump();
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n-------End program--------\n", time_spent);

    return 0;
}