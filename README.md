# RISC-V 시뮬레이터
24개의 32비트 RISC-V 명령어(*.s)를 기계어 코드(*.o)로 변환하고 프로그램 카운터 값을 기록한 파일(*.trace)을 생성하는 프로그램

### 구현 명령어
ADD, SUB, ADDI, AND, OR, XOR, ANDI, ORI, XORI, SLL, SRL, SRA, SLLI, SRLI, SRAI, LW, SW, BEQ, BNE, BGE, BLT, JAL, JALR <br/>
EXIT : 코드 실행 종료 명령어, 0xFFFFFFFF로 가정 <br/>
x1-x6레지스터는 1-6 초기값을 가짐, 나머지 레지스터는 0 초기값 <br/>
명령어 대소문자 구분하지 않음.

### 입력 및 출력
terminate 입력시 프로그램 종료 <br/>
파일명.o, 파일명.trace 생성 / 문법오류 발생시 생성하지 않음

### 실행 예시
```
>> Enter Input File Name: test1.s
test1.s 
ADD x7, x1, x2         
SUB x8, x3, x4         
AND x9, x5, x6

test1.o
00000000001000001000001110110011
01000000010000011000010000110011
00000000011000101111010010110011

test1.trace
1000
1004
1008
```
