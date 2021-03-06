//#pragma GCC optimize ("Ofast")
#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include<string>
int get_bits(int num, int high, int low) {
    int lownum = (1 << (high - low + 1)) - 1;
    return num >> low & (lownum);
}
unsigned char MEM[500000000];
unsigned int reg[33] = {0};
int pc = 0, rdpos, rs1pos, rs2pos, imm, rdpos_data, rs1pos_data, rs2pos_data, fun3, fun7;
int write_rd_data, output;
int menaccess, memload;
int mem_to_load;
int shamt;
//string s;//core
extern int tempppc = 0;
int index1 = 0;
enum cmd_family {
    U_lui, U_auipc, J, I_jair, B, I_l, S, I_addi, R, I_fence, I_ecall
};
cmd_family cmd_type;
cmd_family cmd_TYPE_check(int temp) {
    if (temp == 0b0110111) return U_lui;
    else if (temp == 0b0010111) { return U_auipc; }
    else if (temp == 0b1101111) { return J; }
    else if (temp == 0b1100111) { return I_jair; }
    else if (temp == 0b1100011) { return B; }
    else if (temp == 0b0000011) { return I_l; }
    else if (temp == 0b0100011) { return S; }
    else if (temp == 0b0010011) { return I_addi; }
    else if (temp == 0b0110011) { return R; }
    else if (temp == 0b0001111) { return I_fence; }
    else if (temp == 0b1110011) { return I_ecall; }
}
int getimm(int index,  cmd_family cmdtmp) {
    if (cmdtmp == U_lui || cmdtmp == U_auipc) {
        int nummm = get_bits(index, 31, 12);
        nummm = nummm << 12;
        int ans = nummm;
        return ans;
    }
    if (cmdtmp == J) {
        int temp = get_bits(index, 31, 31);
        if (temp == 1) {
            temp = (temp << 12) - 1;// 111111111111   12
            temp = temp << 8;
        }
        temp+=get_bits(index,19,12);
        temp<<=1;
        temp+=get_bits(index,20,20);
        temp<<=10;
        temp+=get_bits(index,30,21);
        temp<<=1;
        return temp;
    } else if (cmdtmp == B) {
        int temp = get_bits(index, 31, 31);
        if (temp == 1) {
            temp = (temp << 20) - 1;
            temp = temp << 1;
        }
        temp+=get_bits(index,7,7);
        temp<<=6;
        temp+=get_bits(index,30,25);
        temp<<=4;
        temp+=get_bits(index,11,8);
        temp<<=1;
        return temp;
    } else if (cmdtmp == I_l || cmdtmp == I_addi || cmdtmp == I_jair) {
        int temp = get_bits(index, 31, 31);
        if (temp == 1) {
            temp = (temp << 20) - 1;
            temp = temp << 12;
        }
        temp+=get_bits(index,31,20);
        return temp;
    } else if (cmdtmp == S) {
        int temp = get_bits(index, 31, 31);
        if (temp == 1) {
            temp = (temp << 20) - 1;
            temp = temp << 7;
        }
        temp+=get_bits(index,31,25);
        temp<<=5;
        temp+=get_bits(index,11,7);
        return temp;
    }

}

int cnt = 0;

void _IF() {
    tempppc = pc + 4;
    index1 = int(MEM[pc]) + int(MEM[pc + 1] << 8) + int(MEM[pc + 2] << 16) + int(MEM[pc + 3] << 24);

};

void _ID() {//??????????????????????????????????????????????????????????????????????????????????????????
    if ((unsigned int) index1 == 267388179) {
        printf("%u\n", reg[10] & 255u);
        exit(0);
    }

    int cmdnum = get_bits(index1, 6, 0);
    cmd_type = cmd_TYPE_check(cmdnum);
    rdpos = get_bits(index1, 11, 7);
    rdpos_data = reg[rdpos];
    imm = getimm(index1,  cmd_type);
    if (1) {
        rs1pos = get_bits(index1, 19, 15);
        rs1pos_data = reg[rs1pos];
    }
    if (cmd_type == B || cmd_type == S || cmd_type == R) {
        rs2pos = get_bits(index1, 24, 20);
        rs2pos_data = reg[rs2pos];
    }
};

void _EX() {
    if (cmd_type == U_lui) {
        write_rd_data = imm;
        return;
    } else if (cmd_type == U_auipc) {
        write_rd_data = imm + pc;
        return;
    } else if (cmd_type == J) {
        write_rd_data = pc + 4;
        tempppc = pc + imm;
        return;
    } else if (cmd_type == I_jair) {
        tempppc = (imm + rs1pos_data) & ~1, write_rd_data = pc + 4;
        return;

    } else if (cmd_type == B) {
        fun3 = get_bits(index1, 14, 12);
        if (fun3 == 0) {//beq
            if (rs1pos_data == rs2pos_data) {
                tempppc = pc + imm;
            }
            return;
        } else if (fun3 == 1) {//bne
            if (rs1pos_data != rs2pos_data) {
                tempppc = pc + imm;
            }
            return;
        } else if (fun3 == 4) {//blt

            if ((signed) rs1pos_data < (signed) rs2pos_data) {
                tempppc = pc + imm;
            }
            return;

        } else if (fun3 == 5) {//bge
            if (rs1pos_data >= rs2pos_data) {
                tempppc = pc + imm;
            }
            return;
        } else if (fun3 == 6) {//bltu
            if ((unsigned int) rs1pos_data < (unsigned int) rs2pos_data) {
                tempppc = pc + imm;
            }
            return;
        } else if (fun3 == 7) {//bgeu
            if ((unsigned int) rs1pos_data >= (unsigned int) rs2pos_data) {
                tempppc = pc + imm;
            }
            return;
        }
        return;
    } else if (cmd_type == I_l) {//????????????????????????????????????????????????????????????
        fun3 = get_bits(index1, 14, 12);
        if (fun3 == 0) {//lb
            menaccess = rs1pos_data + imm;
            return;
        } else if (fun3 == 1) {//lh
            menaccess = rs1pos_data + imm;
            return;
        } else if (fun3 == 2) {//lw
            menaccess = rs1pos_data + imm;
            return;
        } else if (fun3 == 4) {//lbu
            menaccess = rs1pos_data + imm;
            return;
        } else if (fun3 == 5) {//lhu
            menaccess = rs1pos_data + imm;
            return;
        }
        return;
    } else if (cmd_type == S) {//oprater mem
        memload = rs1pos_data + imm;
        return;
    } else if (cmd_type == I_addi) {//write in reg
        fun3 = get_bits(index1, 14, 12);
        if (fun3 == 0) {//addi
            write_rd_data = rs1pos_data + imm;
            return;

        } else if (fun3 == 2) {//slti
            write_rd_data = rs1pos_data < imm ? 1 : 0;
            return;
        } else if (fun3 == 3) {//sltiu
            write_rd_data = rs1pos_data < (unsigned int) imm ? 1 : 0;
            return;
        } else if (fun3 == 4) {//xori
            write_rd_data = rs1pos_data ^ imm;
            return;
        } else if (fun3 == 6) {//ori
            write_rd_data = rs1pos_data | imm;
            return;
        } else if (fun3 == 7) {//andi
            write_rd_data = rs1pos_data & imm;
            return;
        } else if (fun3 == 1) {//slli
            shamt = get_bits(index1, 24, 20);
            write_rd_data = rs1pos_data << shamt;
            return;
        } else if (fun3 == 5) {
            shamt = get_bits(index1, 24, 20);
            fun7 = get_bits(index1, 31, 25);
            if (fun7 == 0) {//srli
                write_rd_data = (unsigned int) rs1pos_data >> shamt;
            } else {//srai
                write_rd_data = rs1pos_data >> shamt;
            }
        }
        return;
    } else if (cmd_type == R) {
        fun3 = get_bits(index1, 14, 12);
        if (fun3 == 0) {
            fun7 = get_bits(index1, 31, 25);

            if (fun7 == 0) {//add
                write_rd_data = rs1pos_data + rs2pos_data;
            } else {//sub
                write_rd_data = rs1pos_data - rs2pos_data;
            }
            return;
        } else if (fun3 == 1) {//sll
            int remove;
            remove = rs2pos_data & 0b00011111;
            write_rd_data = rs1pos_data << remove;
            return;
        } else if (fun3 == 2) {//slt
            write_rd_data = rs1pos_data < rs2pos_data ? 1 : 0;
            return;
        } else if (fun3 == 3) {//sltu
            write_rd_data = (unsigned int) rs1pos_data < (unsigned int) rs2pos_data ? 1 : 0;
            return;
        } else if (fun3 == 4) {//xor
            write_rd_data = rs2pos_data ^ rs1pos_data;
            return;
        } else if (fun3 == 5) {
            fun7 = get_bits(index1, 31, 25);
            int remove;
            remove = rs2pos_data & 0b00011111;
            if (fun7 == 0) {//srl
                write_rd_data = (unsigned int) rs1pos_data >> remove;
            } else {//sra
                write_rd_data = rs1pos_data >> remove;
            }
        } else if (fun3 == 6) {//or
            write_rd_data = rs1pos_data | rs2pos_data;
            return;
        } else if (fun3 == 7) {//and
            write_rd_data = rs1pos_data & rs2pos_data;
            return;
        }
    }
};

void _MEM() {
    pc = tempppc;
    if (cmd_type == I_l) {//todo  //load reg
        if (fun3 == 0) {//lb
            int tmpnum_ = int(MEM[menaccess]);
            if (tmpnum_ >> 7 == 1) { tmpnum_ = tmpnum_ | 0b11111111111111111111111100000000; }
            write_rd_data = tmpnum_;
            return;
        } else if (fun3 == 1) {//lh
            int tmpnum_ = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8);
            if (tmpnum_ >> 15 == 1) { tmpnum_ = tmpnum_ | 0b11111111111111110000000000000000; }
            write_rd_data = tmpnum_;
            return;
        } else if (fun3 == 2) {//lw
            write_rd_data = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8) + int(MEM[menaccess + 2] << 16) +
                            int(MEM[menaccess + 3] << 24);
            return;
        } else if (fun3 == 4) {//lbu
            write_rd_data = int(MEM[menaccess]);
            return;
        } else if (fun3 == 5) {//lhu
            write_rd_data = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8);
            return;
        }
    } else if (cmd_type == S) {//todo

        fun3 = get_bits(index1, 14, 12);
        if (fun3 == 0) {//sb
            mem_to_load = rs2pos_data & 0b11111111;
            MEM[memload] = mem_to_load;
            return;
        } else if (fun3 == 1) {//sh
            mem_to_load = rs2pos_data & 0b1111111111111111;
            int tmpload1 = mem_to_load & 0b11111111;
            MEM[memload] = tmpload1;
            mem_to_load >>= 8;
            int tmpload2 = mem_to_load & 0b11111111;
            MEM[++memload] = tmpload2;
            return;
        } else if (fun3 == 2) {//sw
            mem_to_load = rs2pos_data;
            int tmpload1 = mem_to_load & 0b11111111;
            MEM[memload] = tmpload1;
            mem_to_load >>= 8;
            int tmpload2 = mem_to_load & 0b11111111;
            MEM[++memload] = tmpload2;
            mem_to_load >>= 8;
            int tmpload3 = mem_to_load & 0b11111111;
            MEM[++memload] = tmpload3;
            mem_to_load >>= 8;
            int tmpload4 = mem_to_load & 0b11111111;
            MEM[++memload] = tmpload4;
        }
    }
};

void _WB() {
    if (cmd_type == U_lui) {
        reg[rdpos] = write_rd_data;

    } else if (cmd_type == U_auipc) {
        reg[rdpos] = write_rd_data;

    } else if (cmd_type == I_jair) {
        reg[rdpos] = write_rd_data;

    } else if (cmd_type == J) {
        reg[rdpos] = write_rd_data;

    } else if (cmd_type == I_l) {
        reg[rdpos] = write_rd_data;

    } else if (cmd_type == I_addi) {
        reg[rdpos] = write_rd_data;

    } else if (cmd_type == R) {
        reg[rdpos] = write_rd_data;

    }
    reg[0] = 0;
//    cout<<rs1pos<<"  "<<rs2pos<<" "<<endl;
//    for (int i = 0; i <=31 ; ++i) {
//      cout<<"   reg  "<<i<<"   :"<<reg[i]<<endl;
//    }
};


int main() {
    string input;
    int address = 0;
    while (cin >> input) {
        if (input == "*") break;
        if (input[0] == '@') {
            sscanf(input.c_str() + 1, "%x", &address);
        } else {
            int temp;
            sscanf(input.c_str(), "%x", &temp);
            MEM[address] = temp;
            scanf("%x", &temp);
            MEM[++address] = temp;
            scanf("%x", &temp);
            MEM[++address] = temp;
            scanf("%x", &temp);
            MEM[++address] = temp;
            ++address;
        }
    }
    while (1) {
        _IF();
        _ID();
        _EX();
        _MEM();
        _WB();
    }
}
