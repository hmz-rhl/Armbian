rm *.o
gcc -DBSP_DEBUG -c lib/BSP_wrappers.c
gcc -c BSP*.c
gcc BSP*.o -o bsptest -lpthread -lgpiod
