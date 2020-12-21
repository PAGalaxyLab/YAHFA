//
// Created by liuruikai756 on 05/07/2017.
//

#ifndef YAHFA_TAMPOLINE_H
#define YAHFA_TAMPOLINE_H

extern int SDKVersion;

void setupTrampoline(uint8_t offset);

void *genTrampoline(void *toMethod, void *entrypoint);

#define TRAMPOLINE_SPACE_SIZE 4096 // 4k mem page size

#endif //YAHFA_TAMPOLINE_H
