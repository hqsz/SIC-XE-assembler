#include "sic_xe.h"

int main() {

	// Start System, in this program System means SIC/XE machine

	Init_System();
	Exec_System();
	Final_System();

}

void Init_System() {

	/*
	Initialize System, This function only called once when program start
	First, set all dump memory to 0, and dump address to 0
	Second, read "opcode.txt" and make hash table
	*/

	int i;
	FILE* fp;
	hashtb_link* hashtb_i = NULL;
	hashtb_link* hashtb_temp;

	// Initialize dump memory and dump address( 
	for (i = 0; i < DUMP_CELL_SIZE; i++) {
		cell_dump[i] = 0;
	}
	addr_dump = 0;

	// Intialize hash table
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		hashtb_head[i] = NULL;
	}

	fp = fopen("opcode.txt", "r");

	while (1) {
		hashtb_temp = (hashtb_link*)calloc(1, sizeof(hashtb_link));
		if (fscanf(fp, "%x %s %s\n", &(hashtb_temp->opcode), hashtb_temp->mnemonic, hashtb_temp->type) == EOF) {
			free(hashtb_temp);
			break;
		}
		hashtb_temp->next = NULL;

		if (hashtb_head[Hash_Function(hashtb_temp->mnemonic)] == NULL) {
			hashtb_head[Hash_Function(hashtb_temp->mnemonic)] = hashtb_temp;
		}
		else {
			hashtb_i = hashtb_head[Hash_Function(hashtb_temp->mnemonic)];
			while (hashtb_i->next != NULL) {
				hashtb_i = hashtb_i->next;
			}
			hashtb_i->next = hashtb_temp;
		}
	}

	addr_prog = 0;
	for (i = 0; i < 10; i++) {
		cell_reg[i] = NONE;
	}
	fclose(fp);

}
void Exec_System() {

	/*
	Execute System, until get quit command, program get command and execute command
	And also get incorrect command or parameter print error message

	Error list
	1. length of command is longer than 99 letter(maximum length of input)
	2. length of command is shorter than 1 letter
	3. unavailable command
	4. range of address is smaller than 0x00000 or bigger than 0xFFFFF
	5. range of value is smaller than 0x00 or bigger than 0xFF
	6. start address is bigger than end address
	7. no such input mnemonic in hash table
	8. no input file in directory
	9. no symbol table in system
	10. no loaded program in system
	*/

	int i;
	char tok_cmd[INPUT_CMD_LENGTH];
	char* tok_i;

	unsigned long first_para, second_para, third_para;
	char first_comma, second_comma, invalid_char;
	char first_filename[INPUT_CMD_LENGTH], second_filename[INPUT_CMD_LENGTH], third_filename[INPUT_CMD_LENGTH];
	hashtb_link* hashtb_temp;
	int tok_num;
	FILE* fp;

	while (1) {
		printf("sicsim> ");
		for (i = 0; i < INPUT_CMD_LENGTH; i++) {
			input_cmd[i] = getchar();
			if (input_cmd[i] == '\n') {
				input_cmd[i] = '\0';
				break;
			}
		}

		if (i == INPUT_CMD_LENGTH) {
			while (getchar() != '\n'); // if input command is longer than maximum length of input, empty input buffer, and then print error code
			printf("ERROR 01 : length of command cannot be longer than 99 letters\n");
			continue;
		}
		else if (i == 0) { // If there is no input( ex : <enter>), print error code
			printf("ERROR 02 : length of command cannot be shorter than 1 letters\n");
			continue;
		}

		strcpy(tok_cmd, input_cmd);
		tok_i = strtok(tok_cmd, " \t"); // tokenize command, then we can classify command

		if (tok_i == NULL) { // If input only contains space or tab, print error code
			printf("ERROR 03 : not available command\n");
			continue;
		}

		strcpy(input_cmd, tok_i); // make input command to strict form

		if (strcmp(tok_i, "help") == 0 || strcmp(tok_i, "h") == 0) { // When command is help  
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			Cmd_Help();
		}
		else if (strcmp(tok_i, "dir") == 0 || strcmp(tok_i, "d") == 0) { // When command is dir
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			Cmd_Dir();
		}
		else if (strcmp(tok_i, "quit") == 0 || strcmp(tok_i, "q") == 0) { // When command is quit
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			break;
		}
		else if (strcmp(tok_i, "history") == 0 || strcmp(tok_i, "hi") == 0) { // When command is history
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			Cmd_History();
		}
		else if (strcmp(tok_i, "dump") == 0 || strcmp(tok_i, "du") == 0) { // When command is dump, If there is no parameter, then parameter pass -1(NONE)
			tok_i = strtok(NULL, "");

			if (tok_i == NULL) { // dump form 1
				Cmd_Dump(NONE, NONE);
			}
			else {
				tok_num = sscanf(tok_i, "%lx %c %lx %c", &first_para, &first_comma, &second_para, &invalid_char); // distinct parameters

				if (tok_num == EOF) { // dump form 1
					Cmd_Dump(NONE, NONE);
				}
				else if (tok_num == 1) { // dump form 2
					if (first_para < 0x00000 || first_para >= DUMP_CELL_SIZE) { // if range of address is smaller than 0x00000 or bigger than 0xFFFFF, print error code
						printf("ERROR 04 : range of value is 0x00000 to 0xFFFFF\n");
						continue;
					}

					// make input command to strict form
					strcat(input_cmd, " ");
					strcat(input_cmd, strtok(tok_i, " \t"));

					Cmd_Dump(first_para, NONE);
				}
				else if (tok_num == 3 && first_comma == ',') { // dump form 3
					if (first_para < 0x00000 || first_para >= DUMP_CELL_SIZE || second_para < 0x00000 || second_para >= DUMP_CELL_SIZE) { // if range of address is smaller than 0x00000 or bigger than 0xFFFFF, print error code
						printf("ERROR 04 : range of value is 0x00000 to 0xFFFFF\n");
						continue;
					}

					if (first_para > second_para) { // if start address is bigger than end address, print error code
						printf("ERROR 06 : start address cannot bigger than end address\n");
						continue;
					}

					// make input command to strict form
					strcat(input_cmd, " ");
					strcat(input_cmd, strtok(tok_i, " ,\t"));
					strcat(input_cmd, ", ");
					strcat(input_cmd, strtok(NULL, " ,\t"));

					Cmd_Dump(first_para, second_para);
				}
				else { // if there is other character in command, print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}
			}
		}
		else if (strcmp(tok_i, "edit") == 0 || strcmp(tok_i, "e") == 0) { // When command is edit
			tok_i = strtok(NULL, "");

			if (tok_i == NULL) {
				printf("ERROR 03 : not available command\n");
				continue;
			}

			tok_num = sscanf(tok_i, "%lx %c %lx %c", &first_para, &first_comma, &second_para, &invalid_char); // distinct parameters
			if (tok_num == 3 && first_comma == ',') {
				if (first_para < 0 || first_para >= DUMP_CELL_SIZE) { // if range of address is smaller than 0x00000 or bigger than 0xFFFFF, print error code
					printf("ERROR 04 : range of value is 0x00000 to 0xFFFFF\n");
					continue;
				}

				if (second_para < 0x00 || second_para > 0xFF) { // if range of value is smaller than 0x00 or bigger than 0xFF, print error code
					printf("ERROR 05 : range of value is 0x00 to 0xFF\n");
					continue;
				}

				// make input command to strict form
				strcat(input_cmd, " ");
				strcat(input_cmd, strtok(tok_i, " ,\t"));
				strcat(input_cmd, ", ");
				strcat(input_cmd, strtok(NULL, " ,\t"));

				Cmd_Edit(first_para, second_para);
			}
			else { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}
		}
		else if (strcmp(tok_i, "fill") == 0 || strcmp(tok_i, "f") == 0) { // When command is fill
			tok_i = strtok(NULL, "");

			if (tok_i == NULL) {
				printf("ERROR 03 : not available command\n");
				continue;
			}

			tok_num = sscanf(tok_i, "%lx %c %lx %c %lx %c", &first_para, &first_comma, &second_para, &second_comma, &third_para, &invalid_char); // distinct parameters
			if (tok_num == 5 && first_comma == ',' && second_comma == ',') {
				if (first_para < 0 || first_para >= DUMP_CELL_SIZE || second_para < 0 || second_para >= DUMP_CELL_SIZE) {
					printf("ERROR 04 : range of value is 0x00000 to 0xFFFFF\n"); // if range of address is smaller than 0x00000 or bigger than 0xFFFFF, print error code
					continue;
				}

				if (third_para < 0x00 || third_para > 0xFF) { // if range of value is smaller than 0x00 or bigger than 0xFF, print error code
					printf("ERROR 05 : range of value is 0x00 to 0xFF\n");
					continue;
				}

				if (first_para > second_para) { // if start address is bigger than end address, print error code
					printf("ERROR 06 : start address cannot bigger than end address\n");
					continue;
				}

				// make input command to strict form
				strcat(input_cmd, " ");
				strcat(input_cmd, strtok(tok_i, " ,\t"));
				strcat(input_cmd, ", ");
				strcat(input_cmd, strtok(NULL, " ,\t"));
				strcat(input_cmd, ", ");
				strcat(input_cmd, strtok(NULL, " ,\t"));

				Cmd_Fill(first_para, second_para, third_para);
			}
			else { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}
		}
		else if (strcmp(tok_i, "reset") == 0) { // When command is reset
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			Cmd_Reset();
		}
		else if (strcmp(tok_i, "opcode") == 0) { // When command is opcode
			tok_i = strtok(NULL, " \t");

			if (tok_i == NULL) { // if there is no mnemonic in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}
			else {
				if (strtok(NULL, " ") != NULL) { // if there is other character in command, print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}

				// make input command to strict form
				strcat(input_cmd, " ");
				strcat(input_cmd, tok_i);

				hashtb_temp = Find_Opcodemnemonic(tok_i);
				if (hashtb_temp == NULL) { // if return of opcode function is NULL, it means there is no such mnemonic in hash table, so print error code
					printf("ERROR 07 : There is no such mnemonic in hash table\n");
					continue;
				}
				else {
					Cmd_Opcodemnemonic(hashtb_temp);
				}
			}
		}
		else if (strcmp(tok_i, "opcodelist") == 0) { // When command is opcodelist
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			Cmd_Opcodelist();
		}
		else if (strcmp(tok_i, "type") == 0) {
			tok_i = strtok(NULL, " \t");

			if (tok_i == NULL) { // if there is no filename in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}
			else {
				if (strtok(NULL, " ") != NULL) { // if there is other character in command, print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}

				strcat(input_cmd, " ");
				strcat(input_cmd, tok_i);

				fp = fopen(tok_i, "r");

				if (fp == NULL) { // if file pointer is NULL, it means there is no such file in now directory, so print error code
					printf("ERROR 08 : There is no such file in directory\n");
					continue;
				}
				else {
					Cmd_Typefilename(fp);
					fclose(fp);
				}
			}
		}
		else if (strcmp(tok_i, "assemble") == 0) {
			tok_i = strtok(NULL, " \t");

			if (tok_i == NULL) { // if there is no filename in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}
			else {
				if (strstr(tok_i, ".asm") == NULL) { // If filename is not assembly file(.asm), print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}

				if (strtok(NULL, " ") != NULL) { // if there is other character in command, print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}

				strcat(input_cmd, " ");
				strcat(input_cmd, tok_i);

				Cmd_Assemblefilename(tok_i);
			}
		}
		else if (strcmp(tok_i, "symbol") == 0) {
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			if (symbol_head == NULL) {
				printf("ERROR 09 : There is no such symbol table in system\n");
				continue;
			}
			else {
				Cmd_Symbol();
			}
		}
		else if (strcmp(tok_i, "progaddr") == 0) {
			tok_i = strtok(NULL, "");

			if (tok_i == NULL) {
				printf("ERROR 03 : not available command\n");
				continue;
			}

			tok_num = sscanf(tok_i, "%lx %c", &first_para, &invalid_char); // distinct parameters
			if (tok_num == 1) {
				if (first_para < 0 || first_para >= DUMP_CELL_SIZE) { // if range of address is smaller than 0x00000 or bigger than 0xFFFFF, print error code
					printf("ERROR 04 : range of value is 0x00000 to 0xFFFFF\n");
					continue;
				}

				// make input command to strict form
				strcat(input_cmd, " ");
				strcat(input_cmd, tok_i);

				Cmd_Progaddr(first_para);
			}
			else { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}
		}
		else if (strcmp(tok_i, "loader") == 0) {
			tok_i = strtok(NULL, "");

			if (tok_i == NULL) {
				printf("ERROR 03 : not available command\n");
				continue;
			}

			for (i = 0; i < INPUT_CMD_LENGTH; i++) { // clear filename parameters
				first_filename[i] = '\0';
				second_filename[i] = '\0';
				third_filename[i] = '\0';
			}

			tok_num = sscanf(tok_i, "%s %s %s %c", first_filename, second_filename, third_filename, &invalid_char); // distinct parameters

			if (tok_num == 1) {
				// make input command to strict form
				strcat(input_cmd, " ");
				strcat(input_cmd, first_filename);

				if (strstr(first_filename, ".obj") == NULL) { // If filename is not object file(.obj), print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}

				Cmd_Loader(tok_num, first_filename, NULL, NULL);
			}
			else if (tok_num == 2) {
				// make input command to strict form
				strcat(input_cmd, " ");
				strcat(input_cmd, first_filename);
				strcat(input_cmd, " ");
				strcat(input_cmd, second_filename);

				if (strstr(first_filename, ".obj") == NULL) { // If filename is not object file(.obj), print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}
				else if (strstr(second_filename, ".obj") == NULL) {
					printf("ERROR 03 : not available command\n");
					continue;
				}

				Cmd_Loader(tok_num, first_filename, second_filename, NULL);
			}
			else if (tok_num == 3) {
				// make input command to strict form
				strcat(input_cmd, " ");
				strcat(input_cmd, first_filename);
				strcat(input_cmd, " ");
				strcat(input_cmd, second_filename);
				strcat(input_cmd, " ");
				strcat(input_cmd, third_filename);

				if (strstr(first_filename, ".obj") == NULL) { // If filename is not object file(.obj), print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}
				else if (strstr(second_filename, ".obj") == NULL) {
					printf("ERROR 03 : not available command\n");
					continue;
				}
				else if (strstr(third_filename, ".obj") == NULL) {
					printf("ERROR 03 : not available command\n");
					continue;
				}

				Cmd_Loader(tok_num, first_filename, second_filename, third_filename);
			}
			else { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}
		}
		else if (strcmp(tok_i, "bp") == 0) {
			tok_i = strtok(NULL, "");

			if (tok_i == NULL) {
				Cmd_Break_Point('P', NONE);
			}
			else {
				tok_num = sscanf(tok_i, "%s %c", first_filename, &invalid_char); // distinct parameters

				if (tok_num == EOF) {
					Cmd_Break_Point('P', NONE);
				}
				else if (tok_num == 1) {
					if (strcmp(first_filename, "clear") == 0) {
						strcat(input_cmd, " ");
						strcat(input_cmd, first_filename);

						Cmd_Break_Point('C', NONE);
					}
					else {
						tok_num = sscanf(first_filename, "%lx", &first_para);

						if (tok_num == 1) {
							if (first_para < 0 || first_para >= DUMP_CELL_SIZE) { // if range of address is smaller than 0x00000 or bigger than 0xFFFFF, print error code
								printf("ERROR 04 : range of value is 0x00000 to 0xFFFFF\n");
								continue;
							}

							strcat(input_cmd, " ");
							strcat(input_cmd, first_filename);

							Cmd_Break_Point('A', first_para);
						}
						else { // if there is other character in command, print error code
							printf("ERROR 03 : not available command\n");
							continue;
						}
					}
				}
				else { // if there is other character in command, print error code
					printf("ERROR 03 : not available command\n");
					continue;
				}
			}

		}
		else if (strcmp(tok_i, "run") == 0) { // When command is help  
			if (strtok(NULL, " \t") != NULL) { // if there is other character in command, print error code
				printf("ERROR 03 : not available command\n");
				continue;
			}

			if (extern_symbol_head == NULL) {
				printf("ERROR 10 : There is no loaded program in system\n");
				continue;
			}

			Cmd_Run();
		}
		else {  // if there is no profit command , print error code
			printf("ERROR 03 : not available command\n");
			continue;
		}
	}

}
void Final_System() {

	/*
	Finalize System, This function only called once when program end
	First, Finalize history, it means deallocate history
	Second, Finalize hash table, deallocate hash table
	*/

	int i;
	his_link* his_i = his_head;
	his_link* his_free = NULL;
	hashtb_link* hashtb_i;
	hashtb_link* hashtb_free = NULL;

	// Finalize history
	while (his_i != NULL) {
		his_free = his_i;
		his_i = his_i->next;
		free(his_free);
	}

	his_head = NULL;

	// Finalize hash table
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		hashtb_i = hashtb_head[i];

		while (hashtb_i != NULL) {
			hashtb_free = hashtb_i;
			hashtb_i = hashtb_i->next;
			free(hashtb_free);
		}

		hashtb_head[i] = NULL;
	}

	// Finalize Symbol table
	Free_Symbol();

	// Finalize Extern Symbol table
	Free_Extern_Symbol();

	// Finalize Extern Symbol table
	Free_Break_Point();

}
void Cmd_Help() {

	/* Print all command which can be executed and add command to history */

	printf("h[elp]\n");
	printf("d[ir]\n");
	printf("q[uit]\n");
	printf("hi[story]\n");
	printf("du[mp][start, end]\n");
	printf("e[dit] address, value\n");
	printf("f[ill] start, end, value\n");
	printf("reset\n");
	printf("opcode mnemonic\n");
	printf("opcodelist\n");
	printf("assemble filename\n");
	printf("type filename\n");
	printf("symbol\n");
	printf("progaddr\n");
	printf("loader\n");
	printf("run\n");
	printf("bp\n");

	Add_History();
}
void Cmd_Dir() {

	/*
	Print list of files in current directory
	If file is executable file, print "*" next to file name
	If file is directory, print "/" next to file name
	After print list of files, then add command to history
	*/

	DIR* dirp;
	struct dirent *direntp = NULL;
	struct stat file_info;
	int print_cnt = 0;

	// open current directory
	dirp = opendir(".");

	// print list of files
	while ((direntp = readdir(dirp)) != NULL) {
		stat(direntp->d_name, &file_info);

		if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
			continue;
		}

		printf("%25s", direntp->d_name);

		if (S_ISDIR(file_info.st_mode)) { // file is directory
			printf("/");
		}
		else if (S_IEXEC & file_info.st_mode) { // file is executable file
			printf("*");
		}

		print_cnt++;
		if (print_cnt % 3 == 0) {
			printf("\n");
		}
	}

	printf("\n");
	closedir(dirp);

	Add_History();
}
void Cmd_History() {

	/*
	Print history, it means list of successful commands
	First add command to history, then print history
	*/

	his_link* his_i;
	int his_count = 1;

	Add_History();

	// Print history
	his_i = his_head;
	while (his_i != NULL) {
		printf("%d\t%s\n", his_count, his_i->his_cmd);
		his_count++;
		his_i = his_i->next;
	}

}
void Add_History() {

	/*
	Add command to history
	To history, we only add successful command
	*/

	his_link* his_i = NULL;
	his_link* his_temp;
	his_temp = (his_link*)calloc(1, sizeof(his_link));
	strcpy(his_temp->his_cmd, input_cmd);
	his_temp->next = NULL;

	// add command to history
	if (his_head == NULL) {
		his_head = his_temp;
	}
	else {
		his_i = his_head;
		while (his_i->next != NULL) {
			his_i = his_i->next;
		}
		his_i->next = his_temp;
	}

}
void Cmd_Dump(int start_addr, int end_addr) {

	/*
	Print dump memory, we have three forms how to print dump memory
	1) dump : print 160 values from address (recently dump (end address + 1), initialized to 0)
	2) dump start : print 160 values from start address
	3) dump start, end : print from start address to end address
	if parameter is -1 it means there is no parameter(NONE)
	and then add command to history
	*/

	int i;
	int cnt_print_line;
	int addr_dump_temp;

	// calculate dump address depending on form
	if (end_addr == NONE) {
		if (start_addr == NONE) { // form 1 : start address = recently dump end address + 1
			start_addr = addr_dump;
		}

		if (start_addr + (DUMP_ONELINE_SIZE * 10) < DUMP_CELL_SIZE) { // form 2 : end address = start address + 160;
			end_addr = start_addr + (DUMP_ONELINE_SIZE * 10) - 1;
		}
		else { // if end value is bigger than end of dump, then end address is end of dump
			end_addr = DUMP_CELL_SIZE - 1;

		}
	}

	cnt_print_line = end_addr / DUMP_ONELINE_SIZE - start_addr / DUMP_ONELINE_SIZE + 1;
	addr_dump_temp = start_addr - start_addr % DUMP_ONELINE_SIZE;

	// print dump 
	while (cnt_print_line--) {
		printf("%05X ", addr_dump_temp);
		for (i = addr_dump_temp; i < addr_dump_temp + DUMP_ONELINE_SIZE; i++) {
			if (i < start_addr || i > end_addr) {
				printf("   ");
			}
			else {
				printf("%02X ", cell_dump[i]);
			}
		}
		printf("; ");
		for (i = addr_dump_temp; i < addr_dump_temp + DUMP_ONELINE_SIZE; i++) {
			if (i < start_addr || i > end_addr) {
				printf(".");
			}
			else {
				if (cell_dump[i] >= 0x20 && cell_dump[i] <= 0x7E) { // if dump memory is between 0x20 to 0x7E, then print charactor with ASCII
					printf("%c", cell_dump[i]);
				}
				else {
					printf(".");
				}
			}
		}
		printf("\n");
		addr_dump_temp += DUMP_ONELINE_SIZE;
	}

	// store recently dump end address
	if (end_addr == DUMP_CELL_SIZE - 1) { // if end address is end of dump, then next dump command start at start of dump
		addr_dump = 0;
	}
	else {
		addr_dump = end_addr + 1;
	}

	Add_History();
}
void Cmd_Reset() {

	/* Reset all dump memory to 0 and add command to history */

	int i;
	for (i = 0; i < DUMP_CELL_SIZE; i++) {
		cell_dump[i] = 0;
	}

	Add_History();
}
void Cmd_Edit(int edit_addr, int value) {

	/* Get edit address and value, then edit dump memory and add command to history */

	cell_dump[edit_addr] = value;

	Add_History();
}
void Cmd_Fill(int start_addr, int end_addr, int value) {

	/*
	Get start address, end address and value
	In dump memory, from start address to end address fill with value
	After fill value, then add command to history
	*/

	int i;
	for (i = start_addr; i < end_addr + 1; i++) {
		cell_dump[i] = value;
	}

	Add_History();
}
void Cmd_Opcodelist() {

	/*	Print hash table containing opcode, and then add command to history */

	int i;
	hashtb_link* hashtb_i;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		printf("%d : ", i);
		hashtb_i = hashtb_head[i];

		while (hashtb_i != NULL) {
			printf("[%s,%02X]", hashtb_i->mnemonic, hashtb_i->opcode);
			if (hashtb_i->next != NULL) {
				printf(" -> ");
			}
			hashtb_i = hashtb_i->next;
		}

		printf("\n");
	}

	Add_History();
}
int Hash_Function(char* mnemonic) {

	/*
	Get mnemonic and calculate proper index then return index
	Hash function : get ASCII code mnemonic and multiple with index of character add all of them and get mod 20
	*/

	int i;
	int hash_idx = 0;
	for (i = 0; i < strlen(mnemonic); i++) {
		hash_idx += mnemonic[i] * i;
	}

	hash_idx = hash_idx % HASH_TABLE_SIZE;
	return hash_idx;
}
hashtb_link* Find_Opcodemnemonic(char* mnemonic) {

	/*
	Print opcode which is equal input mnemonic, and then return data
	If there is no such mnemonic in hash table, then return is NULL
	*/

	hashtb_link* hashtb_i = hashtb_head[Hash_Function(mnemonic)];

	while (hashtb_i != NULL) {
		if (strcmp(mnemonic, hashtb_i->mnemonic) == 0) {
			break;
		}
		hashtb_i = hashtb_i->next;
	}

	return hashtb_i;

}
void Cmd_Opcodemnemonic(hashtb_link* hashtb_temp) {

	/*
	Print opcode which is in parameter hash data
	And then add command to history
	*/

	printf("opcode is %02X\n", hashtb_temp->opcode);

	Add_History();
}
void Cmd_Typefilename(FILE* fp) {

	/*
	Get file pointer, and print file data
	And then add command to history
	*/

	char temp, last_check = '\0';

	while ((temp = fgetc(fp)) != EOF) {
		printf("%c", temp);
		last_check = temp;
	}
	if (last_check != '\n') {
		printf("\n");
	}
	else if (last_check == '\0') {

	}

	Add_History();

}
int Cmd_Assemblefilename(char* asm_filename) {

	char read_line[100];
	char tok_line[100];
	char* tok_i;
	int tok_num;
	int max_line_length = 0;

	int i;
	int temp_value;
	char temp_char;
	char temp_string[100];
	int* temp_modified = NULL;
	int modifed_num = 0;
	int obj_length = 0;

	int linectr = 0, locctr = 0;
	int start_addr = NONE, end_addr = NONE;
	int reg_b = NONE;
	char indexed;
	char* indexed_loc;
	char* comma_loc;
	int object_code;

	symbol_link* symbol_temp;

	char lst_filename[100];
	char obj_filename[100];
	FILE* asm_fp;
	FILE* lst_fp;
	FILE* obj_fp;

	Free_Symbol();

	asm_fp = fopen(asm_filename, "r");
	if (asm_fp == NULL) {
		// If there is no such assembly file in directory, then error
		printf("ERROR 08 : There is no such file in directory\n");
		return FALSE;
	}

	strcpy(lst_filename, asm_filename);
	strcpy(strstr(lst_filename, ".asm"), ".lst");
	lst_fp = fopen(lst_filename, "w");

	strcpy(obj_filename, asm_filename);
	strcpy(strstr(obj_filename, ".asm"), ".obj");
	obj_fp = fopen(obj_filename, "w");

	/*
	Pass 1 : make symbol table, store start address and end address, calculate location, and also we can find some error
	*/

	while (fgets(read_line, 100, asm_fp) != NULL) {
		linectr += 5;

		if (read_line[0] == '.') { // if line is comment, then ignore it
			continue;
		}

		if (strlen(read_line) > max_line_length) { // find maximum length of input code
			max_line_length = strlen(read_line);
		}

		for (i = 0; i < strlen(read_line); i++) { // if there is '\r'(carrage return) than exchange to ' '(space)
			if (read_line[i] == '\r') {
				read_line[i] = ' ';
			}
		}

		tok_i = strtok(read_line, " \t\n");

		if (tok_i == NULL) { // if there is nothing in input ex) <enter>, then ignore it
			continue;
		}

		if (tok_i[0] == '+' && Find_Opcodemnemonic(tok_i + 1) != NULL) { // if input is type 4 add locctr 4
			locctr += 4;
		}
		else if (Find_Opcodemnemonic(tok_i) != NULL) {
			if (strcmp(Find_Opcodemnemonic(tok_i)->type, "1") == 0) { // if input is type 1 add locctr 1
				locctr += 1;
			}
			else if (strcmp(Find_Opcodemnemonic(tok_i)->type, "2") == 0) { // if input is type 2 add locctr 2
				locctr += 2;
			}
			else { // if input is type 3 add locctr 3
				locctr += 3;
			}
		}
		else if (strcmp(tok_i, "BASE") == 0) { // if there is 'BASE' in input, then ignore it
			continue;
		}
		else if (strcmp(tok_i, "END") == 0) { // if there is 'END', then break while loop
			end_addr = locctr;
		}
		else {
			if (Find_Symbol(tok_i) == NULL) {
				if (locctr == 0 && start_addr == NONE) { // if there is 'START' at first in assembly code, then store start address
					tok_num = sscanf(tok_i + strlen(tok_i) + 1, " %s %x %c", temp_string, &locctr, &temp_char);
					if (strcmp(temp_string, "START") == 0) {
						if (tok_num == 2) {
							start_addr = locctr;
							strcpy(temp_string, tok_i);

							if (tok_i[0] < 'A' || tok_i[0] > 'z' || (tok_i[0] < 'a' && tok_i[0] > 'Z')) {
								// If there is unvalid input with label,then error
								Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with symbol");
								return FALSE;
							}

							for (i = 1; i < strlen(tok_i); i++) {
								if (tok_i[i] < '0' || (tok_i[i] > '9' && tok_i[i] < 'A') || tok_i[i] > 'z' || (tok_i[i] < 'a' && tok_i[i] > 'Z')) {
									// If there is unvalid input with label,then error
									Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with symbol");
									return FALSE;
								}
							}

							Add_Symbol(tok_i, locctr);
							continue;
						}
						else {
							// If there is unvalid input, where line with 'START'
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with START.");
							return FALSE;
						}
					}
				}

				if (tok_i[0] < 'A' || tok_i[0] > 'z' || (tok_i[0] < 'a' && tok_i[0] > 'Z')) {
					// If there is unvalid input with label,then error
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with symbol");
					return FALSE;
				}

				for (i = 1; i < strlen(tok_i); i++) {
					if (tok_i[i] < '0' || (tok_i[i] > '9' && tok_i[i] < 'A') || tok_i[i] > 'z' || (tok_i[i] < 'a' && tok_i[i] > 'Z')) {
						// If there is unvalid input with label,then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with symbol");
						return FALSE;
					}
				}

				Add_Symbol(tok_i, locctr);
			}
			else {
				// If already have same symbol, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Already have same symbol.");
				return FALSE;
			}

			tok_i = strtok(NULL, " \t\n");

			if (tok_i == NULL) {
				// If there is unvalid input, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
				return FALSE;
			}

			if (tok_i[0] == '+' && Find_Opcodemnemonic(tok_i + 1) != NULL) { // if input is type 4 add locctr 4
				locctr += 4;
			}
			else if (Find_Opcodemnemonic(tok_i) != NULL) {
				if (strcmp(Find_Opcodemnemonic(tok_i)->type, "1") == 0) { // if input is type 1 add locctr 1
					locctr += 1;
				}
				else if (strcmp(Find_Opcodemnemonic(tok_i)->type, "2") == 0) { // if input is type 2 add locctr 2
					locctr += 2;
				}
				else { // if input is type 3 add locctr 3
					locctr += 3;
				}
			}
			else if (strcmp(tok_i, "WORD") == 0) { // if input is 'WORD' add locctr 3
				locctr += 3;
			}
			else if (strcmp(tok_i, "RESW") == 0) { // if input is 'RESW' add locctr 3 * value

				tok_i = strtok(NULL, " \t\n");

				if (tok_i == NULL) {
					// If there is unvalid input with RESW, then error
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with RESW.");
					return FALSE;
				}

				tok_num = sscanf(tok_i, "%d %c", &temp_value, &temp_char);

				if (tok_num == 1) {
					locctr += 3 * temp_value;
				}
				else {
					// If there is unvalid input, where line with 'RESW'
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with RESW.");
					return FALSE;
				}

				if (strtok(NULL, " \t\n") != NULL) {
					// If there is unvalid input with RESW, then error
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with RESW.");
					return FALSE;
				}
			}
			else if (strcmp(tok_i, "RESB") == 0) { // if input is 'RESB' add locctr value

				tok_i = strtok(NULL, " \t\n");

				if (tok_i == NULL) {
					// If there is unvalid input with RESB, then error
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with RESB.");
					return FALSE;
				}

				tok_num = sscanf(tok_i, "%d %c", &temp_value, &temp_char);

				if (tok_num == 1) {
					locctr += temp_value;
				}
				else {
					// If there is unvalid input with RESB, then error
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with RESB.");
					return FALSE;
				}

				if (strtok(NULL, " \t\n") != NULL) {
					// If there is unvalid input with RESB, then error
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with RESB.");
					return FALSE;
				}
			}
			else if (strcmp(tok_i, "BYTE") == 0) { // if input is 'BYTE' add locctr size of input (1 char = 1 byte, 2 hexadecimal = 1 byte)
				tok_i = strtok(NULL, "");

				tok_num = sscanf(tok_i, " %c", &temp_char);

				if (tok_num == 1 && temp_char == 'C') {
					tok_i = strchr(tok_i, 'C') + 1;

					if (tok_i[0] == '\'') {
						for (i = 1; i < strlen(tok_i); i++) {
							if (tok_i[i] == '\'') {
								break;
							}
							locctr += 1;
						}
						for (i = i + 1; i < strlen(tok_i); i++) {
							if (tok_i[i] != ' ' && tok_i[i] != '\t' && tok_i[i] != '\n') {
								// If there is unvalid input, where line with 'BYTE'
								Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with BYTE.");
								return FALSE;
							}
						}
					}
					else {
						// If there is unvalid input, where line with 'BYTE'
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with BYTE.");
						return FALSE;
					}
				}
				else if (tok_num == 1 && temp_char == 'X') {
					tok_i = strtok(tok_i, " \t\n");

					if (tok_i[0] == 'X' && tok_i[1] == '\'' && tok_i[strlen(tok_i) - 1] == '\'') {
						for (i = 2; i < strlen(tok_i) - 3; i++) {
							if (tok_i[i] != '0') {
								break;
							}
						}
						locctr += (strlen(tok_i) - 1 - i) / 2 + (strlen(tok_i) - 1 - i) % 2;
					}
					else {
						// If there is unvalid input, where line with 'BYTE'
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with BYTE.");
						return FALSE;
					}
				}
				else {
					// If there is unvalid input, where line with 'BYTE'
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with BYTE.");
					return FALSE;
				}
			}
			else {
				// If there is no operator, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No such operator.");
				return FALSE;
			}
		}
	}

	/*
	Pass 2 : Calculate object code and find modification record, and make list(.lst) file and object(.obj) file and also we can find some error
	*/

	if (end_addr == NONE) {
		// If there is no END, then error
		Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No END.");
		return FALSE;
	}
	else if (end_addr != locctr) {
		// If END is not end of assembly file, then error
		Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with END.");
		return FALSE;
	}

	if (start_addr != NONE) {
		fprintf(obj_fp, "H%-6s%06X%06X\n", temp_string, start_addr, end_addr - start_addr);
	}
	else { // if there is no start_address mean no 'START' line
		fprintf(obj_fp, "H      000000%06X\n", end_addr);
		start_addr = 0;
	}

	locctr = start_addr;
	linectr = 0;
	rewind(asm_fp);

	while (fgets(read_line, 100, asm_fp) != NULL) {
		linectr += 5;
		object_code = 0;

		for (i = 0; i < strlen(read_line); i++) {
			if (read_line[i] == '\r') {
				read_line[i] = ' ';
			}
		}

		if (read_line[0] == '.') {
			fprintf(lst_fp, "%d\t    \t %s", linectr, read_line + 1);
			continue;
		}

		strcpy(tok_line, read_line);

		tok_i = strtok(tok_line, " \t\n");

		if (tok_i == NULL) {
			fprintf(lst_fp, "%d\n", linectr);
			continue;
		}

		if (Find_Symbol(tok_i) != NULL) { // if there is symbol in line, then pass to the next word
			tok_i = strtok(NULL, " \t\n");
		}

		if (tok_i[0] == '+' && Find_Opcodemnemonic(tok_i + 1) != NULL) { // if type is 4
			fprintf(lst_fp, "%d\t%04X\t", linectr, locctr);
			for (i = 0; i < strlen(read_line) - 1; i++) {
				fprintf(lst_fp, "%c", read_line[i]);
			}
			for (; i < max_line_length; i++) {
				fprintf(lst_fp, " ");
			}

			if (obj_length + 8 > 60) { // if exceed maximum length of object code line, then print T record
				fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
				obj_length = 0;
			}

			locctr += 4;
			object_code += Find_Opcodemnemonic(tok_i + 1)->opcode * 0x1000000 + 0x100000;

			if (strcmp(tok_i + 1, "RSUB") == 0) { // if opcode is RSUB, it means this line have no label
				object_code += 0x3000000;
			}
			else {
				tok_i = strtok(NULL, "");

				if (strchr(tok_i, ',') == NULL) {
					tok_i = strtok(tok_i, " \t\n");

					if (tok_i == NULL) {
						// If there is unvalid input, then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
						return FALSE;
					}
				}
				else {
					tok_num = sscanf(strchr(tok_i, ',') + 1, " %c %c", &indexed, &temp_char);
					indexed_loc = strchr(tok_i, 'X');
					comma_loc = strchr(tok_i, ',');

					if (tok_num == 1 && indexed == 'X') { // if it is indexed mode ( x = 1 )
						tok_i = strtok(tok_i, " \t\n");

						if (tok_i[0] != '#' && tok_i[0] != '@') {
							object_code += 0x8000;
							indexed_loc[0] = '\0';
							comma_loc[0] = '\0';
						}
						else {
							// If there is unvalid comma in input, or invalid indexed addressing mode ( ex) indexed + indirect, indexed + immediate), then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with indexed addressing mode.");
							return FALSE;
						}
					}
					else {
						// If there is unvalid comma in input, or invalid indexed addressing mode ( ex) indexed + indirect, indexed + immediate), then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with indexed addressing mode.");
						return FALSE;
					}
				}

				if (tok_i[0] == '#') { // if it is immediate ( i = 0 )
					object_code += 0x1000000;
					tok_i = tok_i + 1;
				}
				else if (tok_i[0] == '@') { // if it is indirect ( n = 0 )
					object_code += 0x2000000;
					tok_i = tok_i + 1;
				}
				else { // it it is simple ( n,i = 0 )
					object_code += 0x3000000;
				}

				symbol_temp = Find_Symbol(tok_i);

				if (symbol_temp == NULL) {
					tok_num = sscanf(tok_i, "%d", &temp_value);

					if (tok_num == 1 && (object_code / 0x1000000) % 4 == 1) { // if there is constant ex) #17
						object_code += temp_value;
					}
					else {
						// if there is no such symbol in symbol table, then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No such symbol.");
						return FALSE;
					}
				}
				else {
					object_code += symbol_temp->locctr;

					// if it is modification record, then add address to modification array 
					if ((object_code / 0x1000000) % 4 == 3 || (object_code / 0x1000000) % 4 == 2) {
						if (temp_modified == NULL) {
							temp_modified = (int*)calloc(1, sizeof(int));
							temp_modified[0] = locctr - 3;
							modifed_num++;
						}
						else {
							temp_modified = realloc(temp_modified, sizeof(int) * (modifed_num + 1));
							temp_modified[modifed_num] = locctr - 3;
							modifed_num++;
						}
					}
				}
			}

			if (strtok(NULL, " \t\n") != NULL) {
				// If there is unvalid input, then error ex) +JSUB  WRREC abc
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
				return FALSE;
			}

			sprintf(temp_string + obj_length, "%08X", object_code);
			obj_length += 8;
			fprintf(lst_fp, "\t%08X\n", object_code);
		}
		else if (Find_Opcodemnemonic(tok_i) != NULL) {
			fprintf(lst_fp, "%d\t%04X\t", linectr, locctr);
			for (i = 0; i < strlen(read_line) - 1; i++) {
				fprintf(lst_fp, "%c", read_line[i]);
			}
			for (; i < max_line_length; i++) {
				fprintf(lst_fp, " ");
			}

			if (strcmp(Find_Opcodemnemonic(tok_i)->type, "1") == 0) { // if type is 1
				if (obj_length + 2 > 60) {
					fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
					obj_length = 0;
				}

				locctr += 1;

				object_code += Find_Opcodemnemonic(tok_i)->opcode;

				if (strtok(NULL, " \t\n") != NULL) {
					// If there is unvalid input, then error ex) FIX abc 
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
					return FALSE;
				}

				sprintf(temp_string + obj_length, "%02X", object_code);
				obj_length += 2;
				fprintf(lst_fp, "\t%02X\n", object_code);
			}
			else if (strcmp(Find_Opcodemnemonic(tok_i)->type, "2") == 0) { // if type is 2
				if (obj_length + 4 > 60) {
					fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
					obj_length = 0;
				}

				locctr += 2;

				object_code += Find_Opcodemnemonic(tok_i)->opcode * 0x100;

				if (strcmp(tok_i, "SVC") == 0) { // if input is one number
					tok_i = strtok(NULL, "");

					if (tok_i == NULL) {
						// If there is unvalid input, then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
						return FALSE;
					}

					tok_num = sscanf(tok_i, "%d %c", &temp_value, &temp_char);
					if (tok_num == 1 && temp_value >= 0x0 && temp_value <= 0xF) {
						object_code += temp_value * 0x10;
					}
					else {
						// If there is unvalid input or n size is not between 0x0 to 0xF (r2 = n), then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "N is between 0 to 15.");
						return FALSE;
					}
				}
				else if (strcmp(tok_i, "SHIFTL") == 0 || strcmp(tok_i, "SHIFTR") == 0) { // if input is one register and one number
					tok_i = strtok(NULL, "");

					if (tok_i == NULL) {
						// If there is unvalid input, then error ex) SHIFTR
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
						return FALSE;
					}

					if (strchr(tok_i, ',') != NULL) {
						tok_num = sscanf(strchr(tok_i, ',') + 1, "%d %c", &temp_value, &temp_char);
						if (tok_num == 1 && temp_value >= 0x1 && temp_value <= 0x10) {
							object_code += temp_value - 1;
						}
						else {
							// If there is unvalid input or n size is not between 0x1 to 0x10 (r2 = n-1), then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "N is between 1 to 16.");
							return FALSE;
						}

						tok_i = strtok(tok_i, ", \t\n");

						if (strcmp(tok_i, "A") == 0) {

						}
						else if (strcmp(tok_i, "X") == 0) {
							object_code += 0x10;
						}
						else if (strcmp(tok_i, "L") == 0) {
							object_code += 0x20;
						}
						else if (strcmp(tok_i, "PC") == 0) {
							object_code += 0x80;
						}
						else if (strcmp(tok_i, "SW") == 0) {
							object_code += 0x90;
						}
						else if (strcmp(tok_i, "B") == 0) {
							object_code += 0x30;
						}
						else if (strcmp(tok_i, "S") == 0) {
							object_code += 0x40;
						}
						else if (strcmp(tok_i, "T") == 0) {
							object_code += 0x50;
						}
						else if (strcmp(tok_i, "F") == 0) {
							object_code += 0x60;
						}
						else {
							// If there is unvalid register, then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid register.");
							return FALSE;
						}
					}
					else {
						// If there is no comma, then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No comma(,).");
						return FALSE;
					}
				}
				else if (strcmp(tok_i, "CLEAR") == 0 || strcmp(tok_i, "TIXR") == 0) { // if input is one register
					tok_i = strtok(NULL, " \t\n");

					if (tok_i == NULL) {
						// If there is unvalid input, then error ex) CLEAR
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
						return FALSE;
					}

					if (strcmp(tok_i, "A") == 0) {

					}
					else if (strcmp(tok_i, "X") == 0) {
						object_code += 0x10;
					}
					else if (strcmp(tok_i, "L") == 0) {
						object_code += 0x20;
					}
					else if (strcmp(tok_i, "PC") == 0) {
						object_code += 0x80;
					}
					else if (strcmp(tok_i, "SW") == 0) {
						object_code += 0x90;
					}
					else if (strcmp(tok_i, "B") == 0) {
						object_code += 0x30;
					}
					else if (strcmp(tok_i, "S") == 0) {
						object_code += 0x40;
					}
					else if (strcmp(tok_i, "T") == 0) {
						object_code += 0x50;
					}
					else if (strcmp(tok_i, "F") == 0) {
						object_code += 0x60;
					}
					else {
						// If there is unvalid register, then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid register.");
						return FALSE;
					}

					if (strtok(NULL, " \t\n") != NULL) {
						// If there is unvalid input, then error ex) CLEAR X 123
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
						return FALSE;
					}
				}
				else { // if input is two register
					tok_i = strtok(NULL, "");

					if (tok_i == NULL) {
						// If there is unvalid input, then error 
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
						return FALSE;
					}

					if (strchr(tok_i, ',') != NULL && strchr(tok_i, ',') == strrchr(tok_i, ',')) {
						tok_i = strtok(tok_i, ", \t\n");

						if (strcmp(tok_i, "A") == 0) {

						}
						else if (strcmp(tok_i, "X") == 0) {
							object_code += 0x10;
						}
						else if (strcmp(tok_i, "L") == 0) {
							object_code += 0x20;
						}
						else if (strcmp(tok_i, "PC") == 0) {
							object_code += 0x80;
						}
						else if (strcmp(tok_i, "SW") == 0) {
							object_code += 0x90;
						}
						else if (strcmp(tok_i, "B") == 0) {
							object_code += 0x30;
						}
						else if (strcmp(tok_i, "S") == 0) {
							object_code += 0x40;
						}
						else if (strcmp(tok_i, "T") == 0) {
							object_code += 0x50;
						}
						else if (strcmp(tok_i, "F") == 0) {
							object_code += 0x60;
						}
						else {
							// If there is unvalid register, then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid register.");
							return FALSE;
						}

						tok_i = strtok(NULL, " \t\n");

						if (strcmp(tok_i, "A") == 0) {

						}
						else if (strcmp(tok_i, "X") == 0) {
							object_code += 0x1;
						}
						else if (strcmp(tok_i, "L") == 0) {
							object_code += 0x2;
						}
						else if (strcmp(tok_i, "PC") == 0) {
							object_code += 0x8;
						}
						else if (strcmp(tok_i, "SW") == 0) {
							object_code += 0x9;
						}
						else if (strcmp(tok_i, "B") == 0) {
							object_code += 0x3;
						}
						else if (strcmp(tok_i, "S") == 0) {
							object_code += 0x4;
						}
						else if (strcmp(tok_i, "T") == 0) {
							object_code += 0x5;
						}
						else if (strcmp(tok_i, "F") == 0) {
							object_code += 0x6;
						}
						else {
							// If there is unvalid register, then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid register.");
							return FALSE;
						}

						if (strtok(NULL, " \t\n") != NULL) {
							// If there is unvalid input, then error ex) COMPR A, X 1
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
							return FALSE;
						}
					}
					else {
						// If there is no comma or more than two comma, then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No comma(,) or more than two comma.");
						return FALSE;
					}
				}

				sprintf(temp_string + obj_length, "%04X", object_code);
				obj_length += 4;
				fprintf(lst_fp, "\t%04X\n", object_code);
			}
			else { // if type is 3
				if (obj_length + 6 > 60) {
					fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
					obj_length = 0;
				}

				locctr += 3;
				object_code += Find_Opcodemnemonic(tok_i)->opcode * 0x10000;

				if (strcmp(tok_i, "RSUB") == 0) {
					object_code += 0x30000;
				}
				else {
					tok_i = strtok(NULL, "");

					if (strchr(tok_i, ',') == NULL) {
						tok_i = strtok(tok_i, " \t\n");

						if (tok_i == NULL) {
							// If there is unvalid input, then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
							return FALSE;
						}
					}
					else {
						tok_num = sscanf(strchr(tok_i, ',') + 1, " %c %c", &indexed, &temp_char);
						indexed_loc = strchr(tok_i, 'X');
						comma_loc = strchr(tok_i, ',');

						if (tok_num == 1 && indexed == 'X') { // if it is indexed mode
							tok_i = strtok(tok_i, " \t\n");

							if (tok_i[0] != '#' && tok_i[0] != '@') {
								object_code += 0x8000;
								indexed_loc[0] = '\0';
								comma_loc[0] = '\0';
							}
							else {
								// If there is unvalid comma in input, or invalid indexed addressing mode ( ex) indexed + indirect, indexed + immediate), then error
								Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with indexed addressing mode.");
								return FALSE;
							}
						}
						else {
							// If there is unvalid comma in input, or invalid indexed addressing mode ( ex) indexed + indirect, indexed + immediate), then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with indexed addressing mode.");
							return FALSE;
						}
					}

					if (tok_i[0] == '#') { // if it is immediate
						object_code += 0x10000;
						tok_i = tok_i + 1;
					}
					else if (tok_i[0] == '@') { // if it is indirect
						object_code += 0x20000;
						tok_i = tok_i + 1;
					}
					else { // if it is simple
						object_code += 0x30000;
					}

					symbol_temp = Find_Symbol(tok_i);

					if (symbol_temp == NULL) {
						tok_num = sscanf(tok_i, "%d %c", &temp_value, &temp_char);

						if (tok_num == 1 && (object_code / 0x10000) % 4 == 1) { // if it is constant
							object_code += temp_value;
						}
						else {
							// if there is no such symbol in symbol table, then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No such symbol.");
							return FALSE;
						}
					}
					else {
						temp_value = symbol_temp->locctr - locctr;

						if (temp_value >= -2048 && temp_value <= 2047) { // if it is PC relative
							object_code += 0x2000;
							if (temp_value < 0) {
								object_code += 0x1000 + temp_value;
							}
							else {
								object_code += temp_value;
							}
						}
						else if (reg_b != NONE) { // if it is BASE relative
							temp_value = symbol_temp->locctr - reg_b;

							if (temp_value >= 0 && temp_value <= 4095) {
								object_code += 0x4000 + temp_value;
							}
						}
						else {
							// if cannot calulate address with pc relative and base relative, then error
							Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Cannot calculate address.");
							return FALSE;
						}
					}
				}

				if (strtok(NULL, " \t\n") != NULL) {
					// If there is unvalid input, then error ex) JSUB  WRREC abc
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input.");
					return FALSE;
				}

				sprintf(temp_string + obj_length, "%06X", object_code);
				obj_length += 6;
				fprintf(lst_fp, "\t%06X\n", object_code);
			}
		}
		else if (strcmp(tok_i, "BASE") == 0) { // if input is 'BASE' then get base address
			tok_i = strtok(NULL, " \t\n");

			if (tok_i == NULL) {
				// If there is unvalid input with BASE, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with BASE.");
				return FALSE;
			}

			symbol_temp = Find_Symbol(tok_i);

			if (symbol_temp != NULL) {
				reg_b = symbol_temp->locctr;
			}
			else {
				// if there is no such symbol in symbol table, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No such symbol with BASE.");
				return FALSE;
			}

			if (strtok(NULL, " \t\n") != NULL) {
				// If there is unvalid input with BASE, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with BASE.");
				return FALSE;
			}

			fprintf(lst_fp, "%d\t    \t%s", linectr, read_line);
		}
		else if (strcmp(tok_i, "START") == 0) {
			fprintf(lst_fp, "%d\t%04X\t%s", linectr, locctr, read_line);
		}
		else if (strcmp(tok_i, "END") == 0) { // if input is 'END' then finialize .obj file.
			fprintf(lst_fp, "%d\t    \t%s", linectr, read_line);

			if (obj_length != 0) {
				fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
				obj_length = 0;
			}

			if (temp_modified != NULL) {
				for (i = 0; i < modifed_num; i++) {
					fprintf(obj_fp, "M%06X05\n", temp_modified[i] - start_addr);
				}
			}

			tok_i = strtok(NULL, " \t\n");

			if (tok_i == NULL) {
				// If there is unvalid input, where line with 'END' ex) END
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with END.");
				return FALSE;
			}

			symbol_temp = Find_Symbol(tok_i);

			if (symbol_temp != NULL) {
				fprintf(obj_fp, "E%06X", symbol_temp->locctr);
			}
			else {
				// if there is no such symbol in symbol table, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "No such symbol with END.");
				return FALSE;
			}

			if (strtok(NULL, " \t\n") != NULL) {
				// If there is unvalid input, where line with 'END' ex) END FIRST 123
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with END.");
				return FALSE;
			}

			break;
		}
		else if (strcmp(tok_i, "WORD") == 0) { // if input is 'WORD' 
			fprintf(lst_fp, "%d\t%04X\t", linectr, locctr);
			for (i = 0; i < strlen(read_line) - 1; i++) {
				fprintf(lst_fp, "%c", read_line[i]);
			}
			for (; i < max_line_length; i++) {
				fprintf(lst_fp, " ");
			}

			if (obj_length + 6 > 60) {
				fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
				obj_length = 0;
			}

			locctr += 3;

			tok_i = strtok(NULL, " \t\n");

			if (tok_i == NULL) {
				// If there is unvalid input with WORD, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with WORD.");
				return FALSE;
			}

			tok_num = sscanf(tok_i, "%d %c", &temp_value, &temp_char);

			if (tok_num == 1 && temp_value >= 0 && temp_value <= 0xFFFFFF) {
				object_code = temp_value;
			}
			else {
				// If there is unvalid input with WORD, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with WORD.");
				return FALSE;
			}

			if (strtok(NULL, " \t\n") != NULL) {
				// If there is unvalid input with WORD, then error
				Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with WORD.");
				return FALSE;
			}

			sprintf(temp_string + obj_length, "%06X", object_code);
			obj_length += 6;
			fprintf(lst_fp, "\t%06X\n", object_code);
		}
		else if (strcmp(tok_i, "RESW") == 0) { // if input is 'RESW' 
			fprintf(lst_fp, "%d\t%04X\t%s", linectr, locctr, read_line);

			if (obj_length != 0) {
				fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
				obj_length = 0;
			}

			tok_num = sscanf(strtok(NULL, " \t\n"), "%d", &temp_value);
			locctr += 3 * temp_value;
		}
		else if (strcmp(tok_i, "RESB") == 0) { // if input is 'RESB' 
			fprintf(lst_fp, "%d\t%04X\t%s", linectr, locctr, read_line);

			if (obj_length != 0) {
				fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
				obj_length = 0;
			}

			tok_num = sscanf(strtok(NULL, " \t\n"), "%d", &temp_value);
			locctr += temp_value;
		}
		else if (strcmp(tok_i, "BYTE") == 0) { // if input is 'BYTE' 
			fprintf(lst_fp, "%d\t%04X\t", linectr, locctr);
			for (i = 0; i < strlen(read_line) - 1; i++) {
				fprintf(lst_fp, "%c", read_line[i]);
			}
			for (; i < max_line_length; i++) {
				fprintf(lst_fp, " ");
			}

			fprintf(lst_fp, "\t");

			tok_i = strtok(NULL, "");
			sscanf(tok_i, " %c", &temp_char);

			if (temp_char == 'C') { // if input is character
				tok_i = strchr(tok_i, 'C') + 2;

				for (i = 0; i < strlen(tok_i); i++) {
					if (tok_i[i] == '\'') {
						break;
					}
				}

				if (obj_length + 2 * i > 60) {
					fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
					obj_length = 0;
				}

				for (i = 0; i < strlen(tok_i); i++) {
					if (tok_i[i] == '\'') {
						break;
					}

					sprintf(temp_string + obj_length, "%02X", tok_i[i]);
					obj_length += 2;
					fprintf(lst_fp, "%02X", tok_i[i]);
					locctr += 1;
				}
			}
			else { // if input is hexadecimal
				tok_i = strtok(tok_i, " \t\n");

				for (i = 2; i < strlen(tok_i) - 3; i++) {
					if (tok_i[i] != '0') {
						break;
					}
				}

				if (obj_length + 2 * ((strlen(tok_i) - 1 - i) / 2 + (strlen(tok_i) - 1 - i) % 2) > 60) {
					fprintf(obj_fp, "T%06X%02X%s\n", locctr - obj_length / 2, obj_length / 2, temp_string);
					obj_length = 0;
				}

				locctr += (strlen(tok_i) - 1 - i) / 2 + (strlen(tok_i) - 1 - i) % 2;

				for (; i < strlen(tok_i) - 1; i = i + 2) {
					if ((strlen(tok_i) - 1 - i) % 2 == 1) {
						tok_num = sscanf(tok_i + i, "%1x", &temp_value);
						i--;
					}
					else {
						tok_num = sscanf(tok_i + i, "%2x", &temp_value);
					}

					if (tok_num == 1) {
						sprintf(temp_string + obj_length, "%02X", temp_value);
						obj_length += 2;
						fprintf(lst_fp, "%02X", temp_value);
					}
					else {
						// If input is not number in BYTE, then error
						Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "BYTE 'X' only can hexadecimal number.");
						return FALSE;
					}
				}

				if (strtok(NULL, " \t\n") != NULL) {
					// If there is unvalid input with BYTE, then error
					Error_Assemble(asm_fp, lst_fp, obj_fp, lst_filename, obj_filename, temp_modified, linectr, "Unvalid input with BYTE.");
					return FALSE;
				}
			}

			fprintf(lst_fp, "\n");
		}
	}

	if (temp_modified != NULL) {
		free(temp_modified);
	}
	fclose(asm_fp);
	fclose(lst_fp);
	fclose(obj_fp);

	printf("output file : [%s], [%s]\n", lst_filename, obj_filename);

	Add_History();

	return TRUE;

}
void Error_Assemble(FILE* asm_fp, FILE* lst_fp, FILE* obj_fp, char* lst_filename, char* obj_filename, int* temp_modified, int error_line, char* error_msg) {

	/*
	If there is error in assembly code, then close file pointer
	And if there are .lst and .obj file then remove the file and deallocate symbol table and modified array
	Last, print error line
	*/

	fclose(asm_fp);
	fclose(lst_fp);
	fclose(obj_fp);

	remove(lst_filename);
	remove(obj_filename);

	if (temp_modified != NULL) {
		free(temp_modified);
	}

	Free_Symbol();

	printf("Assemble error at %d line : %s\n", error_line, error_msg);

}
symbol_link* Find_Symbol(char* label) {

	/*
	Return symbol which is equal input label
	If there is no such label in symbol table, then return is NULL
	*/

	symbol_link* symbol_i = symbol_head;

	while (symbol_i != NULL) {
		if (strcmp(label, symbol_i->label) == 0) {
			break;
		}
		symbol_i = symbol_i->next;
	}

	return symbol_i;
}
void Add_Symbol(char* label, int locctr) {

	/*
	Add label to symbol table, and it is also insertion sort by alphabet.
	*/

	symbol_link* symbol_i = NULL;
	symbol_link* symbol_temp;
	symbol_link* symbol_prev;

	symbol_temp = (symbol_link*)calloc(1, sizeof(symbol_link));
	strcpy(symbol_temp->label, label);
	symbol_temp->locctr = locctr;
	symbol_temp->next = NULL;

	if (symbol_head == NULL) {
		symbol_head = symbol_temp;
	}
	else {
		if (strcmp(symbol_temp->label, symbol_head->label) > 0) {
			symbol_temp->next = symbol_head;
			symbol_head = symbol_temp;
		}
		else if (strcmp(symbol_temp->label, symbol_head->label) < 0) {
			symbol_i = symbol_head;
			while (symbol_i != NULL && strcmp(symbol_temp->label, symbol_i->label) < 0) {
				symbol_prev = symbol_i;
				symbol_i = symbol_i->next;
			}

			symbol_temp->next = symbol_prev->next;
			symbol_prev->next = symbol_temp;
		}
	}
}
void Free_Symbol() {

	/*
	Deallocate symbol table. This function is called below conditions
	First, when program end
	Second, when assembler find error in assembly code
	Third, when call assemble command
	*/

	symbol_link* symbol_i = symbol_head;
	symbol_link* symbol_free = NULL;

	while (symbol_i != NULL) {
		symbol_free = symbol_i;
		symbol_i = symbol_i->next;
		free(symbol_free);
	}

	symbol_head = NULL;
}
void Cmd_Symbol() {

	/*
	Print symbol table which is maked by assembler, and then add command to history
	*/

	symbol_link* symbol_i;

	symbol_i = symbol_head;
	while (symbol_i != NULL) {
		printf("\t%s\t%04X\n", symbol_i->label, symbol_i->locctr);
		symbol_i = symbol_i->next;
	}

	Add_History();

}
void Cmd_Progaddr(int prog_start_addr) {

	/* Get program start address, then set program start address and add command to history */

	addr_prog = prog_start_addr;

	Add_History();
}
int Cmd_Loader(int file_num, char* first_file, char* second_file, char* third_file) {

	char read_line[100];
	char tok_line[100];
	int tok_num;
	char temp_string[100];
	int temp_value;
	int loop_cnt;
	int i;

	int addr_load = addr_prog;
	int addr_obj;
	int length_obj;
	char sign_char;
	int reference_idx;
	int end_flag = FALSE;
	int* set_record = NULL;
	int set_num = 0;

	FILE* first_fp;
	FILE* second_fp;
	FILE* third_fp;
	FILE* temp_fp;

	Free_Extern_Symbol();

	switch (file_num) {
	case 3:
		third_fp = fopen(third_file, "r");
		if (third_fp == NULL) {
			// If there is no such object file in directory, then error
			printf("ERROR 08 : There is no such file in directory\n");
			return FALSE;
		}
	case 2:
		second_fp = fopen(second_file, "r");
		if (second_fp == NULL) {
			// If there is no such object file in directory, then error
			printf("ERROR 08 : There is no such file in directory\n");
			return FALSE;
		}
	case 1:
		first_fp = fopen(first_file, "r");
		if (first_fp == NULL) {
			// If there is no such object file in directory, then error
			printf("ERROR 08 : There is no such file in directory\n");
			return FALSE;
		}
	default:
		break;
	}

	/* Pass 1*/
	for (i = 0; i < 300; i++) {
		reference_array[0][i] = NONE;
	}

	for (loop_cnt = 0; loop_cnt < file_num; loop_cnt++) {
		if (loop_cnt == 0) {
			temp_fp = first_fp;
		}
		else if (loop_cnt == 1) {
			temp_fp = second_fp;
		}
		else if (loop_cnt == 2) {
			temp_fp = third_fp;
		}

		while (fgets(read_line, 100, temp_fp) != NULL) {
			strcpy(tok_line, read_line);

			if (read_line[0] == '.') { // if line is comment, then ignore it
				continue;
			}
			else if (read_line[0] == 'H') {
				if (strlen(read_line) != 20) { // if header record is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "header record is wrong");
					return FALSE;
				}

				strncpy(temp_string, strtok(tok_line + 1, " "), 6);
				temp_string[6] = '\0';
				tok_num = sscanf(read_line + 7, "%6x %6x", &addr_obj, &length_obj);

				if (tok_num != 2) { // if header record is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "header record is wrong");
					return FALSE;
				}

				if (Find_Extern_Symbol(read_line[0], temp_string) == NULL) {
					Add_Extern_Symbol(read_line[0], temp_string, addr_load, length_obj);
					reference_array[loop_cnt][1] = addr_load + addr_obj;
				}
				else { // if there is same control section name in extern symbol table, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "already have same control section name");
					return FALSE;
				}
			}
			else if (read_line[0] == 'D') {
				if ((strlen(read_line) - 2) % 12 != 0) { // if define record is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "define record is wrong");
					return FALSE;
				}

				if (strlen(read_line) - 2 > 72) { // if length of define record is too long, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "define record is wrong");
					return FALSE;
				}

				for (i = 1; i < strlen(read_line) - 1; i += 12) {
					strncpy(temp_string, strtok(tok_line + i, " "), 6);
					temp_string[6] = '\0';
					tok_num = sscanf(read_line + i + 6, "%6x", &temp_value);

					if (tok_num != 1) { // if define record is wrong, then error
						Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "define record is wrong");
						return FALSE;
					}

					if (temp_value > length_obj) { // if address of symbol is bigger than length of section, then error
						Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "define record is wrong");
						return FALSE;
					}

					if (Find_Extern_Symbol(read_line[0], temp_string) == NULL) {
						Add_Extern_Symbol(read_line[0], temp_string, addr_load + temp_value, 0);
					}
					else { // if there is same symbol in extern symbol table, then error
						Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "already have same symbol name");
						return FALSE;
					}
				}
			}
			else if (read_line[0] == 'E') {
				end_flag = TRUE;
				break;
			}
			else if (read_line[0] == 'R' || read_line[0] == 'T' || read_line[0] == 'M') {

			}
			else {
				Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "object file is wrong");
				return FALSE;
			}
		}

		addr_load += addr_obj + length_obj;
	}

	if (end_flag == FALSE) { // if there is no end record, then error
		Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "no end record");
		return FALSE;
	}

	/* Pass 2*/
	for (loop_cnt = 0; loop_cnt < file_num; loop_cnt++) {
		if (loop_cnt == 0) {
			temp_fp = first_fp;
		}
		else if (loop_cnt == 1) {
			temp_fp = second_fp;
		}
		else if (loop_cnt == 2) {
			temp_fp = third_fp;
		}

		rewind(temp_fp);
		if (set_record != NULL) {
			free(set_record);
		}
		set_record = NULL;

		while (fgets(read_line, 100, temp_fp) != NULL) {
			strcpy(tok_line, read_line);

			if (read_line[0] == '.') { // if line is comment, then ignore it
				continue;
			}
			else if (read_line[0] == 'H') {
				tok_num = sscanf(read_line + 7, "%6x", &addr_load);
			}
			else if (read_line[0] == 'R') {
				for (i = 1; i < strlen(read_line) - 1; i += 8) {
					tok_num = sscanf(read_line + i, "%2d", &temp_value);

					if (tok_num != 1) { // if refer record is wrong, then error
						Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "refer record is wrong");
						return FALSE;
					}

					strncpy(temp_string, strtok(tok_line + i + 2, " \n"), 6);
					temp_string[6] = '\0';

					if (Find_Extern_Symbol(read_line[0], temp_string) != NULL) {
						if (reference_array[loop_cnt][temp_value] != NONE) { // if there is already same reference number, then error
							Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "already have same reference number");
							return FALSE;
						}
						else {
							reference_array[loop_cnt][temp_value] = Find_Extern_Symbol(read_line[0], temp_string)->address;
						}
					}
					else { // if there is no reference symbol in extern symbol table, then error
						Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "no such reference symbol");
						return FALSE;
					}
				}
			}
			else if (read_line[0] == 'T') {
				tok_num = sscanf(read_line + 1, "%6x%2x", &addr_obj, &length_obj);
				addr_obj -= addr_load;

				if (tok_num != 2) { // if text record is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "text record is wrong");
					return FALSE;
				}

				if (strlen(read_line) - 2 != 8 + 2 * length_obj) { // if length of object code is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "text record is wrong");
					return FALSE;
				}

				for (i = 0; i < 2 * length_obj; i += 2) {
					tok_num = sscanf(read_line + 9 + i, "%2x", &temp_value);

					if (tok_num != 1) { // if text record is wrong, then error
						Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "text record is wrong");
						return FALSE;
					}

					cell_dump[reference_array[loop_cnt][1] + addr_obj + i / 2] = temp_value;

					if (set_record == NULL) {
						set_record = (int*)calloc(1, sizeof(int));
						set_record[0] = addr_obj + i / 2;
						set_num++;
					}
					else {
						set_record = realloc(set_record, sizeof(int) * (set_num + 1));
						set_record[set_num] = addr_obj + i / 2;
						set_num++;
					}
				}
			}
			else if (read_line[0] == 'M') {
				if (strlen(read_line) != 13) { // if modification record is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "modification record is wrong");
					return FALSE;
				}

				tok_num = sscanf(read_line + 1, "%6x%2x%c%2d", &addr_obj, &length_obj, &sign_char, &reference_idx);

				if (tok_num != 4) { // if modification record is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "modification record is wrong");
					return FALSE;
				}

				if (reference_array[loop_cnt][reference_idx] == NONE) { // if there is no such symbol index, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "no such symbol index");
					return FALSE;
				}

				for (i = 0; i < set_num; i++) {
					if (set_record[i] == addr_obj + 2) {
						if (i >= 2) {
							if (set_record[i - 1] == addr_obj + 1 && set_record[i - 2] == addr_obj) {
								break;
							}
						}
					}
				}
				if (i == set_num) { // if modification address is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "modification record is wrong");
					return FALSE;
				}

				temp_value = cell_dump[reference_array[loop_cnt][1] + addr_obj] * 0x10000 + cell_dump[reference_array[loop_cnt][1] + addr_obj + 1] * 0x100 + cell_dump[reference_array[loop_cnt][1] + addr_obj + 2];

				if (sign_char == '+') {
					temp_value += reference_array[loop_cnt][reference_idx];
				}
				else if (sign_char == '-') {
					temp_value -= reference_array[loop_cnt][reference_idx];
				}
				else { // if there is other character in modification record, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "modification record is wrong");
					return FALSE;
				}

				if (length_obj == 5) {
					cell_dump[reference_array[loop_cnt][1] + addr_obj] = cell_dump[reference_array[loop_cnt][1] + addr_obj] / 0x10 * 0x10 + (temp_value % 0x100000) / 0x10000;
				}
				else if (length_obj == 6) {
					cell_dump[reference_array[loop_cnt][1] + addr_obj] = (temp_value % 0x1000000) / 0x10000;
				}
				else { // if length of modfication field is wrong, then error
					Error_Loader(file_num, first_fp, second_fp, third_fp, set_record, "modification record is wrong");
					return FALSE;
				}

				cell_dump[reference_array[loop_cnt][1] + addr_obj + 1] = (temp_value % 0x10000) / 0x100;
				cell_dump[reference_array[loop_cnt][1] + addr_obj + 2] = temp_value % 0x100;
			}
			else if (read_line[0] == 'E') {
				break;
			}
		}
	}

	if (set_record != NULL) {
		free(set_record);
	}

	switch (file_num) {
	case 3:
		fclose(third_fp);
	case 2:
		fclose(second_fp);
	case 1:
		fclose(first_fp);
	default:
		break;
	}

	Print_Extern_Symbol();
	Add_History();

	return TRUE;

}
void Error_Loader(int file_num, FILE* first_fp, FILE* second_fp, FILE* third_fp, int* set_record, char* error_msg) {

	/*
	If there is error in object code, then close file pointer
	And deallocate extren symbol table and record array
	Last, print error message
	*/

	switch (file_num) {
	case 3:
		fclose(third_fp);
	case 2:
		fclose(second_fp);
	case 1:
		fclose(first_fp);
	default:
		break;
	}

	if (set_record != NULL) {
		free(set_record);
	}

	Free_Extern_Symbol();

	printf("Loader error : %s\n", error_msg);

}
void Add_Extern_Symbol(char symbol_flag, char* name_string, int address, int length) {

	/*
	Add control section and extern symbol to extern symbol table
	*/

	extern_symbol_link* extern_symbol_i = NULL;
	extern_symbol_link* extern_symbol_temp;

	extern_symbol_temp = (extern_symbol_link*)calloc(1, sizeof(extern_symbol_link));
	if (symbol_flag == 'H') {
		extern_symbol_temp->symbol_flag = 'H';
		strncpy(extern_symbol_temp->ctrl_sec, name_string, 6);
		extern_symbol_temp->length = length;
	}
	else {
		extern_symbol_temp->symbol_flag = 'D';
		strncpy(extern_symbol_temp->symbol, name_string, 6);
	}
	extern_symbol_temp->address = address;
	extern_symbol_temp->next = NULL;

	// add to extern symbol table
	if (extern_symbol_head == NULL) {
		extern_symbol_head = extern_symbol_temp;
	}
	else {
		extern_symbol_i = extern_symbol_head;
		while (extern_symbol_i->next != NULL) {
			extern_symbol_i = extern_symbol_i->next;
		}
		extern_symbol_i->next = extern_symbol_temp;
	}
}
void Print_Extern_Symbol() {

	/*
	Print extern symbol table which is maked by loader
	*/

	extern_symbol_link* extern_symbol_i = extern_symbol_head;
	length_prog = 0;

	printf("\t%-12s%-12s%-15s%-12s\n", "control", "symbol", "address", "length");
	printf("\t%-12s%-12s\n", "section", "name");
	printf("\t---------------------------------------------\n");

	while (extern_symbol_i != NULL) {
		if (extern_symbol_i->symbol_flag == 'H') {
			printf("\t%-12s%-12s%04X%-11s%04X\n", extern_symbol_i->ctrl_sec, " ", extern_symbol_i->address, " ", extern_symbol_i->length);
			length_prog += extern_symbol_i->length;
		}
		else {
			printf("\t%-12s%-12s%04X\n", " ", extern_symbol_i->symbol, extern_symbol_i->address);
		}

		extern_symbol_i = extern_symbol_i->next;
	}

	printf("\t---------------------------------------------\n");
	printf("\t%-24stotal length%-3s%04X\n", " ", " ", length_prog);
}
void Free_Extern_Symbol() {

	/*
	Deallocate Extern symbol table. This function is called below conditions
	First, when program end
	Second, when loader find error in object code
	Third, when call loader command
	*/

	extern_symbol_link* extern_symbol_i = extern_symbol_head;
	extern_symbol_link* extern_symbol_free = NULL;

	while (extern_symbol_i != NULL) {
		extern_symbol_free = extern_symbol_i;
		extern_symbol_i = extern_symbol_i->next;
		free(extern_symbol_free);
	}

	extern_symbol_head = NULL;
}
extern_symbol_link* Find_Extern_Symbol(char symbol_flag, char* symbol) {

	/*
	Return extern symbol which is equal input symbol
	If there is no such label in symbol table, then return is NULL
	*/

	extern_symbol_link* extern_symbol_i = extern_symbol_head;

	while (extern_symbol_i != NULL) {
		if (symbol_flag == 'H') {
			if (strcmp(symbol, extern_symbol_i->ctrl_sec) == 0) {
				break;
			}
		}
		else if (symbol_flag == 'D' || symbol_flag == 'R') {
			if (strcmp(symbol, extern_symbol_i->symbol) == 0) {
				break;
			}
		}

		extern_symbol_i = extern_symbol_i->next;
	}

	return extern_symbol_i;
}
void Cmd_Break_Point(char flag, int address) {

	if (flag == 'P') {
		Print_Break_Point();
	}
	else if (flag == 'C') {
		Free_Break_Point();
		printf("\t[ok] clear all breakpoints\n");
	}
	else if (flag == 'A') {
		Add_Break_Point(address);
		printf("\t[ok] create breakpoint %X\n", address);
	}

	Add_History();

}
void Add_Break_Point(int address) {

	break_point_link* break_point_i = NULL;
	break_point_link* break_point_temp;
	break_point_link* break_point_prev;

	break_point_temp = (break_point_link*)calloc(1, sizeof(break_point_link));
	break_point_temp->point = address;
	break_point_temp->next = NULL;

	if (break_point_head == NULL) {
		break_point_head = break_point_temp;
	}
	else {
		if (break_point_temp->point < break_point_head->point) {
			break_point_temp->next = break_point_head;
			break_point_head = break_point_temp;
		}
		else if (break_point_temp->point > break_point_head->point) {
			break_point_i = break_point_head;
			while (break_point_i != NULL && break_point_temp->point > break_point_i->point) {
				break_point_prev = break_point_i;
				break_point_i = break_point_i->next;
			}

			break_point_temp->next = break_point_prev->next;
			break_point_prev->next = break_point_temp;
		}
		else {

		}
	}
}
void Print_Break_Point() {

	break_point_link* break_point_i = break_point_head;

	printf("\tbreakpoint\n");
	printf("\t----------\n");

	while (break_point_i != NULL) {
		printf("\t%-5X\n", break_point_i->point);
		break_point_i = break_point_i->next;
	}
}
void Free_Break_Point() {

	break_point_link* break_point_i = break_point_head;
	break_point_link* break_point_free = NULL;

	while (break_point_i != NULL) {
		break_point_free = break_point_i;
		break_point_i = break_point_i->next;
		free(break_point_free);
	}

	break_point_head = NULL;
	break_point_current = NULL;
}
void Cmd_Run() {

	int addr_end_prog;
	int i;
	int target_address;
	int target_value;
	int first_target_register, second_target_register;

	// initialize registers
	for (i = 0; i < 10; i++) {
		if (i == PC) {
			continue;
		}
		else if (i == L) {
			if (cell_reg[i] == NONE || cell_reg[PC] == NONE) { // if run start first or restart
				cell_reg[i] = reference_array[0][1] + length_prog;
			}
		}
		else {
			if (cell_reg[i] == NONE || cell_reg[PC] == NONE) {
				cell_reg[i] = 0;
			}
		}
	}

	if (cell_reg[PC] == NONE) {
		cell_reg[PC] = reference_array[0][1];
	}

	// calculate program start address and end address
	if (break_point_current == NULL) {
		break_point_current = break_point_head;

		if (break_point_head == NULL) { // if there is no break point
			addr_end_prog = reference_array[0][1] + length_prog;
		}
		else { // if run program first time with first break point
			addr_end_prog = break_point_current->point;
		}
	}
	else {
		break_point_current = break_point_current->next;
		if (break_point_current == NULL) { // if run program with end of break point
			addr_end_prog = reference_array[0][1] + length_prog;
		}
		else {
			addr_end_prog = break_point_current->point;
		}
	}

	while (cell_reg[PC] < addr_end_prog) {
		if (0x18 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x1C) { // ADD : A <- (A) + (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] += target_value;
		}
		else if (0x58 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x5C) { // ADDF : F <- (F) + (m..m+5) (floating-point instruction -> cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x90 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x94) { // ADDR : r2 <- (r2) + (r1)
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			cell_reg[second_target_register] += cell_reg[first_target_register];
		}
		else if (0x40 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x44) { // AND : A <- (A) & (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] = cell_reg[A] & target_value;
		}
		else if (0xB4 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xB8) { // CLEAR : r1 <- 0
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			cell_reg[first_target_register] = 0;
		}
		else if (0x28 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x2C) { // COMP : (A) : (m..m+2)
			Calc_Deassemble(3, &target_address, &target_value);

			if (cell_reg[A] > target_value) {
				cell_reg[SW] = GREAT; // '>'
			}
			else if (cell_reg[A] == target_value) {
				cell_reg[SW] = EQUAL; // '='
			}
			else {
				cell_reg[SW] = LESS; // '<'
			}
		}
		else if (0x88 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x8C) { // COMPF : (F) : (m..m+5) (floating-point instruction -> cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0xA0 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xA4) { // COMPR : (r1) : (r2)
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			if (cell_reg[first_target_register] > cell_reg[second_target_register]) {
				cell_reg[SW] = GREAT; // '>'
			}
			else if (cell_reg[first_target_register] == cell_reg[second_target_register]) {
				cell_reg[SW] = EQUAL; // '='
			}
			else {
				cell_reg[SW] = LESS; // '<'
			}
		}
		else if (0x24 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x28) { // DIV : A <- (A) / (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] /= target_value;
		}
		else if (0x64 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x68) { // DIVF : F <- (F) / (m..m+5) (floating-point instruction -> cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x9C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xA0) { // DIVR : r2 <- (r2) / (r1)
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			cell_reg[second_target_register] /= cell_reg[first_target_register];
		}
		else if (0x3C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x40) { // J : PC <- m
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[PC] = target_address;
		}
		else if (0x30 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x34) { // JEQ : PC <- m if CC is set to = 
			Calc_Deassemble(3, &target_address, &target_value);

			if (cell_reg[SW] == EQUAL) {
				cell_reg[PC] = target_address;
			}
		}
		else if (0x34 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x38) { // JGT : PC <- m if CC is set to >
			Calc_Deassemble(3, &target_address, &target_value);

			if (cell_reg[SW] == GREAT) {
				cell_reg[PC] = target_address;
			}
		}
		else if (0x38 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x3C) { // JLT : PC <- m if CC is set to <
			Calc_Deassemble(3, &target_address, &target_value);

			if (cell_reg[SW] == LESS) {
				cell_reg[PC] = target_address;
			}
		}
		else if (0x48 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x4C) { // JSUB : L <- (PC); PC <- m 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[L] = cell_reg[PC];
			cell_reg[PC] = target_address;
		}
		else if (0x00 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x04) { // LDA : A <- (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] = target_value;
		}
		else if (0x68 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x6C) { // LDB : B <- (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[B] = target_value;
		}
		else if (0x50 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x54) { // LDCH : A[rightmost byte] <- (m) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] = cell_reg[A] - cell_reg[A] % 0x100 + cell_dump[target_address];
		}
		else if (0x70 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x74) { // LDF : F <- (m..m+5) (floating-point instruction -> cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x08 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x0C) { // LDL : L <- (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[L] = target_value;
		}
		else if (0x6C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x70) { // LDS : S <- (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[S] = target_value;
		}
		else if (0x74 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x78) { // LDT : T <- (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[T] = target_value;
		}
		else if (0x04 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x08) { // LDX : X <- (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[X] = target_value;
		}
		else if (0xD0 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xD4) { // LPS : Load processor status from information beginning at address m (cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x20 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x24) { // MUL : A <- (A) * (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] *= target_value;
		}
		else if (0x60 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x64) { // MULF : F <- (F) * (m..m+5) (floating-point instruction -> cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x98 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x9C) { // MULR : r2 <- (r2) * (r1)
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			cell_reg[second_target_register] *= cell_reg[first_target_register];
		}
		else if (0x44 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x48) { // OR : A <- (A) | (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] = cell_reg[A] | target_value;
		}
		else if (0xD8 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xDC) { // RD : A[rightmost byte] <- data from device (previleged)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0xAC <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xB0) { // RMO : r2 <- (r1)
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			cell_reg[second_target_register] = cell_reg[first_target_register];
		}
		else if (0x4C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x50) { // RSUB : PC <- (L)
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[PC] = cell_reg[L];
		}
		else if (0xA4 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xA8) { // SHIFTL : r1 <- (r1); left circular shift
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			for (i = 0; i < second_target_register; i++) {
				cell_reg[first_target_register] = cell_reg[first_target_register] << 1;
				cell_reg[first_target_register] = cell_reg[first_target_register] - (cell_reg[first_target_register] % 2) + cell_reg[first_target_register] % 0x10000000 / 0x1000000 % 2;
			}
		}
		else if (0xA8 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xAC) { // SHIFTR : r1 <- (r1); right circular shift
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			for (i = 0; i < second_target_register; i++) {
				cell_reg[first_target_register] = cell_reg[first_target_register] % 0x1000000 + (cell_reg[first_target_register] % 2 * 0x1000000);
				cell_reg[first_target_register] = cell_reg[first_target_register] >> 1;
			}
		}
		else if (0xEC <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xF0) { // SSK : Protection key for addres m <- (A) (cannot express) 
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x0C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x10) { // STA : m..m+2 <- (A) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_dump[target_address] = cell_reg[A] / 0x10000;
			cell_dump[target_address + 1] = cell_reg[A] % 0x10000 / 0x100;
			cell_dump[target_address + 2] = cell_reg[A] % 0x100;
		}
		else if (0x78 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x7C) { // STB : m..m+2 <- (B) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_dump[target_address] = cell_reg[B] / 0x10000;
			cell_dump[target_address + 1] = cell_reg[B] % 0x10000 / 0x100;
			cell_dump[target_address + 2] = cell_reg[B] % 0x100;
		}
		else if (0x54 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x58) { // STCH : m <- (A)[rightmost byte]
			Calc_Deassemble(3, &target_address, &target_value);

			cell_dump[target_address] = cell_reg[A] % 0x100;
		}
		else if (0x80 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x84) { // STF : m..m+5 <- (F)  (floating-point instruction -> cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0xD4 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xD8) { // STI : Interval timer value <- (m..m+2) (cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x14 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x18) { // STL : m..m+2 <- (L) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_dump[target_address] = cell_reg[L] / 0x10000;
			cell_dump[target_address + 1] = cell_reg[L] % 0x10000 / 0x100;
			cell_dump[target_address + 2] = cell_reg[L] % 0x100;
		}
		else if (0x7C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x80) { // STS : m..m+2 <- (S) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_dump[target_address] = cell_reg[S] / 0x10000;
			cell_dump[target_address + 1] = cell_reg[S] % 0x10000 / 0x100;
			cell_dump[target_address + 2] = cell_reg[S] % 0x100;
		}
		else if (0xE8 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xEC) { // STSW : m..m+2 <- (SW) (privileged)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x84 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x88) { // STT : m..m+2 <- (T) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_dump[target_address] = cell_reg[T] / 0x10000;
			cell_dump[target_address + 1] = cell_reg[T] % 0x10000 / 0x100;
			cell_dump[target_address + 2] = cell_reg[T] % 0x100;
		}
		else if (0x10 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x14) { // STX : m..m+2 <- (X) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_dump[target_address] = cell_reg[X] / 0x10000;
			cell_dump[target_address + 1] = cell_reg[X] % 0x10000 / 0x100;
			cell_dump[target_address + 2] = cell_reg[X] % 0x100;
		}
		else if (0x1C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x20) { // SUB : A <- (A) - (m..m+2) 
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[A] -= target_value;
		}
		else if (0x5C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x60) { // SUBF : F <- (F) - (m..m+5) (floating-point instruction -> cannot express)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else if (0x94 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x98) { // SUBR : r2 <- (r2) - (r1)
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			cell_reg[second_target_register] -= cell_reg[first_target_register];
		}
		else if (0xB0 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xB4) { // SVC : Generate SVC interrupt (cannnot express)
			Calc_Deassemble(2, &first_target_register, &second_target_register);
		}
		else if (0xE0 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xE4) { // TD : Test device specified by (m) (previleged)
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[SW] = GREAT; // '<' means ready
		}
		else if (0x2C <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0x30) { // TIX : X <- (X) + 1; (X) : (m..m+2)
			Calc_Deassemble(3, &target_address, &target_value);

			cell_reg[X]++;
			if (cell_reg[X] > target_value) {
				cell_reg[SW] = GREAT; // '>'
			}
			else if (cell_reg[X] == target_value) {
				cell_reg[SW] = EQUAL; // '='
			}
			else {
				cell_reg[SW] = LESS; // '<'
			}
		}
		else if (0xB8 <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xBC) { // TIXR : X <- (X) + 1; (X) : (r1)
			Calc_Deassemble(2, &first_target_register, &second_target_register);

			cell_reg[X]++;
			if (cell_reg[X] > cell_reg[first_target_register]) {
				cell_reg[SW] = GREAT; // '>'
			}
			else if (cell_reg[X] == cell_reg[first_target_register]) {
				cell_reg[SW] = EQUAL; // '='
			}
			else {
				cell_reg[SW] = LESS; // '<'
			}
		}
		else if (0xDC <= cell_dump[cell_reg[PC]] && cell_dump[cell_reg[PC]] < 0xE0) { // WD : Device specified by (m) <- (A)[rightmostbyte] (previleged)
			Calc_Deassemble(3, &target_address, &target_value);
		}
		else {
			cell_reg[PC]++;
		}
	}

	printf("\t\tA : %06X X : %06X\n", cell_reg[A], cell_reg[X]);
	printf("\t\tL : %06X PC: %06X\n", cell_reg[L], cell_reg[PC]);
	printf("\t\tB : %06X S : %06X\n", cell_reg[B], cell_reg[S]);
	printf("\t\tT : %06X\n", cell_reg[T]);

	if (break_point_current != NULL) {
		printf("\tStop at checkpoint[%5X]\n", break_point_current->point);
	}
	else {
		printf("\tEnd Program\n");
		cell_reg[PC] = NONE;
	}

	Add_History();
}
void Calc_Deassemble(int type, int* first_para, int* second_para) {

	int ni;
	int xbpe;
	int negative_sub_num = 0;

	if (type == 3) { // first_para = address, second_para = value
		ni = cell_dump[cell_reg[PC]] % 4;
		xbpe = cell_dump[cell_reg[PC] + 1] / 0x10;
		*first_para = (cell_dump[cell_reg[PC] + 1] % 0x10) * 0x100 + cell_dump[cell_reg[PC] + 2];
		if (*first_para / 0x100 >= 8) {
			negative_sub_num = 0x1000;
		}
		*second_para = 0;

		if (xbpe / 8 == 1) { // if x = 1 (indexed mode)
			*first_para += cell_reg[X];
		}

		if (ni / 2 == 0 && ni % 2 == 0) { // if n = 0, i = 0 (sic) 
			*first_para += xbpe % 8 * 0x1000;
			*second_para = cell_dump[*first_para];
			cell_reg[PC] += 3;
		}
		else {
			if (xbpe % 2 == 1) { // if e = 1  (extended, format 4)
				*first_para = *first_para * 0x100 + cell_dump[cell_reg[PC] + 3] + reference_array[0][1];
				negative_sub_num *= 0x100;
				cell_reg[PC] += 4;
			}
			else {
				cell_reg[PC] += 3;
			}

			if ((xbpe % 8) / 4 == 0 && (xbpe % 4) / 2 == 1) { // if b = 0, p = 1 (pc relatvie)
				*first_para = cell_reg[PC] + *first_para - negative_sub_num;
			}
			else if ((xbpe % 8) / 4 == 1 && (xbpe % 4) / 2 == 0) { // if b = 1, p = 0 (base relative)
				*first_para = cell_reg[B] + *first_para - negative_sub_num;
			}
			else if ((xbpe % 8) / 4 == 0 && (xbpe % 4) / 2 == 0) { // if b = 0, p = 0 (direct addressing)
				*first_para = *first_para - negative_sub_num;
			}

			if (ni / 2 == 1 && ni % 2 == 0) { // if n = 1, i = 0 (indirect)
				*second_para = cell_dump[*first_para] * 0x10000;
				*second_para += cell_dump[*first_para + 1] * 0x100;
				*second_para += cell_dump[*first_para + 2];

				*first_para = *second_para;

				*second_para = cell_dump[*first_para] * 0x10000;
				*second_para += cell_dump[*first_para + 1] * 0x100;
				*second_para += cell_dump[*first_para + 2];
			}
			else if (ni / 2 == 0 && ni % 2 == 1) { // if n = 0, i = 1 (immediate)
				*second_para = *first_para;
			}
			else if (ni / 2 == 1 && ni % 2 == 1) { // if n = 1, i = 1 (simple)
				*second_para = cell_dump[*first_para] * 0x10000;
				*second_para += cell_dump[*first_para + 1] * 0x100;
				*second_para += cell_dump[*first_para + 2];
			}
		}
	}
	else if (type == 2) { // first_para = first_register, second_para = second_register
		*first_para = cell_dump[cell_reg[PC] + 1] / 0x10;
		*second_para = cell_dump[cell_reg[PC] + 1] % 0x10;
		cell_reg[PC] += 2;
	}

}
