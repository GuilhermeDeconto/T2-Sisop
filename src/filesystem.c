#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define BLOCK_SIZE 1024
#define BLOCKS 2048
#define FAT_SIZE (BLOCKS * 2)
#define FAT_BLOCKS (FAT_SIZE / BLOCK_SIZE)
#define ROOT_BLOCK FAT_BLOCKS
#define DIR_ENTRY_SIZE 32
#define DIR_ENTRIES (BLOCK_SIZE / DIR_ENTRY_SIZE)
#define FILE_DAT "filesystem.dat"

/* directory entry */
typedef struct dir_entry_s {
    int8_t filename[25];
    int8_t attributes;
    int16_t first_block;
    int32_t size;
} dir_entry_s;

union{
    struct dir_entry_s dir[FAT_SIZE / sizeof(dir_entry_s)];
    uint8_t data[FAT_SIZE];
} data_cluster;

/* FAT data structure */
int16_t fat[BLOCKS];
/* data block */
int8_t data_block[BLOCK_SIZE];
/* size diretory root */
struct dir_entry_s root_dir[DIR_ENTRY_SIZE];

/*Functions declarations*/
void init(void);
void read_block(char *file, int32_t block, int8_t *record);
void write_block(char *file, int32_t block, int8_t *record);
void read_fat(char *file, int16_t *fat);
void write_fat(char *file, int16_t *fat);
void read_dir_entry(int32_t block, int32_t entry, struct dir_entry_s *dir_entry);
void write_dir_entry(int block, int entry, struct dir_entry_s *dir_entry);
void copy_str(char *target, char *src);

void init(void)
{
    FILE *f;
    int32_t i;
    struct dir_entry_s dir_entry;
    
    /* create filesystem.dat if it doesn't exist */
    f = fopen(FILE_DAT, "a");
    
    /* initialize the FAT */
    for (i = 0; i < FAT_BLOCKS; i++)
        fat[i] = 0x7ffe;
    fat[ROOT_BLOCK] = 0x7fff;
    for (i = ROOT_BLOCK + 1; i < BLOCKS; i++)
        fat[i] = 0;
    /* write it to disk */
    write_fat(FILE_DAT, fat);
    
    /* initialize an empty data block */
    for (i = 0; i < BLOCK_SIZE; i++)
        data_block[i] = 0;
    
    /* write an empty ROOT directory block */
    write_block(FILE_DAT, ROOT_BLOCK, data_block);
    
    /* write the remaining data blocks to disk */
    for (i = ROOT_BLOCK + 1; i < BLOCKS; i++)
        write_block(FILE_DAT, i, data_block);
    /* fill three root directory entries and list them */
    memset((char *)dir_entry.filename, 0, sizeof(struct dir_entry_s));
    strcpy((char *)dir_entry.filename, "file1");
    dir_entry.attributes = 0x01;
    dir_entry.first_block = 1111;
    dir_entry.size = 222;
    write_dir_entry(ROOT_BLOCK, 0, &dir_entry);
    
    memset((char *)dir_entry.filename, 0, sizeof(struct dir_entry_s));
    strcpy((char *)dir_entry.filename, "file2");
    dir_entry.attributes = 0x01;
    dir_entry.first_block = 2222;
    dir_entry.size = 333;
    write_dir_entry(ROOT_BLOCK, 1, &dir_entry);
    
    memset((char *)dir_entry.filename, 0, sizeof(struct dir_entry_s));
    strcpy((char *)dir_entry.filename, "file1");
    dir_entry.attributes = 0x01;
    dir_entry.first_block = 3333;
    dir_entry.size = 444;
    write_dir_entry(ROOT_BLOCK, 2, &dir_entry);
    
    /* list entries from the root directory */
    // for (i = 0; i < DIR_ENTRIES; i++)
    // {
    // 	read_dir_entry(ROOT_BLOCK, i, &dir_entry);
    // 	printf("Entry %d, file: %s attr: %d first: %d size: %d\n", i, dir_entry.filename, dir_entry.attributes, dir_entry.first_block, dir_entry.size);
    // }
    
    fclose(f);
}

/* reads a data block from disk */
void read_block(char *file, int32_t block, int8_t *record)
{
    FILE *f;
    
    f = fopen(file, "r+");
    fseek(f, block * BLOCK_SIZE, SEEK_SET);
    fread(record, 1, BLOCK_SIZE, f);
    fclose(f);
}

/* writes a data block to disk */
void write_block(char *file, int32_t block, int8_t *record)
{
    FILE *f;
    
    f = fopen(file, "r+");
    fseek(f, block * BLOCK_SIZE, SEEK_SET);
    fwrite(record, 1, BLOCK_SIZE, f);
    fclose(f);
}

/* reads the FAT from disk */
void read_fat(char *file, int16_t *fat)
{
    FILE *f;
    
    f = fopen(file, "r+");
    fseek(f, 0, SEEK_SET);
    fread(fat, 2, BLOCKS, f);
    fclose(f);
}

/* writes the FAT to disk */
void write_fat(char *file, int16_t *fat)
{
    FILE *f;
    
    f = fopen(file, "r+");
    fseek(f, 0, SEEK_SET);
    fwrite(fat, 2, BLOCKS, f);
    fclose(f);
}

/* reads a directory entry from a directory */
void read_dir_entry(int32_t block, int32_t entry, struct dir_entry_s *dir_entry)
{
    read_block(FILE_DAT, block, data_block);
    memcpy(dir_entry, &data_block[entry * sizeof(struct dir_entry_s)], sizeof(struct dir_entry_s));
}

/* writes a directory entry in a directory */
void write_dir_entry(int block, int entry, struct dir_entry_s *dir_entry)
{
    read_block(FILE_DAT, block, data_block);
    memcpy(&data_block[entry * sizeof(struct dir_entry_s)], dir_entry, sizeof(struct dir_entry_s));
    write_block(FILE_DAT, block, data_block);
}

/* copy string from src to target */
void copy_str(char *target, char *src)
{
    int len = strlen(src);
    int i;
    for (i = 0; i < len; ++i)
    {
        target[i] = src[i];
    }
}

//void mymkdir(const char *dir)
//{
//	char tmp[256];
//	char *p = NULL;
//	size_t len;
//
//	snprintf(tmp, sizeof(tmp), "%s", dir);
//	len = strlen(tmp);
//	if (tmp[len - 1] == '/')
//		tmp[len - 1] = 0;
//	for (p = tmp + 1; *p; p++)
//		if (*p == '/')
//		{
//			*p = 0;
//			mkdir(tmp);
//			*p = '/';
//		}
//	mkdir(tmp);
//}

uint8_t find_empty_clusters(int n,uint16_t* locations)
{
    uint16_t i = 0x000A;
    uint16_t count = 0x0000;
    for(i = 0x000A; i < 0x1000; i++){
        if(fat[i] == 0x0000){
            locations[count] = i;
            count+= 0x0001;
            if(count == n)break;
        }
    }
    if(count == n) return 0x01;
    return 0x00;
}

int mymkdir(const int arg_count,char** arg_value){
    uint16_t fat_entry = 0x0000;
    uint16_t fat_parent_entry = 0x0000;
    uint8_t dir_index = 0x00;
    uint8_t dir_name[18];
    memset(dir_name,0x0000,18*sizeof(uint8_t));
    if(arg_count > 1){
        uint8_t rc = walk_path(arg_value[1],&fat_entry,&fat_parent_entry,&dir_index,dir_name);
        
        if(data_cluster.dir[dir_index].first_block != 0x0000){
            return 0;
        }
        
        uint16_t empty_index = 0x0000;
        rc = find_empty_clusters(1,&empty_index);
        
        if(rc == 0x00){
            return 0;
        }
        if(dir_index < 0x20){
            dir_entry_s new_dir = {
                .filename = "",
                .attributes=0x01,
                .first_block = empty_index,
                .size = 0x00000000
            };
            memcpy(new_dir.filename,dir_name,sizeof(char) * 18);
            fat[empty_index] = 0xffff;
            write_fat("f",empty_index);
            if(rc == 0x00){
                return 0;
            }
            data_cluster.dir[dir_index] = new_dir;
            write_block("f",0x00,fat_entry);
        }else{
            return 0;
        }
    }
    return 1;
}

void command(void)
{
    char name[4096] = { 0 };
    char name2[4096] = { 0 };
    char nameCopy[4096] = { 0 };
    const char aux[2] = "/";
    char aux2[4096] = { 0 };
    
    char *token;
    int i;
    
    fgets(name,4096,stdin);
    
    strcpy(nameCopy,name);
    
    token = strtok(name,aux);
    
    if ( strcmp(token, "init\n") == 0)
    {
        printf("\nComando -> %s\n", token);
        init();
    }
    else if ( strcmp(token, "mkdir ") == 0 && nameCopy[6] == '/')
    {
        printf("\nComando -> %s\n", token);
        for(i = 6; i < strlen(nameCopy)-1; ++i)
        {
            aux2[i-6] = nameCopy[i];
        }
        printf("\nDiretorio: %s\n", aux2);
        //		mymkdir("./hello");
    }
}

int main(void)
{
    init();
    
    while (1)
    {
        command();
    }
    
    return 0;
}
