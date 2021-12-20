#include <stdio.h>
 
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: %s <BrainFuck source-file> <output C source-file>\n", argv[0]);
        return 1;
    }
    
    FILE *in = fopen(argv[1], "r");
    FILE *out = fopen(argv[2], "w");
    int c;
    int bufferSize = 30000;
 
    fprintf(out, "#include <stdio.h>\n"
                 "#include <stdlib.h>\n\n"
                 "int main(int argc, char **argv)\n{\n"
                 "\tint *array = (int*) malloc(%d * sizeof(int));\n"
                 "\tint *ptr = array;\n"
                 "\tint ret\n;", bufferSize);
 
    while ((c = getc(in)) != EOF) {
        switch (c) {
            case '>': fprintf(out, "\t\t++ptr;\n"); break;
            case '<': fprintf(out, "\t\t--ptr;\n"); break;
            case '+': fprintf(out, "\t\t++*ptr;\n"); break;
            case '-': fprintf(out, "\t\tif (*ptr) --*ptr;\n"); break;
            case '.': fprintf(out, "\t\tputchar(*ptr); fflush(stdout);\n"); break;
            case ',': fprintf(out, "\t\tret = scanf(\"%cd\", ptr);\n", '%'); break;
            case '[': fprintf(out, "\twhile (*ptr) {\n"); break;
            case ']': fprintf(out, "\t}\n"); break;
            default: break;
        }
    }
    
    fprintf(out, "\n\tfree(array);\n\treturn 0;\n}\n\n");
}