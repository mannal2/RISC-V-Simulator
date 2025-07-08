#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct inst {
	char command[10];
	char rs1[15];
	char rs2[15];
	char rs3[15];
}inst;

typedef struct _label {
	char* name; //라벨이름
	int number; //라벨이 위치한 명령어줄
	struct _label* next;
}_label;

typedef struct label {
	int count;
	struct _label* head;
}label;

typedef struct _memory {
	int address; //메모리 주소
	int data; //메모리 저장된 데이터
	struct _memory* next;
}_memory;

typedef struct memory {
	int count;
	struct _memory* head;
}memory;

int reg[32];
inst instructions[7000];
int total_instCnt; //총 명령어 개수
int currentPC; //현재 pc, 나중에 +1000해서 계산
label labelHead;
memory memoryHead;
char RRR[][7] = { "add", "sub", "sll", "xor", "srl", "sra", "or", "and" };
char RRI[][7] = { "addi","xori", "ori", "andi", "slli", "srli", "srai", "beq", "bne", "blt", "bge" };
char RIR[][7] = { "lw", "jalr", "sw" };
char RI[][7] = { "jal" };

char* rtrim(char* s) {
	char t[50];
	char* end;

	strcpy(t, s);
	end = t + strlen(t) - 1;
	while (end != t && isspace(*end))
		end--;
	*(end + 1) = '\0';
	s = t;
	return s;
}

char* ltrim(char* s) {
	char* begin;
	begin = s;

	while (*begin != '\0') {
		if (isspace(*begin))
			begin++;
		else {
			s = begin;
			break;
		}
	}
	return s;
}

char* trim(char* str) {
	while (isspace((unsigned char)*str)) str++;

	char* end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	*(end + 1) = '\0';
	return str;
}

int whatInst(char* inst) {
	for (int i = 0; i < 8; i++) {
		if (strcmp(trim(inst), RRR[i]) == 0) return 1;
	}
	for (int i = 0; i < 11; i++) {
		if (strcmp(trim(inst), RRI[i]) == 0) return 2;
	}
	for (int i = 0; i < 3; i++) {
		if (strcmp(trim(inst), RIR[i]) == 0) return 3;
	}
	for (int i = 0; i < 1; i++) {
		if (strcmp(trim(inst), RI[i]) == 0) return 4;
	}
	if (strcmp(trim(inst), "exit") == 0) return 5;

	return 0;
}

void initAll() {
	for (int i = 0; i < 32; i++) reg[i] = 0;
	for (int i = 1; i <= 6; i++) reg[i] = i;
	total_instCnt = 0;
	currentPC = 1000;
	labelHead.head = NULL;
	labelHead.count = 0;
	memoryHead.head = NULL;
	memoryHead.count = 0;
}

void showLabelList(label lab) {
	_label* tmp = lab.head;
	while (tmp != NULL) {
		printf("%s : %d\n", tmp->name, tmp->number);
		tmp = tmp->next;
	}
}

void showMemoryList(memory mem) {
	_memory* tmp = mem.head;
	while (tmp != NULL) {
		printf("%d주소 : %d\n", tmp->address, tmp->data);
		tmp = tmp->next;
	}
}

int findLabel(label lab, char* name) {
	_label* tmp = lab.head;
	while (tmp != NULL) {
		if (strcmp(tmp->name, trim(name)) == 0) return tmp->number;
		else tmp = tmp->next;
	}
	return -1;
}

int findMemory(memory mem, int address) {
	_memory* tmp = mem.head;
	while (tmp != NULL) {
		if (tmp->address == address) return tmp->data;
		else tmp = tmp->next;
	}
	return -1;
}

void addLabel(label* head, char* name, int number) {
	_label* tmp = head->head;
	_label* newLabel = (_label*)malloc(sizeof(_label));
	newLabel->name = name;
	newLabel->number = number;
	newLabel->next = NULL;

	if (head->count == 0) {
		head->head = newLabel;
	}
	else {
		while (tmp->next != NULL) tmp = tmp->next;
		tmp->next = newLabel;
	}
	head->count++;
}

void addMemory(memory* head, int address, int data) {
	_memory* tmp = head->head;
	_memory* newMemory = (_memory*)malloc(sizeof(_memory));
	newMemory->address = address;
	newMemory->data = data;
	newMemory->next = NULL;

	if (head->count == 0) {
		head->head = newMemory;
	}
	else {
		while (tmp->next != NULL) tmp = tmp->next;
		tmp->next = newMemory;
	}
	head->count++;
}

void storeMemory(memory mem, int address, int data) {
	_memory* tmp = mem.head;
	while (tmp != NULL) {
		if (tmp->address == address) {
			tmp->data = data;
			return;
		}
	}
	addMemory(&mem, address, data);
}

void initLabel(label* head) {
	head->count = 0;
	head->head = NULL;
}

void initMemory(memory* head) {
	head->count = 0;
	head->head = NULL;
}

void strlower(char* str) {
	int i = 0;
	while (str[i]) {
		if (str[i] >= 'A' && str[i] <= 'Z') {
			str[i] = str[i] + 32;
		}
		i++;
	}
}

int isBlank(char* str) {
	while (*str) {
		if (!isspace(*str)) {
			return 0;
		}
		str++;
	}
	return 1;
}

int pcToIndex(int pcNum) { return (pcNum - 1000) / 4; }

int regi2int(char* str) {
	int len = strlen(str) - 1;
	char temp[100];
	char* dest = (char*)malloc(sizeof(char) * (len + 1));
	if (!dest) return -1;

	strncpy(dest, str + 1, len);
	dest[len] = '\0';

	int result = atoi(dest);
	sprintf(temp, "%d", result);
	if (strlen(temp) != strlen(dest)) return -1;

	free(dest);
	return result;
}

int isNotRegi(char* str) {
	if (str[0] != 'x') return 1; //레지스터가 아니면
	else {
		int num = regi2int(str);
		if (num == -1 || num < 0 || num > 32) return 1;
		else return 0;
	}
}

int isNotImme(char* str) {
	if (str[0] == 'x') return 1; //숫자가 아니면
	else return 0;
}

int isLabel(char* str) {
	int len = strlen(str);
	for (int i = 0; i < len; i++) {
		if (str[i] == ':') return i;
	}
	return -1;
}

int imme2int(char* str) {
	return atoi(str);
}

char* int2binaryStr(int num, int bit) {
	char* binaryStr = (char*)malloc(bit + 1);
	binaryStr[bit] = '\0';

	for (int i = 0; i < bit; i++) {
		binaryStr[bit - 1 - i] = (num & (1 << i)) ? '1' : '0';
	}
	return binaryStr;
}

int readFile(char* filename) {
	char line[255];

	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		return 1;
	}

	while (fgets(line, sizeof(line), file) != NULL) {
		if (isBlank(line)) continue;
		strlower(line);
		//char* line2 = (char*)malloc(sizeof(char) * strlen(line));
		char* line2 = trim(line);

		int labbb = isLabel(line2);

		if (labbb != -1) {
			char* dest = (char*)malloc(sizeof(char) * (labbb + 1));
			strncpy(dest, line2, labbb);
			dest[labbb] = '\0';
			if (findLabel(labelHead, dest) != -1 || strcmp(dest, "exit")==0) {
				fclose(file);
				return 2;
			}
			addLabel(&labelHead, dest, total_instCnt);
		}
		else {
			char* context = NULL;
			char* ptr = strtok_r(line2, " ", &context);
			if (ptr == NULL || whatInst(ptr) == 0) {
				fclose(file);
				return 2;
			}
			strcpy(instructions[total_instCnt].command, ptr);


			int type = whatInst(ptr);
			ptr = strtok_r(NULL, ",() ", &context);
			int cnt = 0;

			if (type == 1) { //RRR
				while (ptr != NULL) {
					cnt++;
					if (isNotRegi(ptr)) {
						fclose(file);
						return 2;
					}

					if (cnt == 1) strcpy(instructions[total_instCnt].rs1, ptr);
					else if (cnt == 2) strcpy(instructions[total_instCnt].rs2, ptr);
					else if (cnt == 3) strcpy(instructions[total_instCnt].rs3, ptr);
					ptr = strtok_r(NULL, ",() ", &context);

				}
				if (cnt != 3) {
					fclose(file);
					return 2;
				}
			}
			else if (type == 2) { //RRI
				while (ptr != NULL) {
					cnt++;
					if (isNotRegi(ptr) && cnt < 3) {
						fclose(file);
						return 2;
					}
					if (isNotImme(ptr) && cnt == 3) {
						fclose(file);
						return 2;
					}

					if (cnt == 1) strcpy(instructions[total_instCnt].rs1, ptr);
					else if (cnt == 2) strcpy(instructions[total_instCnt].rs2, ptr);
					else if (cnt == 3) strcpy(instructions[total_instCnt].rs3, ptr);
					ptr = strtok_r(NULL, ",() ", &context);

				}
				if (cnt != 3) {
					fclose(file);
					return 2;
				}
			}
			else if (type == 3) { //RIR
				while (ptr != NULL) {
					cnt++;
					if (isNotRegi(ptr) && cnt != 2) {
						fclose(file);
						return 2;
					}
					if (isNotImme(ptr) && cnt == 2) {
						fclose(file);
						return 2;
					}

					if (cnt == 1) strcpy(instructions[total_instCnt].rs1, ptr);
					else if (cnt == 2) strcpy(instructions[total_instCnt].rs2, ptr);
					else if (cnt == 3) strcpy(instructions[total_instCnt].rs3, ptr);
					ptr = strtok_r(NULL, ",() ", &context);

				}
				if (cnt != 3) {
					fclose(file);
					return 2;
				}
			}
			else if (type == 4) {
				while (ptr != NULL) {
					cnt++;
					if (isNotRegi(ptr) && cnt == 1) {
						fclose(file);
						return 2;
					}
					if (isNotImme(ptr) && cnt == 2) {
						fclose(file);
						return 2;
					}

					if (cnt == 1) strcpy(instructions[total_instCnt].rs1, ptr);
					else if (cnt == 2) strcpy(instructions[total_instCnt].rs2, ptr);
					ptr = strtok_r(NULL, ",() ", &context);

				}
				if (cnt != 2) {
					fclose(file);
					return 2;
				}
			}
			else if (type == 5) {

			}
			total_instCnt += 1;
		}

	}

	fclose(file);
	return 0;
}

void main() {

	while (1) {
		char filename[40];
		printf("Enter Input File Name: ");
		scanf("%s", filename);
		if (strcmp(filename, "terminate") == 0) break;

		initAll();
		int n = readFile(filename);
		if (n == 1) {
			printf("Input file does not exist!!\n");
			continue;
		}
		if (n == 2) {
			printf("Syntax Error!!\n");
			continue;
		}

		filename[strlen(filename) - 2] = '\0';
		char ofilename[50];
		char tracefilename[50];

		strncpy(ofilename, filename, sizeof(ofilename) - 1);
		ofilename[sizeof(ofilename) - 1] = '\0';
		strcat(ofilename, ".o");

		strncpy(tracefilename, filename, sizeof(tracefilename) - 1);
		tracefilename[sizeof(tracefilename) - 1] = '\0';
		strcat(tracefilename, ".trace");

		FILE* ofile = fopen(ofilename, "w");
		FILE* tfile = fopen(tracefilename, "w");
		int syntax = 0;

		char ooo[35];

		//.o 출력
		for (int i = 0; i < total_instCnt; i++) {
			char* command = (char*)malloc(sizeof(char) * (strlen(instructions[i].command) + 1));
			strcpy(command, instructions[i].command);

			ooo[0] = '\0';
			if (strcmp(command, "add") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "000", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "sub") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0100000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "000", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "sll") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "001", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "xor") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "100", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "srl") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "101", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "sra") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0100000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "101", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "or") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "110", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "and") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "111", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0110011", 8);
			}
			else if (strcmp(command, "addi") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, int2binaryStr(imm, 12), 12);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "000", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0010011", 8);
			}
			else if (strcmp(command, "xori") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, int2binaryStr(imm, 12), 12);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "100", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0010011", 8);
			}
			else if (strcmp(command, "ori") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, int2binaryStr(imm, 12), 12);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "110", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0010011", 8);
			}
			else if (strcmp(command, "andi") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, int2binaryStr(imm, 12), 12);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "111", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0010011", 8);
			}
			else if (strcmp(command, "slli") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int shamt = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(shamt, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "001", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0010011", 8);
			}
			else if (strcmp(command, "srli") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int shamt = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0000000", 7);
				strncpy(ooo + 7, int2binaryStr(shamt, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "101", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0010011", 8);
			}
			else if (strcmp(command, "srai") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int shamt = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, "0100000", 7);
				strncpy(ooo + 7, int2binaryStr(shamt, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "101", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0010011", 8);
			}
			else if (strcmp(command, "lw") == 0) {
				int rs1 = regi2int(instructions[i].rs3);
				int imm = imme2int(instructions[i].rs2);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, int2binaryStr(imm, 12), 12);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "010", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "0000011", 8);
			}
			else if (strcmp(command, "jalr") == 0) {
				int rs1 = regi2int(instructions[i].rs3);
				int imm = imme2int(instructions[i].rs2);
				int rd = regi2int(instructions[i].rs1);

				strncpy(ooo, int2binaryStr(imm, 12), 12);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "000", 3);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "1100111", 8);
			}
			else if (strcmp(command, "sw") == 0) {
				int rs1 = regi2int(instructions[i].rs3);
				int imm = imme2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs1);

				char* tmp = (char*)malloc(sizeof(char) * 13);
				strncpy(tmp, int2binaryStr(imm, 12), 12);
				strncpy(ooo, tmp, 7);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "010", 3);
				strncpy(ooo + 20, tmp + 7, 5);
				strncpy(ooo + 25, "0100011", 8);
				free(tmp);
			}
			else if (strcmp(command, "beq") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				if (imm == -1) {
					syntax = 1;
					break;
				}
				imm = imm * 4 + 1000;

				int offset = imm - (i * 4 + 1000);

				char* tmp = (char*)malloc(sizeof(char) * 14);
				strncpy(tmp, int2binaryStr(offset, 13), 13);
				strncpy(ooo, tmp, 1);
				strncpy(ooo + 1, tmp + 2, 6);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "000", 3);
				strncpy(ooo + 20, tmp + 8, 4);
				strncpy(ooo + 24, tmp + 1, 1);
				strncpy(ooo + 25, "1100011", 8);
				free(tmp);
			}
			else if (strcmp(command, "bne") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				if (imm == -1) {
					syntax = 1;
					break;
				}
				imm = imm * 4 + 1000;

				int offset = imm - (i * 4 + 1000);

				char* tmp = (char*)malloc(sizeof(char) * 14);
				strncpy(tmp, int2binaryStr(offset, 13), 13);
				strncpy(ooo, tmp, 1);
				strncpy(ooo + 1, tmp + 2, 6);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "001", 3);
				strncpy(ooo + 20, tmp + 8, 4);
				strncpy(ooo + 24, tmp + 1, 1);
				strncpy(ooo + 25, "1100011", 8);
				free(tmp);
			}
			else if (strcmp(command, "blt") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				if (imm == -1) {
					syntax = 1;
					break;
				}
				imm = imm * 4 + 1000;

				int offset = imm - (i * 4 + 1000);

				char* tmp = (char*)malloc(sizeof(char) * 14);
				strncpy(tmp, int2binaryStr(offset, 13), 13);
				strncpy(ooo, tmp, 1);
				strncpy(ooo + 1, tmp + 2, 6);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "100", 3);
				strncpy(ooo + 20, tmp + 8, 4);
				strncpy(ooo + 24, tmp + 1, 1);
				strncpy(ooo + 25, "1100011", 8);
				free(tmp);
			}
			else if (strcmp(command, "bge") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				if (imm == -1) {
					syntax = 1;
					break;
				}
				imm = imm * 4 + 1000;

				int offset = imm - (i * 4 + 1000);

				char* tmp = (char*)malloc(sizeof(char) * 14);
				strncpy(tmp, int2binaryStr(offset, 13), 13);
				strncpy(ooo, tmp, 1);
				strncpy(ooo + 1, tmp + 2, 6);
				strncpy(ooo + 7, int2binaryStr(rs2, 5), 5);
				strncpy(ooo + 12, int2binaryStr(rs1, 5), 5);
				strncpy(ooo + 17, "101", 3);
				strncpy(ooo + 20, tmp + 8, 4);
				strncpy(ooo + 24, tmp + 1, 1);
				strncpy(ooo + 25, "1100011", 8);
				free(tmp);
			}
			else if (strcmp(command, "jal") == 0) {
				int rd = regi2int(instructions[i].rs1);
				int imm = findLabel(labelHead, instructions[i].rs2);
				if (imm == -1) {
					syntax = 1;
					break;
				}
				imm = imm * 4 + 1000;

				int offset = imm - (i * 4 + 1000);

				char* tmp = (char*)malloc(sizeof(char) * 22);
				strncpy(tmp, int2binaryStr(offset, 21), 21);
				strncpy(ooo, tmp, 1);
				strncpy(ooo + 1, tmp + 10, 10);
				strncpy(ooo + 11, tmp + 9, 1);
				strncpy(ooo + 12, tmp + 1, 8);
				strncpy(ooo + 20, int2binaryStr(rd, 5), 5);
				strncpy(ooo + 25, "1101111", 8);
				free(tmp);
			}
			else if (strcmp(trim(command), "exit") == 0) {
				strncpy(ooo, "11111111111111111111111111111111", 33);
			}


			//printf("%s\n", ooo);
			strcat(ooo, "\n");
			fputs(ooo, ofile);
		}
		fclose(ofile);
		//////////////////////////////////////
		if (syntax) {
			fclose(tfile);
			printf("Syntax Error!!\n");
			//파일삭제
			remove(ofilename);
			remove(tracefilename);
			continue;
		}
		///////////////////////////////////////
		while (1) {
			int i = pcToIndex(currentPC);
			int stop = 0;

			//여기서 pc쓰기
			//printf("%d\n", currentPC);
			char text[40];
			sprintf(text, "%d", currentPC);
			strcat(text, "\n");
			fputs(text, tfile);

			char* command = (char*)malloc(sizeof(char) * (strlen(instructions[i].command) + 1));
			strcpy(command, instructions[i].command);

			if (strcmp(command, "add") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] + reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "sub") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] - reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "sll") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] << reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "xor") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] ^ reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "srl") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = (unsigned)reg[rs1] >> reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "sra") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] >> reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "or") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] | reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "and") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] & reg[rs2];
				currentPC += 4;
			}
			else if (strcmp(command, "addi") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] + imm;
				currentPC += 4;
			}
			else if (strcmp(command, "xori") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] ^ imm;
				currentPC += 4;
			}
			else if (strcmp(command, "ori") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] | imm;
				currentPC += 4;
			}
			else if (strcmp(command, "andi") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int imm = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] & imm;
				currentPC += 4;
			}
			else if (strcmp(command, "slli") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int shamt = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] << shamt;
				currentPC += 4;
			}
			else if (strcmp(command, "srli") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int shamt = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = (unsigned)reg[rs1] >> shamt;
				currentPC += 4;
			}
			else if (strcmp(command, "srai") == 0) {
				int rs1 = regi2int(instructions[i].rs2);
				int shamt = imme2int(instructions[i].rs3);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = reg[rs1] >> shamt;
				currentPC += 4;
			}
			else if (strcmp(command, "lw") == 0) {
				int rs1 = regi2int(instructions[i].rs3);
				int imm = imme2int(instructions[i].rs2);
				int rd = regi2int(instructions[i].rs1);

				int memAddress = reg[rs1] + imm;
				reg[rd] = findMemory(memoryHead, memAddress);
				currentPC += 4;
			}
			else if (strcmp(command, "jalr") == 0) {
				int rs1 = regi2int(instructions[i].rs3);
				int imm = imme2int(instructions[i].rs2);
				int rd = regi2int(instructions[i].rs1);

				reg[rd] = currentPC + 4;
				currentPC = (reg[rs1] + imm) & (-1);
			}
			else if (strcmp(command, "sw") == 0) {
				int rs1 = regi2int(instructions[i].rs3);
				int imm = imme2int(instructions[i].rs2);
				int rs2 = regi2int(instructions[i].rs1);

				int memAddress = reg[rs1] + imm;
				storeMemory(memoryHead, memAddress, reg[rs2]);
				currentPC += 4;
			}
			else if (strcmp(command, "beq") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				imm = imm * 4 + 1000;

				if (reg[rs1] == reg[rs2]) {
					currentPC = imm; //라벨로 이동
				}
				else currentPC += 4;
			}
			else if (strcmp(command, "bne") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				imm = imm * 4 + 1000;

				if (reg[rs1] != reg[rs2]) {
					currentPC = imm; //라벨로 이동
				}
				else currentPC += 4;
			}
			else if (strcmp(command, "blt") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				imm = imm * 4 + 1000;

				if (reg[rs1] < reg[rs2]) {
					currentPC = imm; //라벨로 이동
				}
				else currentPC += 4;
			}
			else if (strcmp(command, "bge") == 0) {
				int rs1 = regi2int(instructions[i].rs1);
				int rs2 = regi2int(instructions[i].rs2);
				int imm = findLabel(labelHead, instructions[i].rs3);
				imm = imm * 4 + 1000;

				if (reg[rs1] >= reg[rs2]) {
					currentPC = imm; //라벨로 이동
				}
				else currentPC += 4;
			}
			else if (strcmp(command, "jal") == 0) {
				int rd = regi2int(instructions[i].rs1);
				int imm = findLabel(labelHead, instructions[i].rs2);
				imm = imm * 4 + 1000;

				reg[rd] = currentPC + 4;
				currentPC = imm; //라벨로 이동
			}
			else if (strcmp(trim(command), "exit") == 0) {
				stop = 1;
			}
			reg[0] = 0;
			i = pcToIndex(currentPC);
			if (i >= total_instCnt || stop) {
				fclose(tfile);
				printf("%s, %s created\n", ofilename, tracefilename);
				break;
			}
		}
	}
}