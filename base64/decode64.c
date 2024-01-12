#include <stdio.h>
#include "op_base64.h"

int main(int argc, char** argv)
{
    int len = 0;
    char * str = NULL;
    char * str2 = (char*)malloc(256);
    
    FILE * fp = fopen(argv[1], "r");
    FILE * fp2 = fopen("out.jpg", "w+");
    
    while(fgets(str2, 256, fp)){
    
	//fputc('.', stdout);
	
	len = op_base64decode_len(str2);
	str = (char*)malloc(len);
	
	//fprintf(stdout, "%s\n", str2);
    
	op_base64decode(str, str2);
	
	fwrite(str, len-1, 1, fp2);
	
	free(str);
    
    }
    
    fclose(fp); fclose(fp2);
    
    return 0;
}
