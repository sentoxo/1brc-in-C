/*  gcc main.c -o 1brc -O2
    3.18s best on linux using fgets()
    2.71s best on crude mmap() on linux
    2.54s best on custom copy() instead mecpy/memset
    25.99s best for 1G
    20.4 after 'refactor'
    17.4/1.7(100M) after inlining functions(i was sure that compiler do that auto)
*/
#define NDEBUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#define TABLE_SIZE (1<<12)

struct brc{
    const char* pCity;
    double min, max, sum;
    int count;
};

//Hashmap array
struct brc* db[TABLE_SIZE] = {0};

//Calculate hash directly from mmap file and write to pLen length of hashed word.
inline size_t djb2_hash(const char* pStr, size_t* pLen){
    size_t hash_value = 5381;
    char c;
    *pLen = 0;
    while((c = *pStr++) != ';'){
        *pLen = *pLen + 1;
        hash_value = ((hash_value << 5) + hash_value) + c;
    }
    //*(pStr-1) = '\0';
    return hash_value % TABLE_SIZE;
}

//Add struct brc to hashmap, works lineary.
inline void hash_map_add(struct brc* entry, size_t hash){
    while(db[hash]){
        hash = (hash+1) % TABLE_SIZE;
    }
    db[hash] = entry;
}

//Chechk if hash word is in hashmap, return pointer to struct or NULL.
inline struct brc* hash_map_get(const char* file, size_t hash, size_t len){
    while(db[hash] && strncmp(db[hash]->pCity, file, len)){
        hash = (hash+1) % TABLE_SIZE;
    }
    if(db[hash]){
        return db[hash];
    }else{
        return NULL;
    }
}

//Print brc to console.
inline void print_brc(struct brc* entry){
    if(entry){
        printf("'%s' min:%.2f, max:%.2f, avg:%.2f, sum:%.1f, count:%u\n", 
                entry->pCity, entry->min, entry->max, entry->sum/entry->count,entry->sum, entry->count);
    }else{
        fprintf(stderr, "print_brc  null ptr");
        exit(EXIT_FAILURE);
    }
}

//Dumps all hashmap to console.
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

//Custom parser for floats in range -99.9 to 99.9,
//takes pointer to file and write to len pointer length of the word.
inline double parser(const char* pStr, size_t* len){
    assert(pStr);
    double r = 1.0F;
    if(*pStr == '-'){
        r = -1.0F;
        pStr++;
        *len = *len + 1;
    }
    if(pStr[1]=='.'){
        *len = *len + 3;
        return (((double)pStr[0] + (double)pStr[2] / 10.0) - 1.1 * '0') * r;
    }else{
        *len = *len + 4;
        return ((double)((pStr[0]) * 10 + pStr[1]) + (double)pStr[3] / 10.0 - 11.1 * '0') * r;
    }
}

int main(int argc, char const *argv[])
{
    printf("---START---\n");
    int fd = open("100M.txt", O_RDONLY, S_IRUSR | S_IWUSR);
    struct stat sb;
    if (fstat(fd, &sb) == -1){
        fprintf(stderr, "File error\n");
        return EXIT_FAILURE;
    }
    const char* nmap_file = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    clock_t start = clock();
    struct brc pBrcT[500];
    int BrcTI = 1;
    unsigned int line_count = 0;
    size_t endF = (size_t)nmap_file + sb.st_size;
    while ((size_t)nmap_file < endF){
        #ifndef NDEBUG
        ++line_count;
        if(line_count%10000000==0) printf("%u\n", line_count/10000000);
        #endif
        struct brc *pBrc;
        char* city;
        size_t len;
        size_t hash = djb2_hash(nmap_file, &len);

        pBrc = hash_map_get(nmap_file, hash, len);
        if(!pBrc) {
            city = malloc(len+1);
            strncpy(city, nmap_file, len);
        }
        nmap_file += len+1;
        len = 0;
        double measurement = parser(nmap_file, &len);
        nmap_file += len+1;

        if(pBrc){
            pBrc->count++;
            pBrc->max = measurement>pBrc->max ? measurement : pBrc->max;
            pBrc->min = measurement<pBrc->min ? measurement : pBrc->min;
            pBrc->sum += measurement;
        }else{
            pBrcT[BrcTI].count=1;
            pBrcT[BrcTI].max = measurement;
            pBrcT[BrcTI].min = measurement;
            pBrcT[BrcTI].sum = measurement;
            pBrcT[BrcTI].pCity = city;
            hash_map_add(&pBrcT[BrcTI], hash);
            BrcTI++;
        }
        #ifndef NDEBUG
        if (BrcTI >= 500) {
            fprintf(stderr, "Error: pBrcT array is full.\n");
            break;
        }
        #endif
    }
    hash_map_dump();
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n-------End program--------\n", time_spent);

    return 0;
}