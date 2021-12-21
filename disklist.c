#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


char* get_file_n(char * file_entry);
int sec_byte;
void print_date_time(char * directory_entry_startPos);
void apply_file(char *entry, char *name, char *type, int size);
char* apply_folder(char *parent_folder, char *folder_n);
int traverse_file(char* r, char *folder_f, char* dir, int len);

int main(int argc, char *argv[])
{
    if (argc != 2)
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
    
    //print root
    sec_byte = (unsigned int) (start[0x0B] | start[0x0B + 1] << 8);
    apply_folder("", "Root");
    traverse_file(start, start + 19 * sec_byte, "/", 14);
}

void print_date_time(char * directory_entry_startPos)
{
    int time, date;
    int hours, minutes, day, month, year;
    time = *(unsigned short *)(directory_entry_startPos + 14);
    date = *(unsigned short *)(directory_entry_startPos + 16);
    year = ((date & 0xFE00) >> 9) + 1980;
    month = (date & 0x1E0) >> 5;
    day = (date & 0x1F);
    printf("%d-%02d-%02d ", year, month, day);
    hours = (time & 0xF800) >> 11;
    minutes = (time & 0x7E0) >> 5;
    printf("%02d:%02d\n", hours, minutes);
    return ;
}

//get the file name
char * get_file_n(char* entry)
{
    int i;
    char *end = malloc(sizeof(char) * 13);
    if(end == NULL)
    {
        fprintf(stderr, "Memory invalid.\n");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < 8; i++)
    {
        if(entry[i] == ' ')
        { break;}
        end[i] = entry[i];
    }

    if((entry[11] & 0x10) == 0x10) {
        return end;
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

//traverse the file entry
int traverse_file(char* r, char* folder_f, char *dir, int len)
{
    int i = 0;
    int is_folder, is_hidden;
    int is_vol, is_dot;
    int list_val[len];
    int count = 0;
    char *list_folder_n[len];
    char *type; 
    char *name;
    unsigned int length;
    unsigned int cluster;
    
    while(folder_f[0] != 0x00 && i < len)
    {
        is_vol = (folder_f[11] & 0x08) == 0x08;
        is_dot = folder_f[0] == '.';
        is_folder = (folder_f[11] & 0x10) == 0x10;
        is_hidden = (folder_f[11] & 0x02) == 0x02;
        cluster = (folder_f[27] & 0x00ff) << 8 | (folder_f[26] & 0x00ff);
        if(is_hidden || is_vol || is_dot || cluster <= 1)
            goto end;

        name = get_file_n(folder_f);
        length = (((folder_f[30] & 0x00ff) << 16) | ((folder_f[31] & 0x00ff) << 24) | (folder_f[28] & 0x00ff) | ((folder_f[29] & 0x00ff) << 8));
        if(is_folder)
        {
            list_folder_n[count] = name;
            list_val[count] = cluster;
            count++;
            type = "D"; //folder
        }
        else if(!(is_vol && is_dot))
        {
            type = "F"; //file
        }
        apply_file(folder_f, name, type, length);
        end:
        folder_f += 32;
    }

    //dig into the subfolder
    for(int i = 0; i < count; i++)
    {
        char *hand1 = &r[(list_val[i] + 31) * 512];
        char *hand2 = apply_folder(dir, list_folder_n[i]);
        traverse_file(r, hand1, hand2, 16);
    }
    return 0;
}

//print the folder info
char * apply_folder(char *parent_folder, char *folder_n)
{
    int size_f = strlen(folder_n);
    int size_p = strlen(parent_folder);
    int size = size_p + size_f;
    char *s = malloc(size * sizeof(char) + 2);
    strncat(s, parent_folder, size_p);
    strncat(s, folder_n, size_f);
    s[size + 1] = '\0';
    printf("%s", s);
    printf("\n==================\n");
    s[size] = '/';
    return s;
}

//print the file info
void apply_file(char *entry, char *name, char *type, int size)
{
    printf("%s %10d %20s ", type, size, name);
    print_date_time(entry);
}
