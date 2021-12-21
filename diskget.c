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

char * get_name(char *entry);
int write_file(char *r, FILE *file, unsigned int val, unsigned int size);
int copy_file(char * src, char *end, int file_size);
int get_entry_val(char *r, int val);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Argument invalid.\n");
        exit(EXIT_FAILURE);
    }
    int f = open(argv[1], O_RDWR);
    struct stat sb;
    if(f < 0)
    {
        fprintf(stderr, "Disk invalid: %s.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if(fstat(f, &sb) == -1)
    {
        fprintf(stderr, "File size invalid.\n");
        exit(EXIT_FAILURE);
    }
    char *start = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, f, 0);
    if (start == MAP_FAILED)
    {
        fprintf(stderr, "Memory mapping invalid.\n");
        exit(EXIT_FAILURE);
    }
    char *handle = argv[2];
    while(*handle)
    {
        *handle = toupper((unsigned int) *handle);
        handle++;
    }

    char *name;
    char *entry;
    int count = 1;
    int non = 1;
    handle = start + 19 * 512;
    while(count++ != 14 && handle[0] != 0x00)
    {
        name = get_name(handle);
        if(!strcmp(argv[2], name))
        {
            non--;
            break;
        }
        handle += 32;
    }
    if(non)
    {
        fprintf(stderr, "File not found.\n");
        exit(1);
    }
    
    entry = handle;
    unsigned int entry_f = ((entry[27] & 0x00ff) << 8 | (entry[26] & 0x00ff));
    unsigned int size = ((entry[28] & 0x00ff) | ((entry[29] & 0x00ff) << 8) | ((entry[31] & 0x00ff) << 24) | ((entry[30] & 0x00ff) << 16));

    FILE *f_write = fopen(argv[2], "w");
    if(f_write == NULL)
    {
        fprintf(stderr, "Writing invalid.\n");
        close(f);
        exit(EXIT_FAILURE);
    }

    if(write_file(start, f_write, entry_f, size))
    {
        fprintf(stderr, "Writing is invalid.\n");
        close(f);
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
    return 0;
}

//get the entr value
int get_entry_val(char *r, int val) {
    int hand1;
    int hand2;
    int result;
    if (val % 2 == 0)
    {
        hand1 = r[512 + (val * 3 / 2) + 1] & 0x0F;
        hand2 = r[512 + (val * 3 / 2)] & 0xFF;
        result = (hand1 << 8) | (hand2);
    }
    else
    {
        hand1 = r[512 + (val * 3 / 2)] & 0xF0;
        hand2 = r[512 + (val * 3 / 2) + 1] & 0xFF;
        result = (hand1 >> 4) | (hand2 << 4);
    }
    return result;
}

//write the file
int write_file(char *r, FILE *file, unsigned int val, unsigned int size)
{
    size_t checker;
    do
    {   
        size -= 512;
        char *src = &r[(33 + val - 2) * 512];
        if(size >= 0)
        {checker = fwrite(src, sizeof(char), 512, file);}
        else
        {checker = fwrite(src, sizeof(char), size + 512, file);}
        if(checker == 0)
        {return 1;}
        val = get_entry_val(r, val) & 0xfff;
    } while (val < 0xff8);
    return 0;
}

int copy_file(char * src, char *end, int file_size) {
    return 0;
}

//get the name
char * get_name(char * entry) {   
    int i;
    char * end = malloc(sizeof(char) * 13);
    if(end == NULL)
    {
        fprintf(stderr, "Memory invalid.");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < 8; i++)
    {
        if(entry[i] == ' ')
        {break;}
        end[i] = entry[i];
    }
    end[i++] = '.';

    for(int j = 0; j < 3; j++)
    {
        if(entry[j + 8] == ' ')
        {break;}
        end[i++] = entry[j + 8];
    }
    end[i] = '\0';
    return end;
}