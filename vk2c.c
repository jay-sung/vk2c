#include "vk2c_native.h"

int main()
{
    Vk2cContex_t ctx;
    vk2cMakeContext(&ctx);
    vk2cCreateContext(&ctx);
    vk2cDestroyContext(&ctx);
    return 0;
}