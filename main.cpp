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
int predict[4096] = {0};//分支预测四位饱和计数器
int hashpc(int pc) {
    return (pc / 4 + 11) % 4095;
}

int pc = 0, rdpos, rs1pos, rs2pos, imm, rdpos_data, rs1pos_data, rs2pos_data, fun3, fun7;
int reg_to_writenum[33] = {0};
int write_rd_data, output;
int menaccess, memload;
int mem_to_load;
int shamt;
bool is_end = false;
int tempppc = 0;
int index1 = 0;
enum cmd_family {
    U_lui, U_auipc, J, I_jair, B, I_l, S, I_addi, R, I_fence, I_ecall,other//other 用来处理置空操作，用在回传的ID阶段
};
cmd_family cmd_type;

class topass {
public:
    bool oc;
    int rs1_pos, rs2_pos, rd, rs1_data, rs2_data, pc_, imm, fun3, fun7;
    int rd_write_in;
    int core_cmd;
    int menaccess, memload;
    int forward_rdpos_from_ex, forward_rdposdata_from_ex, forward_rs1pos, forward_rs2pos, forward_rs1posdata, forward_rs2posdata, forward_rdpos_from_mem, forward_rdposdata_from_mem;//forward
    bool if_forward_from_ex = false, if_forward_from_mem = false;
    cmd_family forward_from_ex, forward_from_mem;
    cmd_family nowtype;

} IF_ID, ID_EX, EX_MEM, MEM_WB;

bool without_return_from_mem_of_TYPE_B = true;
bool IF_oc = true, ID_oc = false, EX_oc = false, MEM_oc = false, WB_oc = false;

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

int getimm(int index, cmd_family cmdtmp) {
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
        temp += get_bits(index, 19, 12);
        temp <<= 1;
        temp += get_bits(index, 20, 20);
        temp <<= 10;
        temp += get_bits(index, 30, 21);
        temp <<= 1;
        return temp;
    } else if (cmdtmp == B) {
        int temp = get_bits(index, 31, 31);
        if (temp == 1) {
            temp = (temp << 20) - 1;
            temp = temp << 1;
        }
        temp += get_bits(index, 7, 7);
        temp <<= 6;
        temp += get_bits(index, 30, 25);
        temp <<= 4;
        temp += get_bits(index, 11, 8);
        temp <<= 1;
        return temp;
    } else if (cmdtmp == I_l || cmdtmp == I_addi || cmdtmp == I_jair) {
        int temp = get_bits(index, 31, 31);
        if (temp == 1) {
            temp = (temp << 20) - 1;
            temp = temp << 12;
        }
        temp += get_bits(index, 31, 20);
        return temp;
    } else if (cmdtmp == S) {
        int temp = get_bits(index, 31, 31);
        if (temp == 1) {
            temp = (temp << 20) - 1;
            temp = temp << 7;
        }
        temp += get_bits(index, 31, 25);
        temp <<= 5;
        temp += get_bits(index, 11, 7);
        return temp;
    }

}

int cntt = 0;

void _IF() {//处理出指令的十进制表示和可能跳转的位置
    if (ID_oc)return;



   // cout << pc << endl;




    index1 = int(MEM[pc]) + int(MEM[pc + 1] << 8) + int(MEM[pc + 2] << 16) + int(MEM[pc + 3] << 24);
    IF_ID.pc_ = pc;
    IF_ID.core_cmd = index1;
    if ((unsigned int) index1 == 267388179) {
        is_end = true;
        ID_oc = 0;
        return;
    }
    pc += 4;
    ID_oc = 1;
};

void _ID() {//（解析）处理出操作类型，寄存器对象，寄存器的数值，立即数
    if (!ID_oc || EX_oc)return;
    int cmdnum = get_bits(IF_ID.core_cmd, 6, 0);
    cmd_type = cmd_TYPE_check(cmdnum);
    rdpos = get_bits(IF_ID.core_cmd, 11, 7);
    imm = getimm(IF_ID.core_cmd, cmd_type);
    if (1) {
        rs1pos = get_bits(IF_ID.core_cmd, 19, 15);
        rs1pos_data = reg[rs1pos];
    }
    if (cmd_type == B || cmd_type == S || cmd_type == R) {
        rs2pos = get_bits(IF_ID.core_cmd, 24, 20);
        rs2pos_data = reg[rs2pos];
    }

//    if (cmd_type == B && without_return_from_mem_of_TYPE_B) {
//        pc = IF_ID.pc_;
//        EX_oc = 0;
//        ID_oc = 0;
//        without_return_from_mem_of_TYPE_B = true;
//        return;
//    }
    //应该是传入的前传的操纵类型是I_l的时候加上特判
    if (IF_ID.forward_from_ex == I_l) {//直接阻塞有点暴力//可以优化，如果数据不冲突则不阻塞
        EX_oc = 0;
        pc = IF_ID.pc_;
        ID_oc = 0;
        IF_ID.forward_from_ex=other;
        //is_end= false;
        return;
    }



//    if (IF_ID.pc_==4248) {
//        cout << rs1pos_data << "  *** " << rs1pos << " *** " << rs2pos_data << " *** " << rs2pos << "      ";
//        cout<<IF_ID.if_forward_from_mem<<"  "<<IF_ID.if_forward_from_ex<<"    ";
//        cout<<"   type:  "<<IF_ID.forward_from_mem<<"     "<<IF_ID.forward_from_ex<<"   ";
//        cout<< IF_ID.forward_rdpos_from_mem<<"  "<<IF_ID.forward_rdposdata_from_mem<<"   "<<IF_ID.forward_rdpos_from_ex<<"  "<<IF_ID.forward_rdposdata_from_ex<<endl;
//    }



    without_return_from_mem_of_TYPE_B = true;
    if ((cmd_type != U_lui) && (cmd_type != U_auipc) && (cmd_type != J)) {
        if (rs1pos == IF_ID.forward_rdpos_from_mem || rs1pos == IF_ID.forward_rdpos_from_ex) {

//            cout << IF_ID.if_forward_from_mem << "   " << rs1pos << "   " << IF_ID.forward_rdpos_from_mem << "   "
//                 << IF_ID.forward_from_mem << "   " << IF_ID.if_forward_from_ex << "   " << rs1pos << "  "
//                 << IF_ID.forward_rdpos_from_ex << "  " << IF_ID.forward_from_ex << endl;

            if (IF_ID.if_forward_from_mem &&
                rs1pos == IF_ID.forward_rdpos_from_mem &&
                (IF_ID.forward_from_mem == I_l || IF_ID.forward_from_mem == I_addi || IF_ID.forward_from_mem == R ||
                 IF_ID.forward_from_mem == J || IF_ID.forward_from_mem == I_jair || IF_ID.forward_from_mem == U_auipc ||
                 IF_ID.forward_from_mem == U_lui)) {

                rs1pos_data = IF_ID.forward_rdposdata_from_mem;
            }
            if (IF_ID.if_forward_from_ex &&
                rs1pos == IF_ID.forward_rdpos_from_ex &&
                (IF_ID.forward_from_ex == I_l || IF_ID.forward_from_ex == I_addi || IF_ID.forward_from_ex == R ||
                 IF_ID.forward_from_ex == J || IF_ID.forward_from_ex == I_jair || IF_ID.forward_from_ex == U_auipc ||
                 IF_ID.forward_from_ex == U_lui)) {

                rs1pos_data = IF_ID.forward_rdposdata_from_ex;
            }
        }
        if (rs2pos == IF_ID.forward_rdpos_from_mem || rs2pos == IF_ID.forward_rdpos_from_ex) {
            if (IF_ID.if_forward_from_mem &&
                rs2pos == IF_ID.forward_rdpos_from_mem &&
                (IF_ID.forward_from_mem == I_l || IF_ID.forward_from_mem == I_addi || IF_ID.forward_from_mem == R ||
                 IF_ID.forward_from_mem == J || IF_ID.forward_from_mem == I_jair ||
                 IF_ID.forward_from_mem == U_auipc ||
                 IF_ID.forward_from_mem == U_lui)) { rs2pos_data = IF_ID.forward_rdposdata_from_mem; }
            if (IF_ID.if_forward_from_ex &&
                rs2pos == IF_ID.forward_rdpos_from_ex &&
                (IF_ID.forward_from_ex == I_l || IF_ID.forward_from_ex == I_addi || IF_ID.forward_from_ex == R ||
                 IF_ID.forward_from_ex == J || IF_ID.forward_from_ex == I_jair || IF_ID.forward_from_ex == U_auipc ||
                 IF_ID.forward_from_ex == U_lui)) { rs2pos_data = IF_ID.forward_rdposdata_from_ex; }
        }
    }
    //BUG！置零操作，在一般逆序中由于是阻塞所以不用特殊考虑，但是在数据前递中是需要特殊考虑的，原因是按照顺序执行的思路，WB之后零号寄存器置零，但是如果forward，那么传回来的值不一定是0
    if (rs1pos==0) rs1pos_data=0;
    if (rs2pos==0)rs2pos_data=0;
    IF_ID.if_forward_from_mem = false;
    IF_ID.if_forward_from_ex = false;


//    //处理数据冒险
//    if ((cmd_type != U_lui) && (cmd_type != U_auipc) && (cmd_type != J)) {
//        if (cmd_type == S || cmd_type == R || cmd_type == B) {
//            if (reg_to_writenum[rs2pos] > 0 || reg_to_writenum[rs1pos] > 0) {
//                EX_oc = 0;
//                ID_oc = 0;
//                pc = IF_ID.pc_;
//                return;
//            }
//        } else {
//            if (reg_to_writenum[rs1pos] > 0) {
//                pc = IF_ID.pc_;
//                EX_oc = 0;
//                ID_oc = 0;
//                return;
//            }
//        }
//    }
//    if ((cmd_type != B) && (cmd_type != S)) { reg_to_writenum[rdpos]++; }
//







    //处理控制冒险
    if (cmd_type == J) {
        pc = IF_ID.pc_ + imm;
    }
    if (cmd_type == I_jair) {
        pc = (imm + rs1pos_data) & ~1;


    }
    if (cmd_type == B) {
        fun3 = get_bits(IF_ID.core_cmd, 14, 12);
        if (fun3 == 0) {//beq
//            if (IF_ID.pc_==4248) cout<<rs1pos_data<<" "<<rs1pos<<"  "<<rs2pos_data<<"  "<<rs2pos<<endl;
//            if (IF_ID.pc_==4248&&rs2pos_data==rs1pos_data) cout<<"jummmmp "<<endl;
            if ( (predict[hashpc(IF_ID.pc_)] > 1)) {
                pc = IF_ID.pc_ + imm;
                ID_oc = 0;
            }
        } else if (fun3 == 1) {//bne

            if ( (predict[hashpc(IF_ID.pc_)] > 1)) {
                pc = IF_ID.pc_ + imm;
                ID_oc = 0;
            }
        } else if (fun3 == 4) {//blt
            if ((predict[hashpc(IF_ID.pc_)] > 1)) {
                pc = IF_ID.pc_ + imm;
                ID_oc = 0;
            }
        } else if (fun3 == 5) {//bge
            if (rs1pos_data >= rs2pos_data && (predict[hashpc(IF_ID.pc_)] > 1)) {
                pc = IF_ID.pc_ + imm;
                ID_oc = 0;
            }
        } else if (fun3 == 6) {//bltu
            if ( (predict[hashpc(IF_ID.pc_)] > 1)) {
                pc = IF_ID.pc_ + imm;
                ID_oc = 0;
            }
        } else if (fun3 == 7) {//bgeu
            if ( (predict[hashpc(IF_ID.pc_)] > 1)) {
                pc = IF_ID.pc_ + imm;
                ID_oc = 0;
            }
        }
    }
    ID_oc = false;
    EX_oc = true;
    ID_EX.nowtype = cmd_type;
    ID_EX.pc_ = IF_ID.pc_;
    ID_EX.core_cmd = IF_ID.core_cmd;
    ID_EX.rs1_data = rs1pos_data;
    ID_EX.rs2_data = rs2pos_data;
    ID_EX.rs1_pos = rs1pos;
    ID_EX.rs2_pos = rs2pos;
    ID_EX.imm = imm;
    ID_EX.rd = rdpos;
};
int win = 0, defeat = 0;

void _EX() {//执行出要写入寄存器的值，计算已经解析好的值
    if (!EX_oc || MEM_oc) return;
    cmd_type = ID_EX.nowtype;
    if (cmd_type == U_lui) {
        write_rd_data = ID_EX.imm;

    } else if (cmd_type == U_auipc) {
        write_rd_data = ID_EX.imm + ID_EX.pc_;

    } else if (cmd_type == J) {
        write_rd_data = ID_EX.pc_ + 4;
        //  pc = ID_EX.pc_ + imm;

    } else if (cmd_type == I_jair) {
        // pc = (ID_EX.imm + ID_EX.rs1_data) & ~1;
        write_rd_data = ID_EX.pc_ + 4;


    } else if (cmd_type == B) {
        fun3 = get_bits(ID_EX.core_cmd, 14, 12);
        if (fun3 == 0) {//beq

            if (ID_EX.rs1_data == ID_EX.rs2_data) {
                int x = predict[hashpc(ID_EX.pc_)];
                if (predict[hashpc(ID_EX.pc_)] == 0) {//预测失败，阻塞下一步，改变PC
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测成功，二位饱和计数器继续加上
                    win++;
                    predict[hashpc(ID_EX.pc_)]++;
                    EX_oc = 0;
                } else {
                    win++;
                    EX_oc = 0;
                }
//                pc = ID_EX.pc_ + ID_EX.imm;
//                EX_oc = 0, ID_oc = 0;
            } else {
                if (predict[hashpc(ID_EX.pc_)] == 0) {
                    win++;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    win++;
                    predict[hashpc(ID_EX.pc_)]--;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测失败，阻塞下一步，重新进行下一步的执行
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                } else {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                }
            }
        } else if (fun3 == 1) {//bne

            if (ID_EX.rs1_data != ID_EX.rs2_data) {

                if (predict[hashpc(ID_EX.pc_)] == 0) {//预测失败，阻塞下一步，改变PC
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测成功，二位饱和计数器继续加上
                    win++;
                    predict[hashpc(ID_EX.pc_)]++;
                    EX_oc = 0;
                } else {
                    win++;
                    EX_oc = 0;
                }
            } else {
                if (predict[hashpc(ID_EX.pc_)] == 0) {
                    win++;

                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    win++;
                    predict[hashpc(ID_EX.pc_)]--;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测失败，阻塞下一步，重新进行下一步的执行
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                } else {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                }
            }
        } else if (fun3 == 4) {//blt
            if ((signed) ID_EX.rs1_data < (signed) ID_EX.rs2_data) {
                if (predict[hashpc(ID_EX.pc_)] == 0) {//预测失败，阻塞下一步，改变PC
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测成功，二位饱和计数器继续加上
                    win++;
                    predict[hashpc(ID_EX.pc_)]++;
                    EX_oc = 0;
                } else {
                    win++;
                    EX_oc = 0;
                }
            } else {
                if (predict[hashpc(ID_EX.pc_)] == 0) {
                    win++;

                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    win++;
                    predict[hashpc(ID_EX.pc_)]--;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测失败，阻塞下一步，重新进行下一步的执行
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                } else {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                }
            }
        } else if (fun3 == 5) {//bge
            if (ID_EX.rs1_data >= ID_EX.rs2_data) {
                if (predict[hashpc(ID_EX.pc_)] == 0) {//预测失败，阻塞下一步，改变PC
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测成功，二位饱和计数器继续加上
                    win++;
                    predict[hashpc(ID_EX.pc_)]++;
                    EX_oc = 0;
                } else {
                    win++;
                    EX_oc = 0;
                }
            } else {
                if (predict[hashpc(ID_EX.pc_)] == 0) {
                    win++;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    win++;
                    predict[hashpc(ID_EX.pc_)]--;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测失败，阻塞下一步，重新进行下一步的执行
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                } else {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                }
            }
        } else if (fun3 == 6) {//bltu
            if ((unsigned int) ID_EX.rs1_data < (unsigned int) ID_EX.rs2_data) {
                if (predict[hashpc(ID_EX.pc_)] == 0) {//预测失败，阻塞下一步，改变PC
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测成功，二位饱和计数器继续加上
                    win++;
                    predict[hashpc(ID_EX.pc_)]++;
                    EX_oc = 0;
                } else {
                    win++;
                    EX_oc = 0;
                }
            } else {
                if (predict[hashpc(ID_EX.pc_)] == 0) {
                    win++;

                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    win++;
                    predict[hashpc(ID_EX.pc_)]--;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测失败，阻塞下一步，重新进行下一步的执行
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                } else {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                }
            }
        } else if (fun3 == 7) {//bgeu

            if ((unsigned int) ID_EX.rs1_data >= (unsigned int) ID_EX.rs2_data) {
                if (predict[hashpc(ID_EX.pc_)] == 0) {//预测失败，阻塞下一步，改变PC
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]++;
                    pc = ID_EX.pc_ + ID_EX.imm;
                    EX_oc = 0, ID_oc = 0;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测成功，二位饱和计数器继续加上
                    win++;
                    predict[hashpc(ID_EX.pc_)]++;
                    EX_oc = 0;
                } else {
                    win++;
                    EX_oc = 0;
                }
            } else {
                if (predict[hashpc(ID_EX.pc_)] == 0) {
                    win++;
                } else if (predict[hashpc(ID_EX.pc_)] == 1) {
                    win++;
                    predict[hashpc(ID_EX.pc_)]--;
                } else if (predict[hashpc(ID_EX.pc_)] == 2) {//预测失败，阻塞下一步，重新进行下一步的执行
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                } else {
                    defeat++;
                    predict[hashpc(ID_EX.pc_)]--;
                    ID_oc = 0, EX_oc = 0;
                    pc = ID_EX.pc_ + 4;
                }
            }
        }
    } else if (cmd_type == I_l) {//涉及对内存的读写，需要在第四级流水中操作
        fun3 = get_bits(ID_EX.core_cmd, 14, 12);
        if (fun3 == 0) {//lb
            menaccess = ID_EX.rs1_data + ID_EX.imm;
        } else if (fun3 == 1) {//lh
            menaccess = ID_EX.rs1_data + ID_EX.imm;
        } else if (fun3 == 2) {//lw
            menaccess = ID_EX.rs1_data + ID_EX.imm;

        } else if (fun3 == 4) {//lbu
            menaccess = ID_EX.rs1_data + ID_EX.imm;

        } else if (fun3 == 5) {//lhu
            menaccess = ID_EX.rs1_data + ID_EX.imm;

        }

    } else if (cmd_type == S) {//oprater mem
        memload = ID_EX.rs1_data + ID_EX.imm;

    } else if (cmd_type == I_addi) {//write in reg
        fun3 = get_bits(ID_EX.core_cmd, 14, 12);
        if (fun3 == 0) {//addi
            write_rd_data = ID_EX.rs1_data + ID_EX.imm;

        } else if (fun3 == 2) {//slti
            write_rd_data = ID_EX.rs1_data < ID_EX.imm ? 1 : 0;

        } else if (fun3 == 3) {//sltiu
            write_rd_data = ID_EX.rs1_data < (unsigned int) ID_EX.imm ? 1 : 0;

        } else if (fun3 == 4) {//xori
            write_rd_data = ID_EX.rs1_data ^ ID_EX.imm;

        } else if (fun3 == 6) {//ori
            write_rd_data = ID_EX.rs1_data | ID_EX.imm;

        } else if (fun3 == 7) {//andi
            write_rd_data = ID_EX.rs1_data & ID_EX.imm;

        } else if (fun3 == 1) {//slli
            shamt = get_bits(ID_EX.core_cmd, 24, 20);
            write_rd_data = ID_EX.rs1_data << shamt;

        } else if (fun3 == 5) {
            shamt = get_bits(ID_EX.core_cmd, 24, 20);
            fun7 = get_bits(ID_EX.core_cmd, 31, 25);
            if (fun7 == 0) {//srli
                write_rd_data = (unsigned int) ID_EX.rs1_data >> shamt;
            } else {//srai
                write_rd_data = ID_EX.rs1_data >> shamt;
            }
        }

    } else if (cmd_type == R) {
        fun3 = get_bits(ID_EX.core_cmd, 14, 12);
        if (fun3 == 0) {
            fun7 = get_bits(ID_EX.core_cmd, 31, 25);

            if (fun7 == 0) {//add
                write_rd_data = ID_EX.rs1_data + ID_EX.rs2_data;
            } else {//sub
                write_rd_data = ID_EX.rs1_data - ID_EX.rs2_data;
            }

        } else if (fun3 == 1) {//sll
            int remove;
            remove = ID_EX.rs2_data & 0b00011111;
            write_rd_data = ID_EX.rs1_data << remove;

        } else if (fun3 == 2) {//slt
            write_rd_data = ID_EX.rs1_data < ID_EX.rs2_data ? 1 : 0;

        } else if (fun3 == 3) {//sltu
            write_rd_data = (unsigned int) ID_EX.rs1_data < (unsigned int) ID_EX.rs2_data ? 1 : 0;

        } else if (fun3 == 4) {//xor
            write_rd_data = ID_EX.rs2_data ^ ID_EX.rs1_data;

        } else if (fun3 == 5) {
            fun7 = get_bits(ID_EX.core_cmd, 31, 25);
            int remove;
            remove = ID_EX.rs2_data & 0b00011111;
            if (fun7 == 0) {//srl
                write_rd_data = (unsigned int) ID_EX.rs1_data >> remove;
            } else {//sra
                write_rd_data = ID_EX.rs1_data >> remove;
            }
        } else if (fun3 == 6) {//or
            write_rd_data = ID_EX.rs1_data | ID_EX.rs2_data;

        } else if (fun3 == 7) {//and
            write_rd_data = ID_EX.rs1_data & ID_EX.rs2_data;

        }
    }

    EX_MEM.fun3 = fun3;
    EX_MEM.fun7 = fun7;
    EX_MEM.pc_ = ID_EX.pc_;
    EX_MEM.menaccess = menaccess;
    EX_MEM.memload = memload;
    EX_MEM.core_cmd = ID_EX.core_cmd;
    EX_MEM.rd_write_in = write_rd_data;
    EX_MEM.nowtype = ID_EX.nowtype;
    EX_MEM.rs2_pos = ID_EX.rs2_pos;
    EX_MEM.rs1_pos = ID_EX.rs1_pos;
    EX_MEM.rs1_data = ID_EX.rs1_data;
    EX_MEM.rs2_data = ID_EX.rs2_data;
    EX_MEM.nowtype = ID_EX.nowtype;
    EX_MEM.rd = ID_EX.rd;


    if(ID_EX.nowtype == I_l || ID_EX.nowtype == I_addi || ID_EX.nowtype == R ||
     ID_EX.nowtype == J || ID_EX.nowtype == I_jair || ID_EX.nowtype == U_auipc ||
     ID_EX.nowtype == U_lui) {
        IF_ID.forward_from_ex = ID_EX.nowtype;

        IF_ID.forward_rdposdata_from_ex = write_rd_data;
        IF_ID.forward_rdpos_from_ex = ID_EX.rd;
        IF_ID.if_forward_from_ex = true;
    }

    EX_oc = false;
    MEM_oc = true;
};

void _MEM() {//进行对内存的操作，1.从内存读值2.将值写入内存
    if (!MEM_oc || WB_oc) return;
    //pc = tempppc;
    cmd_type = EX_MEM.nowtype;
    menaccess = EX_MEM.menaccess;
    write_rd_data = EX_MEM.rd_write_in;
    index1 = EX_MEM.core_cmd;
    memload = EX_MEM.memload;
    rs2pos_data = EX_MEM.rs2_data;
    rs1pos_data = EX_MEM.rs1_data;
    fun3 = EX_MEM.fun3;
    if (cmd_type == I_l) {//todo  //load reg
        if (fun3 == 0) {//lb
            int tmpnum_ = int(MEM[menaccess]);
            if (tmpnum_ >> 7 == 1) { tmpnum_ = tmpnum_ | 0b11111111111111111111111100000000; }
            write_rd_data = tmpnum_;

        } else if (fun3 == 1) {//lh
            int tmpnum_ = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8);
            if (tmpnum_ >> 15 == 1) { tmpnum_ = tmpnum_ | 0b11111111111111110000000000000000; }
            write_rd_data = tmpnum_;

        } else if (fun3 == 2) {//lw
            write_rd_data = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8) + int(MEM[menaccess + 2] << 16) +
                            int(MEM[menaccess + 3] << 24);
        } else if (fun3 == 4) {//lbu
            write_rd_data = int(MEM[menaccess]);
        } else if (fun3 == 5) {//lhu
            write_rd_data = int(MEM[menaccess]) + int(MEM[menaccess + 1] << 8);
        }
    } else if (cmd_type == S) {//todo
        fun3 = get_bits(index1, 14, 12);//buggg 传index
        if (fun3 == 0) {//sb
            mem_to_load = rs2pos_data & 0b11111111;
            MEM[memload] = mem_to_load;
        } else if (fun3 == 1) {//sh
            mem_to_load = rs2pos_data & 0b1111111111111111;
            int tmpload1 = mem_to_load & 0b11111111;
            MEM[memload] = tmpload1;
            mem_to_load >>= 8;
            int tmpload2 = mem_to_load & 0b11111111;
            MEM[++memload] = tmpload2;

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
    MEM_WB.rd = EX_MEM.rd;
    MEM_WB.rd_write_in = write_rd_data;
    MEM_WB.nowtype = EX_MEM.nowtype;

    without_return_from_mem_of_TYPE_B = false;


    if(EX_MEM.nowtype == I_l || EX_MEM.nowtype == I_addi || EX_MEM.nowtype == R ||
       EX_MEM.nowtype == J || EX_MEM.nowtype == I_jair ||
            EX_MEM.nowtype == U_auipc ||
       EX_MEM.nowtype == U_lui) {
        IF_ID.forward_rdpos_from_mem = MEM_WB.rd;
        IF_ID.forward_rdposdata_from_mem = write_rd_data;
        IF_ID.if_forward_from_mem = true;  IF_ID.forward_from_mem = MEM_WB.nowtype;
    }



    WB_oc = true;
    MEM_oc = false;
};

void _WB() {//完成对rd寄存器的赋值
    if (!WB_oc)return;
    cmd_type = MEM_WB.nowtype;
    rdpos = MEM_WB.rd;
    write_rd_data = MEM_WB.rd_write_in;
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
    if ((cmd_type != B) && (cmd_type != S)) { reg_to_writenum[rdpos]--; }
    reg[0] = 0;
    WB_oc = false;




//    cout<<rs1pos<<"  "<<rs2pos<<" "<<endl;
//    for (int i = 0; i <=31 ; ++i) {
//        cout<<"   reg  "<<i<<"   :"<<reg[i]<<endl;
//    }
};

int main() {
    string input;
    int address = 0;
    //指令的读入
//     freopen("superloop.data", "r", stdin);
//     freopen("out.data", "w", stdout);
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
    while (!is_end || WB_oc || ID_oc || EX_oc || MEM_oc) {
        if (is_end) {
            _WB();
            _MEM();
            _EX();
            _WB();
            _MEM();
            _WB();
            break;
        }
        _WB();
        _MEM();
        _EX();
        _ID();
        _IF();
    }
    printf("%u\n", reg[10] & 255u);
}
