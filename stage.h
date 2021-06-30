//
// Created by 18303 on 2021/6/29.
//

#ifndef RISC_V_STAGE_H
#define RISC_V_STAGE_H


class IF{
    int cp=0u;
public:
    IF()=default;
    IF(unsigned int x){
      cp=x;
    }
    void run(){

    }
};


#endif //RISC_V_STAGE_H
