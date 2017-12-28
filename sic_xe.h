#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0

#define INPUT_CMD_LENGTH 100 // length of input
#define NONE -1 
#define DUMP_ONELINE_SIZE 0x10
#define DUMP_CELL_SIZE 0x100000 // virtual memory size, 1MByte(16 * 65536) 
#define HASH_TABLE_SIZE 20

#define A 0
#define X 1
#define L 2
#define B 3
#define S 4
#define T 5
#define F 6
#define PC 8
#define SW 9

#define LESS -2
#define EQUAL 0
#define GREAT 2

typedef struct _his_link {
	char his_cmd[INPUT_CMD_LENGTH];
	struct _his_link* next;
}his_link;

typedef struct _hashtb_link {
	int opcode;
	char mnemonic[10];
	char type[10];
	struct _hashtb_link* next;
}hashtb_link;

typedef struct _symbol_link {
	char label[35];
	int locctr;
	struct _symbol_link* next;
}symbol_link;

typedef struct _extern_symbol_link {
	char symbol_flag;
	char ctrl_sec[25];
	char symbol[25];
	int address;
	int length;
	struct _extern_symbol_link* next;
}extern_symbol_link;

typedef struct _break_point_link {
	int point;
	struct _break_point_link* next;
}break_point_link;

void Init_System(); // Initialize system
void Exec_System(); // Execute system
void Final_System(); // Finalize system
void Cmd_Help(); // Print all command which can be executed
void Cmd_Dir(); // Print list of files in current directory
void Cmd_History(); // Print history
void Add_History(); // Add command to history
void Cmd_Dump(int, int); // Print dump memory
void Cmd_Edit(int, int); // Edit dump memory
void Cmd_Fill(int, int, int); // Fill dump memory
void Cmd_Reset(); // Reset dump memory
void Cmd_Opcodelist(); // Print hash table containing opcode
int Hash_Function(char*); // Calculate hash index
hashtb_link* Find_Opcodemnemonic(char*); // Find hash data which correspond to input mnemonic
void Cmd_Opcodemnemonic(hashtb_link*); // Print opcode which is in hash data
void Cmd_Typefilename(FILE*); // Print file data to input
int Cmd_Assemblefilename(char*); // Assemble asm file and make object(.ojb) and listing(.lst) file
void Error_Assemble(FILE*, FILE*, FILE*, char*, char*, int*, int, char*); // Finalize assembler and print error message
symbol_link* Find_Symbol(char*); // Find symbol data which correspond to input label
void Add_Symbol(char*, int); // Add label to symbol table
void Free_Symbol(); // Deallocate symbol table
void Cmd_Symbol(); // Print symbol table
void Cmd_Progaddr(int); // Set program address
int Cmd_Loader(int, char*, char*, char*); // load program to dump memory
void Error_Loader(int, FILE*, FILE*, FILE*, int*, char*);
void Add_Extern_Symbol(char, char*, int, int);
void Print_Extern_Symbol();
void Free_Extern_Symbol();
extern_symbol_link* Find_Extern_Symbol(char, char*);
void Cmd_Break_Point(char, int);
void Add_Break_Point(int);
void Print_Break_Point();
void Free_Break_Point();
void Cmd_Run();
void Calc_Deassemble(int, int*, int*);

his_link* his_head = NULL;
hashtb_link* hashtb_head[HASH_TABLE_SIZE];
symbol_link* symbol_head = NULL;
extern_symbol_link* extern_symbol_head = NULL;
break_point_link* break_point_head = NULL;

char input_cmd[INPUT_CMD_LENGTH];
unsigned char cell_dump[DUMP_CELL_SIZE];
int addr_dump;

int addr_prog;
int length_prog = NONE;
int reference_array[3][100];
int cell_reg[10];
break_point_link* break_point_current = NULL;

