#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int COUNT_G = 0;
unsigned long int TOTAL_SIZE;
unsigned long int FREE_SIZE;

void count_files(char *pointer);
void diskinfo_entry (char* pointer);
void sub_files(char* pointer, int ent_n);

int main(int argc, char *argv[])
{
    if(argc!=2){
        printf("Argument invalid.\n");
        exit(0);
    }
    int fd;
    struct stat sb;
    fd = open(argv[1], O_RDWR);
    if(fstat(fd, &sb)  == -1){
        perror("File size invalid.\n");
    }
    char * pointer = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    diskinfo_entry(pointer);
    close(fd);
    return 0;
}

//count number of files
void count_files(char *pointer){
    int byte_n; 
    int attr;
    int count = -1;
    int sub[224];
    int idx = 0;
    unsigned long int sec_byte = ( pointer[12] << 8 | pointer[11]);
    int sec_fat = (pointer[23] << 8 | pointer[22]);
    int sec_res = (pointer[15] << 8 | pointer[14]);
    int n_fat = (pointer[16]);
    int start = (n_fat * sec_fat + sec_res) * sec_byte;   
    byte_n = pointer[start];
    
    while( count < 16 * 14 && byte_n != 0x00)
    { //traverse root
        count++;
        byte_n = pointer[start + 32 * (count + 1)];
        attr = pointer[start + count * 32 + 11];
        if(attr != 0x08 && byte_n != 0xE5 && attr != 0x0F){ 
            if (attr == 0x10){  //identify sub folders
                sub[idx] = count;
                idx++;
            }else{  
                COUNT_G++;
            }
        }
    }
    
    for(int i = 0; i < idx; i++){ //dig into subfolders
        count = sub[i];
        int cluster = ((pointer[start + 27 + count * 32] & 0x00FF) << 8) + (pointer[start + 26 + count * 32] & 0x00FF);
        sub_files(pointer, cluster);
    }
}

void diskinfo_entry (char* pointer){
    unsigned long int sec_byte = (pointer[12] << 8 | pointer[11]);
    unsigned long int n_sec = (pointer[20] << 8 | pointer[19]);
    int sec_fat = (pointer[23] << 8 | pointer[22]);
    int sec_res = (pointer[14] | pointer[15]<<8);
    int n_fat = (pointer[16]);
    int start = (sec_res + sec_fat * n_fat) * sec_byte; 
    int non = 1;
    int val;
    int buffer, buffer1;
    int sec_free = 0;
    int start_fat = sec_byte;
    char * OS_n = (char *)malloc(sizeof(char*));
    char *label = (char*) malloc(sizeof(char*));

    for(int i = 0; i < 8; i++){ //OS name
        OS_n[i] = pointer[i+3];
    }
    for(int i = 0; i < 244; i++){ // label name
        int attr = pointer[start + i * 32 + 0x0b];
        if(attr == 0x08){
            non = 0;
            char* idx = pointer + start + (32 * i);
            for(int i = 0; i < 8; i++){
                label[i] = *idx;
                idx++;
            }
            break;
        }
    }
    if(non == 1){
        label = "undefined";
    }

    for (int i = 0; i < n_sec - 31; i++){ //count free size
        if (i % 2 == 0) {
                buffer = pointer[start_fat + ((3 * i) / 2) + 1] & 0x0F;
                buffer1 = pointer[start_fat + ((3 * i) / 2)] & 0xFF;
                val = (buffer1) | (buffer << 8);
            } else {
                buffer = pointer[start_fat + ((3 * i) / 2)] & 0xF0;
                buffer1 = pointer[start_fat + ((3 * i) / 2) + 1] & 0xFF;
                val =  (buffer1 << 4) | (buffer >> 4);
            }
        if(val == 0x000){
            sec_free++;
        }
    }

    printf("OS Name: %s\n",OS_n);
    printf("Label of the disk: %s\n", label);
    TOTAL_SIZE = n_sec * sec_byte;
    FREE_SIZE = sec_byte * sec_free;
    printf("Total size of the disk: %lu bytes\n", TOTAL_SIZE);
    printf("Free size of the disk: %lu bytes\n\n", FREE_SIZE);
    printf("==============\n");
    count_files(pointer);
    printf("The number of files in the disk: %d\n\n", COUNT_G);
    printf("==============\n");
    printf("Number of FAT copies: %d\n", n_fat);
    printf("Sectors per FAT: %d\n", sec_fat);
    return;
}

//count the number of files in all subfolders
void sub_files(char* pointer, int ent_n){
    int attr; 
    int idx = 0;
    int base = (33 + ent_n - 2) * 512;
    int byte_n = pointer[base];
    int sub[16];

    for(int i = 0; i < 16; i++){ //traverse entries
        byte_n = pointer[base + i * 32];
        attr = pointer[base + i * 32 + 11];
        if(byte_n == 0x00){
            break;
        }else if((attr&0x0E) !=0 || attr == 0x0F || byte_n == 0xE5 || byte_n == '.'){
            continue;
        }
        if (attr == 0x10){ //identify the subfolders
            sub[idx] = i;
            idx++;
        }else{
            COUNT_G++;
        }
    }

    for (int i = 0; i < idx; i++){ //dig into the subfolders
        int hand = sub[i];
        int cluster;
        cluster = ((pointer[base + 27 + hand * 32] & 0x00FF) << 8) + (pointer[base + 26 + hand * 32] & 0x00FF);
        sub_files(pointer, cluster);
    }
}
