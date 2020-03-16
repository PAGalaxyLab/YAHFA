//
// Created by liuruikai756 on 2020/3/13.
//

#ifndef YAHFA_DL_H
#define YAHFA_DL_H

void *art_dlopen(const char *name, int flags);
void *art_dlsym(void *handle, const char *name);

#endif //YAHFA_DL_H
