#ifdef PLATFORM_CF20
char ps[32] __attribute__((used, section(".platformString"))) = "0;CF20;R=D";
#elif defined PLATFORM_CF21
char ps[32] __attribute__((used, section(".platformString"))) = "0;CF21;R=B1";
#elif defined PLATFORM_ROADRUNER
char ps[32] __attribute__((used, section(".platformString"))) = "0;RR10;R=B";
#elif defined PLATFORM_RZR
char ps[32] __attribute__((used, section(".platformString"))) = "0;RZ10;R=G";
#else
#error PLATFORM not supported, define it by passing it to make. ex. make PLATFORM=CF20
#endif
