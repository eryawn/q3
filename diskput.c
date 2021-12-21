#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void diskput_entry(char *pointer, char* file);
void set_val_entry(char* pointer,int entry, int val);
int get_val_entry(char* pointer, int byte_ent);
int get_free_fat(char* pointer);
int get_free_size(char *pointer);

int main (int argc, char** argv){
        if(argc!=3){
            printf("Argument invalid.\n");
            exit(0);
        }
        struct stat sb;
        int f = open(argv[1], O_RDWR);
        if(f == -1){
            printf("File not found.\n");
            exit(0);
        }
        if(fstat(f, &sb)  == -1){
            perror("File size invalid.\n");
        }
        char *pointer = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0); // p points to the starting pos of your mapped memory
        diskput_entry(pointer, argv[2]);
        close(f);
        return 0;
}

int get_free_fat(char* pointer){
    for(int i = 0;i < 2880 - 31; i++){
        if(!get_val_entry(pointer, i)){
            return i;
        }
    }
    return -1;
}

//get the entry value
int get_val_entry(char* pointer, int byte_ent){
    int start = 512;
    int buffer_1, buffer_2, ret;
    if (byte_ent % 2 == 0) { //even
        buffer_1 = pointer[start + ((3 * byte_ent) / 2) + 1] & 0x0F;
        buffer_2 = pointer[start + ((3 * byte_ent) / 2)] & 0xFF;
        ret = (buffer_2) | (buffer_1 << 8);
    }else{ // odd
        buffer_1 = pointer[start + ((3 * byte_ent) / 2)] & 0xF0;
        buffer_2 = pointer[start + ((3 * byte_ent) / 2) + 1] & 0xFF;
        ret =  (buffer_2 << 4) | (buffer_1 >> 4);
    }
    return ret;
}

//set the entry value
void set_val_entry(char* pointer,int entry, int val){
        int buffer_1, buffer_2;
        if (entry % 2 == 0) { //even
            buffer_1 = ((val) & 0xFF) | (pointer[512 + ((3 * entry) / 2) + 1] & 0xF0);
            buffer_2 = ((val >> 8) & 0x0F);
            pointer[512 + ((3 * entry) / 2)] = buffer_1;
            pointer[512 + ((3 * entry) / 2) + 1] = buffer_2;
        }
        else {
            buffer_1 = (pointer[512 + ((3 * entry) / 2)] & 0x0F) | ((val << 4) & 0xF0);
            buffer_2 = ((val >> 4) & 0xFF);
            pointer[512 + ((3 * entry) / 2)] = buffer_1;
            pointer[512 + ((3 * entry) / 2) + 1] = buffer_2;
        }
}

void diskput_entry(char *pointer, char* file){
    char* sub_dr[10];
    char* str = file;
    int idx = 0;
    int size_free = get_free_size(pointer);
    while(* str){
        * str = toupper((unsigned char) *str);
        str++;
    }

    char* sub_p = strtok(file,"/");
    while(sub_p!=NULL){

        sub_dr[idx] = sub_p;
        idx++;
        sub_p = strtok(NULL,"/");
    }

    int file_in;
    int end = idx - 1;
    char* ext = sub_dr[end];
    struct stat stat_in;    
    file_in = open(ext, O_RDWR); //retrieve current file
    if(file_in == -1){
        printf("File is not found.\n");
        exit(0);
    }
    if(fstat(file_in, &stat_in)  == -1){
        perror("files size invalid.\n");
    }
    int size_in = stat_in.st_size;
    if(size_in > size_free){
        printf("No enough free space in the disk image.\n");
        exit(0);
    }

    //check file
    char *input = mmap(NULL, size_in, PROT_READ, MAP_SHARED, file_in, 0);
    char* name_file_in = strtok(ext, ".");
    char* extension = strtok(NULL," ");

    
    //path exists
    char* file_name;
    char* dir_name = calloc(1,sizeof(char)*9);
    int attr;
    int n_sub = idx - 1;
    int start = 512 * 19;

    if(n_sub > 0){
        for(int i = 0; i < strlen(sub_dr[0]); i++){
            if(sub_dr[0][i] != ' '){
                if(sub_dr[0][i] != '\0'){
                    dir_name[i] = sub_dr[0][i];
                }else{
                    dir_name[i] = ' ';
                }
            }
            for(int j = strlen(sub_dr[0]); j< 9; j++){
                dir_name[j] = ' ';
            }
        }
        dir_name[8] = '\0';
        int i;
        for(i = 0; i < 16 * 14; i++){
            attr = pointer[start + i * 32 + 11];
            file_name = start + pointer + i * 32;
            if(pointer[start + i * 32] == 0x00){
                printf("Path not found.\n");
                return;
            }  
            if(strncmp(dir_name, file_name, 8) != 0){ 
                continue;
            }else if(   (attr & 0x10) != 0x10 ){ 
                continue;
            }
            break;
        }
        int lg_sec_first = (pointer[start + i * 32 + 26] & 0x00FF) + ((pointer[start + i * 32 + 27] & 0x00FF) << 8);
            
        //traverse subfolders
        for(int i = 1; i < n_sub; i++){ 
            int j;
            for(int s = 0; s < strlen(sub_dr[i]); s++){
                if(sub_dr[i][s] != ' '){
                    if(sub_dr[i][s] != '\0'){
                        dir_name[s] = sub_dr[i][s];
                    }else{
                        dir_name[s] = ' ';
                    }
                }
                for(int s = strlen(sub_dr[i]); s < 9; s++){
                    dir_name[s] = ' ';
                }
            }

            //traverse the entries
            dir_name[8] = '\0';
            for(j = 0; j < 16; j++){  
                file_name = pointer + (31 + lg_sec_first) * 512 + j * 32;
                attr = pointer[(31 + lg_sec_first) * 512 + j * 32 + 11];
                
                //if rest is empty
                if(pointer[(31 + lg_sec_first) * 512 + j * 32] == 0x00){
                    printf("Path not found.\n");
                    return;
                    
                }
                //not this subfolder
                if( strncmp(dir_name, file_name, 8) != 0 ){
                    continue;
                }else if( (attr & 0x10) != 0x10 ){  
                    continue;
                }
                break;
            }
            lg_sec_first = (pointer[512 * (33 + lg_sec_first - 2) + j * 32 + 26] & 0x00FF) + 
                        ((pointer[512 * (33 + lg_sec_first - 2) + j * 32 + 27] & 0x00FF) << 8);
        }
       
        int offset = (lg_sec_first + 31) * 512;
        int free_next = get_free_fat(pointer);
        for(int i = 0; i < 16; i++){ //location of file to put determined
            char *name_byte = offset + pointer +(i * 32);
            int attr = pointer[offset + (i * 32) + 11];
    
            if( !strcmp(name_byte,"") && (!attr) ){
                strncpy(name_byte, name_file_in, 8); //name
                strncpy(name_byte + 8, extension, 3); //extension

                time_t *time = &stat_in.st_ctime;
                struct tm* input_time = localtime(time);
                int year = ((input_time -> tm_year - 80) << 9) & 0xFE00;
                int month = ((input_time -> tm_mon + 1) << 5) & 0x01E0;
                int day = input_time -> tm_mday & 0x001F;
                int hour = ((input_time -> tm_hour) << 11) & 0xFE00; 
                int minute = ((input_time -> tm_min) << 5) & 0x07E0; 
                int second = (input_time -> tm_sec) & 0x001F; 
               
                //save info
                pointer[offset + i * 32 + 26] = free_next & 0xFF;
                pointer[offset + i * 32 + 28 +1] = (size_in >> 8) & 0xFF;
                pointer[offset + i * 32 + 28 + 2] = (size_in >> 16) & 0xFF;
                pointer[offset + i * 32 + 26 + 1] = (free_next >>8 )& 0xFF;
                pointer[offset + i * 3 + 28] = size_in & 0xFF;
                pointer[offset+ i * 32 + 28 + 3] = (size_in >> 24) & 0xFF;
                pointer[offset + i * 32 + 14] = ( hour | minute | second) & 0xFF;
                pointer[offset + i * 32 + 15] = ((hour | minute | second)>>8) & 0xFF;
                pointer[offset + i * 32 + 16] = ( year | month | day) & 0xFF;
                pointer[offset + i * 32 + 17] = ((year | month | day)>>8) & 0xFF;
                break;
            }
        }
            //copy content
            long int file_in_count = 0;
            int l_bytes = size_in;
            int bytes_w;
            do{
                int tmp = free_next;
                if(l_bytes > 512){
                    bytes_w = 512;
                    free_next = get_free_fat(pointer);
                }else{
                    bytes_w = l_bytes;
                    free_next = 0xFFF;
                }
                set_val_entry(pointer, tmp, free_next);

                for(int i = 0; i < bytes_w; i++){
                    pointer[(tmp + 31) * 512 + i] = input[i + file_in_count * 512];

                }
                l_bytes -= bytes_w;
                file_in_count += 1;
            }while(l_bytes > 0);
        return;
    }

    //file copied to root
    int free_next = get_free_fat(pointer);
    for(int i = 0; i < 14 * 16; i++){
        char *name_byte = start + pointer + (i * 32);
        int attr = pointer[start + (i * 32) + 11];
        
        if( !strcmp(name_byte, "") && (!attr) ){
            strncpy(name_byte, name_file_in, 8);
            strncpy(name_byte + 8, extension, 3);

            time_t *time = &stat_in.st_ctime;
            struct tm* input_time = localtime(time);            
            int year = ((input_time -> tm_year - 80) << 9) & 0xFE00;
            int month = ((input_time -> tm_mon + 1) << 5) & 0x01E0;   
            int day = input_time -> tm_mday & 0x001F;
            int hour = ((input_time -> tm_hour) << 11) & 0xFE00; 
            int minute = ((input_time -> tm_min) << 5) & 0x07E0; 
            int second = (input_time -> tm_sec) & 0x001F; 
           
            //save info
            pointer[start + i * 32 + 26] = free_next & 0xFF;
            pointer[start + (i * 32) + 28 + 1] = (size_in >> 8) & 0xFF;
            pointer[start + i * 32 + 26 + 1] = (free_next >>8 )& 0xFF;
            pointer[start + (i * 32) +28] = size_in & 0xFF;
            pointer[start + (i * 32) + 28 + 2] = (size_in >> 16) & 0xFF;
            pointer[start + (i * 32) + 28 + 3] = (size_in >> 24) & 0xFF;
            pointer[start + i * 32 + 14] = ( hour | minute | second) & 0xFF;
            pointer[start + i * 32 + 15] = ((hour | minute | second) >> 8) & 0xFF;
            pointer[start + i * 32 + 16] = ( year | month | day) & 0xFF;
            pointer[start + i * 32 + 17] = ((year | month | day) >> 8) & 0xFF;
            break;
        }
    }
        //copy content
        long int file_in_count = 0;
        int l_bytes = size_in;
        int bytes_w;

        do{
            int tmp = free_next;
            if(l_bytes > 512){
                bytes_w = 512;
                free_next = get_free_fat(pointer);
            }else{
                bytes_w = l_bytes;
                free_next = 0xFFF;
            }
            set_val_entry(pointer, tmp, free_next);

            for(int i = 0; i < bytes_w; i++){
                pointer[(tmp + 31) * 512 + i] = input[i + file_in_count * 512];

            }
            l_bytes -= bytes_w;
            file_in_count += 1;
        }while(l_bytes > 0);
    return;
}

//get the free size in the image
int get_free_size(char *pointer){
    int val;
    int free_sec = 0;
    int start = 512;
    unsigned long int size_free;
    int buffer_1,buffer_2;
    for (int i = 0; i < 2880-31; i++){
        if (i % 2 == 0) { //reserved secs
            buffer_1 = pointer[start + ((3 * i) / 2) + 1] & 0x0F;
            buffer_2 = pointer[start + ((3 * i) / 2)] & 0xFF;
            val = buffer_2 | (buffer_1 << 8);
        } else{
            buffer_1 = pointer[start + ((3 * i) / 2)] & 0xF0;
            buffer_2 = pointer[start + ((3 * i) / 2) + 1] & 0xFF;
            val =  (buffer_2 << 4) | (buffer_1 >> 4);
        }
        if(val == 0x000){
            free_sec++;
        }
    }
    size_free = free_sec*512;
    return size_free;
}
