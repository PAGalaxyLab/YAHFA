//
// Created by liuruikai756 on 05/07/2017.
//

#ifndef YAHFA_ENV_H
#define YAHFA_ENV_H

#define pointer_size sizeof(void*)
#define roundUpToPtrSize(v) (v + pointer_size - 1 - (v - 1) % pointer_size)

#endif //YAHFA_ENV_H
