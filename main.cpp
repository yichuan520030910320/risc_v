#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include<string>
#include<cmath>
#include<algorithm>
unsigned char MEM[500000000];
unsigned int reg[33] = {0};
int pc = 0, rdpos, rs1pos, rs2pos, imm, rdpos_data, rs1pos_data, rs2pos_data, fun3, fun7;
int write_rd_data, output;
int menaccess, memload;
int mem_to_load;
int shamt;
string s;//core
extern int tempppc = 0;
 int index1 = 0;
enum cmd_family {
    U_lui, U_auipc, J, I_jair, B, I_l, S, I_addi, R, I_fence, I_ecall
};
cmd_family cmd_type;

int getshamt(string tmp) {
    reverse(tmp.begin(), tmp.end());
    string returnans = "";
    for (int i = 0; i < 5; ++i) {
        returnans += '0';
    }
    for (int i = 0; i < 5; ++i) {
        returnans[i] = tmp[i + 20];
    }
    reverse(returnans.begin(),returnans.end());
    int ans;
    ans = std::stoi(returnans, nullptr, 2);
    return ans;
}

int getfun3(string tmp) {
    reverse(tmp.begin(), tmp.end());
    string returnans = "";
    for (int i = 0; i < 3; ++i) {
        returnans += '0';
    }
    for (int i = 0; i < 3; ++i) {
        returnans[i] = tmp[i + 12];
    }
    int ans;
    reverse(returnans.begin(),returnans.end());
    ans = std::stoi(returnans, nullptr, 2);
    return ans;
}

int getfun7(string tmp) {
    reverse(tmp.begin(), tmp.end());
    string returnans = "";
    for (int i = 0; i < 7; ++i) {
        returnans += '0';
    }
    for (int i = 0; i < 7; ++i) {
        returnans[i] = tmp[i + 25];
    }
    int ans;
    reverse(returnans.begin(),returnans.end());
    ans = std::stoi(returnans, nullptr, 2);
    return ans;
}

cmd_family cmd_TYPE_check(string temp) {
    if (temp == "0110111") return U_lui;
    else if (temp == "0010111") { return U_auipc; }
    else if (temp == "1101111") { return J; }
    else if (temp == "1100111") { return I_jair; }
    else if (temp == "1100011") { return B; }
    else if (temp == "0000011") { return I_l; }
    else if (temp == "0100011") { return S; }
    else if (temp == "0010011") { return I_addi; }
    else if (temp == "0110011") { return R; }
    else if (temp == "0001111") { return I_fence; }
    else if (temp == "1110011") { return I_ecall; }

}

int getimm(string offset, cmd_family cmdtmp) {//todo
    string tmp = "";
    for (int i = 0; i < 32; ++i) {
        tmp += '0';
    }
    if (cmdtmp == U_lui || cmdtmp == U_auipc) {
        tmp = offset;
        for (int i = tmp.length() - 12; i < tmp.length(); ++i) {
            tmp[i] = '0';
        }
        int ans;
        //ans = std::stoi(tmp, nullptr, 2);
        ans=std::strtol(tmp.c_str(), nullptr, 2);
        return ans;

    }
    reverse(offset.begin(), offset.end());
     if (cmdtmp == J) {
         for (int i = 31; i>=20 ; --i) {
             tmp[i] = offset[31];
         }

        for (int i = 10; i >= 1; --i) {
            tmp[i] = offset[i + 20];
        }
        tmp[11] = offset[20];
        for (int i = 19; i >= 12; --i) {
            tmp[i] = offset[i];
        }
    } else if (cmdtmp == B) {
         for (int i = 31; i>=12 ; --i) {
             tmp[i] = offset[31];
         }
        for (int i = 10; i >= 5; --i) {
            tmp[i] = offset[i + 20];
        }
        for (int i = 4; i >= 1; --i) {
            tmp[i] = offset[i + 7];
        }
        tmp[11] = offset[7];
    } else if (cmdtmp == I_l || cmdtmp == I_addi || cmdtmp == I_jair) {
         for (int i = 31; i >=12 ; --i) {
             tmp[i]=offset[31];
         }
        for (int i = 11; i >= 0; --i) {
            tmp[i] = offset[i + 20];
        }
    } else if (cmdtmp == S) {
         for (int i = 31; i >=12 ; --i) {
             tmp[i]=offset[31];
         }
        for (int i = 11; i >= 5; --i) {
            tmp[i] = offset[i + 20];
        }
        for (int i = 4; i >= 0; --i) {
            tmp[i] = offset[i + 7];
        }
    }
     reverse(tmp.begin(),tmp.end());
     int ans;
    ans=std::strtol(tmp.c_str(), nullptr, 2);

    return ans;

}
int cnt=0;
void _IF() {
    tempppc = pc + 4;
       index1 = int(MEM[pc]) + int(MEM[pc + 1] << 8) + int(MEM[pc + 2] << 16) + int(MEM[pc + 3] << 24);

};

void _ID() {//处理出操作类型，寄存器对象，两个原来的对象，一个放入的对象，
    if ((unsigned int) index1 == 267388179) {
        printf("%u\n", reg[10] & 255u);
        exit(0);
    }
    s = "";
    unsigned int a;
    for ( a =(unsigned int) index1; a; a = a / 2)
        s = s + (a % 2 ? '1' : '0');//不断进行相除

    while (1) {
        if (s.length() == 32) break;
        s += '0';
    }
    reverse(s.begin(), s.end());
    //倒置字符串
    const char *sss = s.c_str();
    string cmd = "";
    for (int i = s.length() - 7; i < s.length(); ++i) {
        cmd += s[i];
    }
    cmd_type = cmd_TYPE_check(cmd);
    string rd = "";
    for (int i = s.length() - 12; i < s.length() - 7; ++i) {
        rd += s[i];
    }
    rdpos = std::stoi(rd, nullptr, 2);
    rdpos_data = reg[rdpos];
    imm = getimm(s, cmd_type);
    if (1) {
        string tmp_rs1=s;
        reverse(tmp_rs1.begin(), tmp_rs1.end());
        string tmprs1 = "";
        for (int i = 19; i >= 15; --i) {
            tmprs1 += tmp_rs1[i];
        }
        rs1pos = std::stoi(tmprs1, nullptr, 2);
        rs1pos_data = reg[rs1pos];
    }
    if (cmd_type == B || cmd_type == S || cmd_type == R) {
        string tmp_rs2=s;
        reverse(tmp_rs2.begin(), tmp_rs2.end());
        string tmprs1 = "";
        for (int i = 24; i >= 20; --i) {
            tmprs1 += tmp_rs2[i];
        }
        rs2pos = std::stoi(tmprs1, nullptr, 2);
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
        tempppc = pc+imm;
        return;
    } else if (cmd_type == I_jair) {
        tempppc = (imm + rs1pos_data) & ~1, write_rd_data = pc + 4;
        return;
       // cout<<tempppc<<"  "<<write_rd_data<<"&&&&  in ex jaiir"<<rs1pos_data<<"%%%"<<imm<<endl;
    } else if (cmd_type == B) {
        fun3 = getfun3(s);
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

            if ((signed) rs1pos_data < (signed)rs2pos_data) {
                tempppc = pc + imm;
            }
            return;
          //  cout<<imm<<"  "<<pc<<"  "<<tempppc<<endl;
        } else if (fun3 == 5) {//bge
            if (rs1pos_data >=rs2pos_data) {
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
    } else if (cmd_type == I_l) {//涉及对内存的读写，需要在第四级流水中操作
        fun3 = getfun3(s);
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
        fun3 = getfun3(s);
        if (fun3 == 0) {//addi
            write_rd_data = rs1pos_data + imm;
            return;
           // cout<<write_rd_data<<"&&&"<<imm<<"&&&"<<rs1pos_data<<"&&&"<<endl;
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
            shamt = getshamt(s);
            write_rd_data = rs1pos_data << shamt;
            return;
        } else if (fun3 == 5) {
            shamt = getshamt(s);
            fun7 = getfun7(s);
            if (fun7 == 0) {//srli
                write_rd_data = (unsigned int) rs1pos_data >> shamt;
            } else {//srai
                write_rd_data = rs1pos_data >> shamt;
            }
        }
        return;
    } else if (cmd_type == R) {
        fun3 = getfun3(s);
        if (fun3 == 0) {
            fun7 = getfun7(s);
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
            fun7 = getfun7(s);
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
            write_rd_data = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8) + int(MEM[menaccess + 2] << 16) + int(MEM[menaccess + 3] << 24);
            return;
        } else if (fun3 == 4) {//lbu
            write_rd_data = int(MEM[menaccess]);
            return;
        } else if (fun3 == 5) {//lhu
            write_rd_data = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8);
            return;
        }
    } else if (cmd_type == S) {//todo
        fun3 = getfun3(s);
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

    }  else if (cmd_type == I_l) {
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
            //cout<<temp<<"   1"<<endl;
            scanf("%x",&temp);
           // cin >> std::hex >> temp;
            //cout<<temp<<"   2"<<endl;
            MEM[++address] = temp;
            scanf("%x",&temp);
           // cin >> std::hex >> temp;
            //cout<<temp<<"   3"<<endl;
            MEM[++address] = temp;
            scanf("%x",&temp);
            //cin >> std::hex >> temp;
            // cout<<temp<<"   4"<<endl;
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
