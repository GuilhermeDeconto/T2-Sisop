#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define BLOCK_SIZE 1024
#define BLOCKS 2048
#define FILE_DAT "filesystem.dat"

/* directory entry */
typedef struct{
    uint8_t filename[25];
    uint8_t attributes;
    uint16_t first_block;
    uint32_t size;
} dir_entry_s;

union data_blocks{
    /* size diretory root */
    dir_entry_s root_dir[BLOCK_SIZE / sizeof (dir_entry_s)];
    uint8_t data[BLOCK_SIZE];
};

/* FAT data structure */
int16_t fat[BLOCKS];

/*Functions declarations*/
void init();
union data_blocks read_block(int block);
void write_block(int block,union data_blocks *record);
void write_fat();
void copy_str(char *target, char *src);

/* reads a data block from disk */
union data_blocks read_block(int block)
{
    FILE *f;
    union data_blocks data_block;
    
    f = fopen(FILE_DAT, "rb");
    fseek(f, block * BLOCK_SIZE, SEEK_SET);
    fread(&data_block,sizeof(union data_blocks),1,f);
    fclose(f);
    
    return data_block;
}

/* writes a data block to disk */
void write_block(int block, union data_blocks *record)
{
    FILE *f;
    
    f = fopen(FILE_DAT, "rb+");
    fseek(f, (block * BLOCK_SIZE), SEEK_SET);
    fwrite(record, BLOCK_SIZE,1, f);
    fclose(f);
}

/* writes the FAT to disk */
void write_fat()
{
    FILE *f;
    
    f = fopen(FILE_DAT, "rb+");
    fseek(f, (BLOCK_SIZE), SEEK_SET);
    fwrite(&fat, BLOCKS, 2, f);
    fclose(f);
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

void slicestr(char * str, char * buffer, int start, int end)
{
    int j = 0;
    for ( int i = start; i < end; i++ ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}

int findFreeSpaceFat(){
    int i;
    for (i = 0; i < BLOCKS; i++){
        if (fat[i] == 0){
            return i;
        }
    }
    
    return -1;
}

void resize(char* directories, size_t extend_size){

    char dir_copy[strlen(directories)];
    strcpy(dir_copy,directories);

    char *cpy = malloc(strlen(directories)*sizeof(char));
    strcpy(cpy, directories);

    char * token;
    token = strtok(cpy,"/"); // pega o primeiro elemento apos root
    
    int index_block_fat = 0;

    union data_blocks block;

    if (directories[0] == '/'){
        index_block_fat = 5;
        block = read_block(5);
    }else{
        printf("Caminho não possui diretório root!\n");
        return;
    }
    
    int count = 0;

    // conta quantos direitorios há na string
    while(token != NULL){
        token = strtok(NULL,"/");
        count++;
    }

    token = strtok(dir_copy,"/");

    // caminha nos diretórios até chegar no penúltimo,
    //  pois o último é o arquivo que deve ser utilizado
    while( count > 1) {
        int i;
        int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
        int found_dir = 0;

        // procura o diretorio atual no anterior
        for (i = 0; i < size_dir; i ++){

            if (strcmp(block.root_dir[i].filename,token) == 0){
                found_dir = 1;
                if(block.root_dir[i].attributes == 1){

                    block.root_dir[i].size += extend_size;
                    write_block(index_block_fat,&block);
                    block = read_block(block.root_dir[i].first_block);
                    index_block_fat = block.root_dir[i].first_block;

                }else{
                    printf("%s não é um diretório!\n",token);
                }
                break;
            }
        }

        if (!found_dir){
            printf("Não existe este diretório %s\n",token);
            return;
        }

        token = strtok(NULL,"/");
        count--;
    }
}

void init()
{
    FILE *f;
    int i;
    union data_blocks boot_block;
    
    /* create filesystem.dat if it doesn't exist */
    f = fopen(FILE_DAT, "wb");
    
    // alocar endereço fat para a propria fat
    for (i = 0; i <= 4; i++){
        fat[i] = 0x7ffe;
    }
    
    // alocar endereço na fat para root block
    fat[5] = 0xffff;
    
    // para o restante dos endereços na fat
    for (i = 6; i < BLOCKS; i++){
        fat[i] = 0x0000;
    }
    
    union data_blocks blocks;
    
    fwrite(&boot_block, BLOCK_SIZE, 1, f);
    fwrite(&fat, sizeof(fat), 1, f);
    
    for (i = 0; i < 2048; i++){
        memset(&blocks,0x0000,BLOCK_SIZE);
        memset(&blocks.data,0x0000,BLOCK_SIZE);
        fwrite(&blocks, BLOCK_SIZE, 1, f);
    }
    fclose(f);
}

void load (){
    FILE *f;
    
    f=fopen(FILE_DAT,"rb");
    
    if (!f){
        printf("Impossivel abrir o arquivo!\n");
        return;
    }
    
    union data_blocks boot_block;
    
    //carrega o boot_block para a memoria
    fread(&boot_block,BLOCK_SIZE,1,f);
    int i;
    
    //carrega a fat para a memoria
    fread(&fat, sizeof(fat), 1, f);
    
    fclose(f);
}

void ls (char* directories){
    char * token;
    
    char *cpy = malloc(strlen(directories)*sizeof(char));
    strcpy(cpy, directories);
    
    token = strtok(cpy,"/"); // pega o primeiro elemento apos root
    
    union data_blocks block;
    
    if (directories[0] == '/'){
        block = read_block(5);
    }else{
        printf("Caminho não possui diretório root!\n");
        return;
    }
    
    // procura o diretorio atual no anterior, se encontrar
    // então pode-se procurar o proximo diretorio neste de agora
    // acontece isso até chegar no último diretório que no final
    // teremos o os diretorios / arquivos deste diretório.
    
    while( token != NULL ) {
        int i;
        int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
        int found_dir = 0;
        
        // procura o diretorio atual no anterior
        for (i = 0; i < size_dir; i ++){

			if (strcmp(block.root_dir[i].filename,token) == 0){
				found_dir = 1;
				if(block.root_dir[i].attributes == 1){
					block = read_block(block.root_dir[i].first_block);
				}else{
					printf("%s não é um diretório!\n",token);
				}
				break;
			}
		}
		if (!found_dir){
			printf("Não existe este diretório %s\n",token);
		}

		token = strtok(NULL, "/");
	}

    // block possui agora o bloco do último diretório
    
    int i;
	int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
	printf("\n");
	for (i = 0; i < size_dir; i ++){
		if(block.root_dir[i].first_block != 0){
			printf("%s \n",block.root_dir[i].filename);
		}
	}
	free(cpy);
}

void mkdir(char* directories){
	char dir_copy[strlen(directories)];
	strcpy(dir_copy,directories);

	char *cpy = malloc(strlen(directories)*sizeof(char)); 
	strcpy(cpy, directories);

	char * token;
	token = strtok(cpy,"/"); // pega o primeiro elemento apos root
	
	int index_block_fat = 0;

	union data_blocks block;

	if (directories[0] == '/'){
		index_block_fat = 5;
		block = read_block(5);
	}else{
		printf("Caminho não possui diretório root!\n");
		return;
	}
	
	int count = 0;

	// conta quantos direitorios há na string
	while(token != NULL){
		token = strtok(NULL,"/"); 
		count++;
	}

	token = strtok(dir_copy,"/");

	// caminha nos diretórios até chegar no penúltimo,
    //  pois o último é o arquivo que deve ser criado
	while( count > 1){;
		int i;
		int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
		int found_dir = 0;

		// procura o diretorio atual no anterior
		for (i = 0; i < size_dir; i ++){

			if (strcmp(block.root_dir[i].filename,token) == 0){
				found_dir = 1;
				if(block.root_dir[i].attributes == 1){
					index_block_fat = block.root_dir[i].first_block;
					block = read_block(block.root_dir[i].first_block);
				}else{
					printf("%s não é um diretório!\n",token);
				}
				break;
			}
		}

		if (!found_dir){
			printf("Não existe este diretório %s\n",token);
			return;
		}

		token = strtok(NULL,"/"); 
		count--;
	}

	int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
	int i;

	// tendo o diretorio no qual queremos criar o novo (token)
	// basta verificar se nao existe um arquivo com este mesmo nome
	// verificar se possui um bloco livre no diretório e na fat
	for (i = 0; i < size_dir; i++){

		if (strcmp(block.root_dir[i].filename, token) == 0){
			printf("Já possui uma entrada neste diretório com este mesmo nome!\n");
				return;
		}

		if (block.root_dir[i].first_block == 0){
			
			int index_fat = findFreeSpaceFat();

			if(index_fat == -1 ){
				printf("Fat não possui espaço vazio!\n");
				return;
			}

			fat[index_fat] = 0xffff;
			dir_entry_s new_dir;
			// limpa o novo diretorio a ser criado (apaga os lixos da memoria)

			memset(&new_dir,0x0000,sizeof(dir_entry_s));
			memcpy(new_dir.filename,token,sizeof(char) * strlen(token));
			new_dir.attributes = 1;
			new_dir.first_block = index_fat;
			new_dir.size = 0;

			// salva este novo diretorio no bloco do pai
			block.root_dir[i] = new_dir;
			write_block(index_block_fat,&block);
			write_fat();
			break;
		}
	}
	
	free(cpy);
}

void create(char* directories){
    char dir_copy[strlen(directories)];
    strcpy(dir_copy,directories);

    char *cpy = malloc(strlen(directories)*sizeof(char)); 
	strcpy(cpy, directories);

    char * token;
    token = strtok(cpy,"/"); // pega o primeiro elemento apos root

    int index_block_fat = 0;
    union data_blocks block;
    if (directories[0] == '/'){
        index_block_fat = 5;
        block = read_block(5);
    }else{
        printf("Caminho não possui diretório root!\n");
        return;
    }
    int count = 0;
    // conta quantos direitorios há na string 
    while(token != NULL){
        token = strtok(NULL,"/");
        count++;
    }
    token = strtok(dir_copy,"/");
    // caminha nos diretórios até chegar no penúltimo,
    //  pois o último é o arquivo que deve ser criado
    while( count > 1){;
        int i;
        int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
        int found_dir = 0;
        // procura o diretorio atual no anterior
        for (i = 0; i < size_dir; i ++){
            if (strcmp(block.root_dir[i].filename,token) == 0){
                found_dir = 1;
                if(block.root_dir[i].attributes == 1){
                    index_block_fat = block.root_dir[i].first_block;
                    block = read_block(block.root_dir[i].first_block);
                }else{
                    printf("%s não é um diretório!\n",token);
                }
                break;
            }
        }
        if (!found_dir){
            printf("Não existe este diretório %s\n",token);
            return;
        }
        token = strtok(NULL,"/");
        count--;
    }
    int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
    int i;
    // tendo o diretorio no qual queremos criar o novo arquivo (token)
    // basta verificar se nao existe um arquivo com este mesmo nome
    // verificar se possui um bloco livre no diretório e na fat
    for (i = 0; i < size_dir; i++){
        if (strcmp(block.root_dir[i].filename, token) == 0){
            printf("Já possui uma entrada neste diretório com este mesmo nome!\n");
            return;
        }
        if (block.root_dir[i].first_block == 0){
            int index_fat = findFreeSpaceFat();
            if(index_fat == -1 ){
                printf("Fat não possui espaço vazio!\n");
                return;
            }
            fat[index_fat] = 0xffff;
            dir_entry_s new_arq;
            // limpa o novo arquivo a ser criado (apaga os lixos da memoria)
            memset(&new_arq,0x0000,sizeof(dir_entry_s));
            memcpy(new_arq.filename,token,sizeof(char) * strlen(token));
            new_arq.attributes = 0;
            new_arq.first_block = index_fat;
            new_arq.size = 0;
            // salva este novo arquivo no bloco do pai
            block.root_dir[i] = new_arq;
            write_block(index_block_fat,&block);
            write_fat();
            break;
        }
    }
    free(cpy);
}

void unlink(char* directories){
    
    char dir_copy[strlen(directories)];
    strcpy(dir_copy,directories);
    
    char *cpy = malloc(strlen(directories)*sizeof(char));
    strcpy(cpy, directories);
    
    char * token;
    token = strtok(cpy,"/"); // pega o primeiro elemento apos root
    
    int index_block_fat = 0;
    
    union data_blocks block;
    
    if (directories[0] == '/'){
        index_block_fat = 4;
        block = read_block(4);
    }else{
        printf("Caminho não possui diretório root!\n");
        return;
    }
    
    int count = 0;
    
    // conta quantos direitorios há na string
    while(token != NULL){
        token = strtok(NULL,"/");
        count++;
    }
    
    token = strtok(dir_copy,"/");
    
    // caminha nos diretórios até chegar no penúltimo,
    //  pois o último é o arquivo que deve ser removido
    while( count > 1) {
        int i;
        int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
        //int found_dir = 0;
        
        // procura o diretorio atual no anterior
        for (i = 0; i < size_dir; i ++){
            
            if (strcmp(block.root_dir[i].filename,token) == 0){
                //found_dir = 1;
                if(block.root_dir[i].attributes == 1){
                    index_block_fat = block.root_dir[i].first_block;
                    read_block(block.root_dir[i].first_block);
                }else{
                    printf("%s não é um diretório!\n",token);
                }
                break;
            }
        }
        
        //if (!found_dir){
        //    printf("Não existe este diretório %s\n",token);
        //    return;
        //}
        
        token = strtok(NULL,"/");
        count--;
    }
    
    int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
    int i;
    int found_unlink = 0;
    // tendo o diretorio no qual queremos criar o novo (token)
    // basta verificar se nao existe um arquivo com este mesmo nome
    // verificar se possui um bloco livre no diretório e na fat
    for (i = 0; i < size_dir; i++){
        
        if (strcmp(block.root_dir[i].filename, token) == 0){
            if(block.root_dir[i].attributes == 1){
                found_unlink = 1;
                union data_blocks blocks_dir = read_block(block.root_dir[i].first_block);
                
                int j;
                for (j = 0; j < size_dir; j++){
                    if(blocks_dir.root_dir[j].first_block != 0){
                        printf("Diretório ainda possui elementos!\n");
                        return;
                    }
                }
                
                memset(&blocks_dir,0x0000,BLOCK_SIZE);
                fat[block.root_dir[i].first_block] = 0x0000;
                
                write_block(block.root_dir[i].first_block,&blocks_dir);
                memset(&block.root_dir[i],0x0000,sizeof(dir_entry_s));
                
                resize(directories,-block.root_dir[i].size);
                
                write_block(index_block_fat,&block);
                write_fat();
                break;
                
            }else if(block.root_dir[i].attributes == 0){
                //found_unlink = 1;
                
                uint16_t current = block.root_dir[i].first_block;
                uint16_t temp = block.root_dir[i].first_block;
                
                // vai apagando os blocos  até o penultimo bloco
                while (fat[current] != 0x7fff){
                    union data_blocks blocks_dir = read_block(current);
                    memset(&blocks_dir,0x0000,BLOCK_SIZE);
                    write_block(current,&blocks_dir);
                    temp = fat[current];
                    fat[current] = 0x0000;
                    current = temp;
                }
                
                // ultimo bloco qu e possui o valor 0xffff
                union data_blocks blocks_dir = read_block(current);
                memset(&blocks_dir,0x0000,BLOCK_SIZE);
                write_block(current,&blocks_dir);
                fat[current] = 0x0000;
                
                resize(directories,-block.root_dir[i].size);
                
                memset(&block.root_dir[i],0x0000,sizeof(dir_entry_s));
                write_block(index_block_fat,&block);
                write_fat();
                break;
                
            }
        }
        
    }
    
    //if(!found_unlink){
    //    printf("Arquivo não encontrado!\n");
    //    return;
    //}
    
    free(cpy);
}

void write(char * words, char* directories){
	char dir_copy[strlen(directories)];
	strcpy(dir_copy,directories);

	char *cpy = malloc(strlen(directories)*sizeof(char)); 
	strcpy(cpy, directories);

	char * token;
	token = strtok(cpy,"/"); // pega o primeiro elemento apos root
	
	int index_block_fat = 0;

	union data_blocks block;

	if (directories[0] == '/'){
		index_block_fat = 4;
		block = read_block(4);
	}else{
		printf("Caminho não possui diretório root!\n");
		return;
	}
	
	int count = 0;

	// conta quantos direitorios há na string
	while(token != NULL){
		token = strtok(NULL,"/"); 
		count++;
	}

	token = strtok(dir_copy,"/");

	// caminha nos diretórios até chegar no penúltimo,
    //  pois o último é o arquivo que deve ser criado
	while( count > 1){;
		int i;
		int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
		int found_dir = 0;

		// procura o diretorio atual no anterior
		for (i = 0; i < size_dir; i ++){

			if (strcmp(block.root_dir[i].filename,token) == 0){
				found_dir = 1;
				if(block.root_dir[i].attributes == 1){
					index_block_fat = block.root_dir[i].first_block;
					block = read_block(block.root_dir[i].first_block);
				}else{
					printf("%s não é um diretório!\n",token);
				}
				break;
			}
		}

		//if (!found_dir){
		//	printf("Não existe este diretório %s\n",token);
		//	return;
		//}

		token = strtok(NULL,"/"); 
		count--;
	}

	int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
	int i;
	int found_unlink = 0;
	// tendo o diretorio no qual queremos criar o novo (token)
	// basta verificar se nao existe um arquivo com este mesmo nome
	// verificar se possui um bloco livre no diretório e na fat
	for (i = 0; i < size_dir; i++){

		if (strcmp(block.root_dir[i].filename, token) == 0){

			if(block.root_dir[i].attributes == 0){
				found_unlink = 1;

				uint16_t current = block.root_dir[i].first_block;
				uint16_t temp = block.root_dir[i].first_block;

				// vai apagando os blocks até o penultimo bloco
				while (fat[current] != 0xffff){
					union data_blocks blocks_dir = read_block(current);
					memset(&blocks_dir,0x0000,BLOCK_SIZE);
					write_block(current,&blocks_dir);
					temp = fat[current];
					fat[current] = 0x0000;
					current = temp;
				}

				// ultimo bloco que possui o valor 0xffff
				union data_blocks blocks_dir = read_block(current);
				memset(&blocks_dir,0x0000,BLOCK_SIZE);
				block.root_dir[i].first_block = current;
				write_block(current,&blocks_dir);

				int len = strlen(words);

				block.root_dir[i].size = len;
				resize(directories,len);

				int number_blocks = ceil(len/(BLOCK_SIZE * 1.0));

    			char buffer[len + 1];
				
				int j = 0;
				while(1){
					int offset = j * BLOCK_SIZE;
					slicestr(words, buffer, offset, BLOCK_SIZE + offset);

					blocks_dir = read_block(current);
					memcpy(blocks_dir.data,buffer,sizeof(char) * strlen(buffer));
					write_block(current,&blocks_dir);

					fat[current] = 0xffff;
					j++;

					if (j < number_blocks){
						int next_index_fat = findFreeSpaceFat();

						if( next_index_fat == -1 ){
							printf("Fat não possui espaço vazio!\n");
							return;
						}

						fat[current] = next_index_fat;
						current = next_index_fat;

					}else{
						break;
					}
				}

				write_block(index_block_fat,&block);
				write_fat();
				break;
			}
				
		}
		
	}

	//if(!found_unlink){
	//	printf("Arquivo não encontrado!\n");
	//	return;
	//}
	free(cpy);
}

void append(char * words, char* directories){
    char dir_copy[strlen(directories)];
    strcpy(dir_copy,directories);

    char *cpy = malloc(strlen(directories)*sizeof(char));
    strcpy(cpy, directories);

    char * token;
    token = strtok(cpy,"/"); // pega o primeiro elemento apos root
    
    int index_block_fat = 0;

    union data_blocks block;

    if (directories[0] == '/'){
        index_block_fat = 5;
        block = read_block(5);
    }else{
        printf("Caminho não possui diretório root!\n");
        return;
    }
    
    int count = 0;

    // conta quantos direitorios há na string
    while(token != NULL){
        token = strtok(NULL,"/");
        count++;
    }

    token = strtok(dir_copy,"/");

    // caminha nos diretórios até chegar no penúltimo,
    //  pois o último é o arquivo que deve ser utilzizado
    while( count > 1){;
        int i;
        int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
        int found_dir = 0;

        // procura o diretorio atual no anterior
        for (i = 0; i < size_dir; i ++){

            if (strcmp(block.root_dir[i].filename,token) == 0){
                found_dir = 1;
                if(block.root_dir[i].attributes == 1){
                    index_block_fat = block.root_dir[i].first_block;
                    block = read_block(block.root_dir[i].first_block);
                }else{
                    printf("%s não é um diretório!\n",token);
                }
                break;
            }
        }

        if (!found_dir){
            printf("Não existe este diretório %s\n",token);
            return;
        }

        token = strtok(NULL,"/");
        count--;
    }

    int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
    int i;
    int found_unlink = 0;
    // tendo o diretorio no qual queremos criar o novo (token)
    // basta verificar se nao existe um arquivo com este mesmo nome
    // verificar se possui um bloco livre no diretório e na fat
    for (i = 0; i < size_dir; i++){

        if (strcmp(block.root_dir[i].filename, token) == 0){

            if(block.root_dir[i].attributes == 0){
                found_unlink = 1;


                uint16_t current = block.root_dir[i].first_block;
                uint16_t temp = block.root_dir[i].first_block;

                int len = strlen(words);

                block.root_dir[i].size += len;
                resize(directories,len);

                char buffer[len + 1];

                int count_letters = 0;

                // procura o último bloco do arquivo
                while (fat[current] != 0xffff){
                    union data_blocks blocks_dir = read_block(current);
                    current = fat[current];
                }

                // tendo chegado no ultimo bloco preencho os espaços livres com a palavra
                union data_blocks blocks_dir = read_block(current);
                int j;
                    for (j = 0; j < BLOCK_SIZE; j ++){
                        if (count_letters >= len){break;}
                        if (blocks_dir.data[j] == 0x0000){
                            blocks_dir.data[j] = words[count_letters];
                            count_letters++;
                        }
                    }
                write_block(current,&blocks_dir);
                
                // se no espaço livre do bloco final coube toda a palavra
                if (count_letters == strlen(words)){
                    write_block(index_block_fat,&block);
                    write_fat();
                    return;
                }
                // realiza-se um split da palavra desta forma se obtem o restante
                // string no qual deve ser salva em blocks livres
                slicestr(words, buffer, count_letters, strlen(words));

                strcpy(words,buffer);
                len = strlen(words);
                int number_blocks = ceil(len/(BLOCK_SIZE * 1.0));

                uint16_t final_blocks = current;
                current = findFreeSpaceFat();
                fat[final_blocks] = current;
                j = 0;
                while(1){
                    int offset = j * BLOCK_SIZE;
                    slicestr(words, buffer, offset, BLOCK_SIZE + offset);

                    blocks_dir = read_block(current);
                    memcpy(blocks_dir.data,buffer,sizeof(char) * strlen(buffer));
                    write_block(current,&blocks_dir);

                    fat[current] = 0xffff;
                    j++;

                    if (j < number_blocks){
                        int next_index_fat = findFreeSpaceFat();

                        if( next_index_fat == -1 ){
                            printf("Fat não possui espaço vazio!\n");
                            return;
                        }

                        fat[current] = next_index_fat;
                        current = next_index_fat;

                    }else{
                        break;
                    }
                }

                write_block(index_block_fat,&block);
                write_fat();
                break;
            }
                
        }
        
    }

    if(!found_unlink){
        printf("Arquivo não encontrado!\n");
        return;
    }
    free(cpy);
}

void read(char* directories){

	char dir_copy[strlen(directories)];
	strcpy(dir_copy,directories);

	char *cpy = malloc(strlen(directories)*sizeof(char)); 
	strcpy(cpy, directories);

	char * token;
	token = strtok(cpy,"/"); // pega o primeiro elemento apos root
	
	int index_block_fat = 0;

	union data_blocks block;

	if (directories[0] == '/'){
		index_block_fat = 4;
		block = read_block(4);
	}else{
		printf("Caminho não possui diretório root!\n");
		return;
	}
	
	int count = 0;

	// conta quantos direitorios há na string
	while(token != NULL){
		token = strtok(NULL,"/"); 
		count++;
	}

	token = strtok(dir_copy,"/");

	// caminha nos diretórios até chegar no ultimo 
	// no qual é o que deve ser lido
	while( count > 1) {
		int i;
		int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
		int found_dir = 0;

		// procura o diretorio atual no anterior
		for (i = 0; i < size_dir; i ++){

			if (strcmp(block.root_dir[i].filename,token) == 0){
				found_dir = 1;
				if(block.root_dir[i].attributes == 1){
					index_block_fat = block.root_dir[i].first_block;
					block = read_block(block.root_dir[i].first_block);
				}else{
					printf("%s não é um diretório!\n",token);
				}
				break;
			}
		}

		//if (!found_dir){
		//	printf("Não existe este diretório %s\n",token);
		//	return;
		//}

		token = strtok(NULL,"/"); 
		count--;
	}

	int size_dir = BLOCK_SIZE / sizeof(dir_entry_s);
	int i;
	int found_unlink = 0;
	// tendo o diretorio no qual queremos criar o novo (token)
	// basta verificar se nao existe um arquivo com este mesmo nome
	// verificar se possui um bloco livre no diretório e na fat
	for (i = 0; i < size_dir; i++){

			if(block.root_dir[i].attributes == 0){
				found_unlink = 1;


				uint16_t current = block.root_dir[i].first_block;
				uint16_t temp = block.root_dir[i].first_block;

				char result[block.root_dir[i].size];
				int count_letters = 0;
				// vai andando nos blocks até o penultimo bloco
				//  e salvando seus valores em result
				while (fat[current] != 0xffff){
					union data_blocks blocks_dir = read_block(current);
					int j;
					for (j = 0; j < BLOCK_SIZE; j ++){
						if ( blocks_dir.data[j] == 0x0000){ break; }
						result[count_letters] = blocks_dir.data[j];
						count_letters++;
					}
					current = fat[current];
				}
				// ultimo bloco com valor na fat de 0xffff
				union data_blocks blocks_dir = read_block(current);
				int j;
					for (j = 0; j < BLOCK_SIZE; j ++){
						if (blocks_dir.data[j] == 0x0000){ break; }
						result[count_letters] = blocks_dir.data[j];
						count_letters++;
					}
				
				// imprime os dados do arquivo
				for (j = 0; j < block.root_dir[i].size; j++){
					printf("%c",result[j]);
				}
				printf("\n");
				break;	
			}	
	}

	if(!found_unlink){
		printf("Arquivo não encontrado!\n");
		return;
	}
	free(cpy);
}

int main(){
    system("clear");
   char input_str[255];
   int ch;
   int i;
    while (1){
        memset(input_str,0x0000,sizeof(input_str));
        printf(">> ");
        for (i = 0; (i < (sizeof(input_str)-1) &&
         ((ch = fgetc(stdin)) != EOF) && (ch != '\n')); i++){
             input_str[i] = ch;
         }

            input_str[i] = '\0';

        if (strcmp(input_str,"init") == 0){
            init();
        }else if (strcmp(input_str,"load") == 0){
            load();
        } else if (strcmp(input_str,"exit") == 0){
            exit(0);

        }else if (strcmp(input_str,"clear") == 0){
            system("clear");
        }else if (strstr(input_str, "\"") != NULL) {
            char *cpy = malloc(strlen(input_str)*sizeof(char));
             strcpy(cpy, input_str);

            char * token;
            token = strtok(cpy," \"");

            if (strcmp(token,"write") == 0){

                char *string = strtok(NULL, "\"");
                
                char *path = strtok(NULL, " \"");
                write(string,path);

            } else if (strcmp(token,"append") == 0){

                char *string = strtok(NULL, "\"");
                
                char *path = strtok(NULL, " \"");
                append(string,path);

            }

        }else {

             char *cpy = malloc(strlen(input_str)*sizeof(char));
             strcpy(cpy, input_str);

            char * token;

            token = strtok(cpy," ");

            if(strcmp(token,"ls") == 0){

                token = strtok(NULL, " ");
                char *teste = malloc(strlen(token)*sizeof(char));
                memcpy(teste,token,strlen(token)*sizeof(char));
                ls(token);

            }else if (strcmp(token,"mkdir") == 0){

                token = strtok(NULL, " ");
                mkdir(token);

            }else if (strcmp(token,"create") == 0){
                char *path = strtok(NULL, " ");
                create(path);

            }else if (strcmp(token,"unlink") == 0){

                char *path = strtok(NULL, " ");
                unlink(path);

            }else if (strcmp(token,"read") == 0){

                char *path = strtok(NULL, " ");
                read(path);

            }else {
                printf("Comando não encontrado!\n");
            }

            free(cpy);
        }
    }
    

   return 0;
}

