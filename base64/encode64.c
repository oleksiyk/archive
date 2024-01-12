#include <stdio.h>
#include "op_base64.h"

int main(int argc, char** argv)
{
    int len = 0;
    char * str = NULL;
    
    if(argc < 2){
	printf("Usage: %s string_to_encode\n", argv[0]);
	return -1;
    }
    
    len = op_base64encode_len(strlen(argv[1]));
    str = (char*)malloc(len);
    
    op_base64encode(str, argv[1], len);
    
    puts(str);
    
    return 0;
}
