#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __init__ -1
#define DICTSIZE 4096

/*
    Dictionary is represented as linked list.
    Linked lists are represented with this dictNode.
    currentSubstring and nextCharacter variables are
    explained thoroughly in the project report.
    int position is the position of that char array
    in the linked list and struct dictNode* next is
    a pointer to another struct dictNode, which is
    another char array in that linked list.
*/ 
typedef struct dictNode
{
    int currentSubstring;
    int nextCharacter;
    int position;
    struct dictNode* next;
} dictNode;

//  Consider them as the head and the tail pointers of the dictionary.
dictNode *dictionary, *tail;

// adds a node to the end of the dictionary.
void addNode(dictNode* node)
{
    if (dictionary) tail->next = node;
    else dictionary = node;
    tail = node;
    node->next = NULL;
}

// adds every character of the ASCII table to the dictionary.
void init()
{
    dictNode* node;
    for (int i = 0; i < 256; i++)
    {
        node = (dictNode*)malloc(sizeof(dictNode));
        node->currentSubstring = __init__;
        node->nextCharacter = i;
        addNode(node);
    }
}

// simply initializes a new node and adds it to the dictionary. 
void addToDictionary(int current, int next, int position)
{
    dictNode* node = (dictNode*)malloc(sizeof(dictNode));
    node->currentSubstring = current;
    node->nextCharacter = next;
    node->position = position;
    addNode(node);
}

// returns the currentSubstring at a given position in the dictionary
int currentSubstring(int position)
{
    for (dictNode* node = dictionary; node != NULL; node = node->next)
    {
        if (node->position == position) return node->currentSubstring;
    }
    return -1; // if not found
}

// returns the nextCharacter at a given position in the dictionary
int nextCharacter(int position)
{
    for (dictNode* node = dictionary; node != NULL; node = node->next)
    {
        if (node->position == position) return node->nextCharacter;
    }
    return -1; // if not found
}

/*
    checks if the concatenation of the currentSubstring and the
    next character is in the dictionary. If it is, returns the
    position of the concatenation; else, returns -1 instead of 0
    since the found value could be at the position 0.
*/
int lookup(int current, int next)
{
    for (dictNode* node = dictionary; node != NULL; node = node->next)
    {
        if (node->currentSubstring == current && node->nextCharacter == next)
        {
            return node->position;
        }
    }
    return -1;
}


// frees each element of the dictionary
void freeDictionary()
{
    while (dictionary)
    {
        dictNode* temp = dictionary;
        dictionary = dictionary->next;
        free(temp);
    }
    free(dictionary);
}


/*
    Bitwise operations are necessary, for we aim for the
    most space efficient version.
    I am not competent enough to write them on my own,
    so I took "inspiration" from a github repository
    check external links from the project report file.
*/
int leftoverCount = 0, leftoverBits;

int binaryRead(FILE * in) {
    int unpackedData;
    // Check if there are any leftover bits from previous read operation
    if (leftoverCount > 0)
    {
        // Combine the leftover bits with the new byte read from the file
        unpackedData = (leftoverBits << 8) + fgetc(in);
        // Reset leftover count
        leftoverCount = 0;
    }
    else
    {
        // Read the first byte from the file
        unpackedData = fgetc(in);
        // Check if end of file is reached
        if (unpackedData == EOF) return 0;

        // Read the second byte from the file
        int nextData = fgetc(in);
        // Save the last 4 bits as leftover
        leftoverBits = nextData & 0xF;
        // Set the leftover count to 1
        leftoverCount = 1;
        // Combine the first byte with the remaining bits of the second byte
        unpackedData = (unpackedData << 4) + (nextData >> 4);
    }
    return unpackedData;
}

void binaryWrite(FILE* out, int originalData)
{
    // check if there are leftover bits from previous write operation
    if (leftoverCount > 0)
    {
        // pack the leftover bits and the first 8 bits of the original data
        int packedCode = (leftoverBits << 4) + (originalData >> 8);

        // write the packed code and the remaining bits of the original data to the file
        fputc(packedCode, out);
        fputc(originalData, out);

        // reset the leftover count
        leftoverCount = 0;
    } 
    else
    {
        // save the last 4 bits of the original data as leftover
        leftoverBits = originalData & 0xF; 
        // set the leftover count to 1
        leftoverCount = 1;

        // write the first 4 bits of the original data to the file
        fputc(originalData >> 4, out);
    }
}

void compress(FILE* in, FILE* out)
{    
    int prefix = getc(in);
    if (prefix == EOF) return;

    int character, nextCode, index;

    // initializing the dictionary with the alphabet for the .txt files: ASCII
    init();

    // ASCII has 256 characters 0-255, so the next available code is 256.
    nextCode = 256;
    
    // while the end of the data isn't reached,
    while ((character = getc(in)) != (unsigned)EOF)
    {
        // check if prefix+character is already in the dictionary.
        if ((index = lookup(prefix, character)) != -1)
        {
            // if it is, continue the while loop for prefix where prefix = prefix+character 
            prefix = index;
        }
        else
        {
            // if it is not,
            // output the code for prefix
            binaryWrite(out, prefix);
            
            // add prefix+character to dictionary
            if (nextCode < DICTSIZE)
            {
                addToDictionary(prefix, character, nextCode++);
            }
            // update the prefix to be the character after.
            prefix = character;
        }
    }
    // output the code for prefix
    binaryWrite(out, prefix); 

    // writes any remaining bits to the output file by shifting them left by 4 
    if (leftoverCount > 0)
    {
        fputc(leftoverBits << 4, out);
    }
}

// helper recursive function for the void decompress(FILE * inputFile, FILE * outputFile)
int decode(FILE* out, int code)
{
    int character, temp;

    // if the sequence not in ASCII table
    if (code > 255)
    {
        // get the character encoded by the given code
        character = nextCharacter(code);

        // recursive call to decode the rest of the sequence
        temp = decode(out, currentSubstring(code));
    }
    else
    {
        // if the code is in the ASCII table, just write it to the output file
        character = code;
        temp = code;
    }
    fputc(character, out);

    // return the last code used in decoding
    return temp;
}

void decompress(FILE* in, FILE* out)
{
    int previousCode; int currentCode;
    int nextCode = 256;
    int firstChar;
    
    // read the first code from the input file
    previousCode = binaryRead(in);
    
    // check for end of file
    if (previousCode == 0) return;

    // write the first character to the output file
    fputc(previousCode, out);
    
    // while the end of the data isn't reached,
    while ((currentCode = binaryRead(in)) > 0)
    {    
        // check if the current code is already in the dictionary
        if (currentCode >= nextCode)
        {
            // if not, we need to decode the previous code first
            fputc(firstChar = decode(out, previousCode), out);
        }
        else
        {
            // if it is in the dictionary, we can just decode the current code
            firstChar = decode(out, currentCode);
        }
        
        // if the next code is out of bounds of the dictionary,
        if (nextCode < DICTSIZE)
        {
            // add a new code to the string table
            addToDictionary(previousCode, firstChar, nextCode++);
        }
        previousCode = currentCode;
    }

    // free the dictionary
    freeDictionary();
}


/*
    #################
    # RETURN VALUES #
    #################
    -1: Incorrect usage.
    -2: Invalid input filetype.
    -3: Invalid command.
    -4: Can't open file.
     0: Success. 
*/
int main(int argc, char** argv)
{
    // check if correct amount of arguments are provided
    if (argc != 2)
    {
        printf("Usage: <program_call> <input_file>");
        return -1;
    }

    int len = strlen(argv[1]);

    int i;
    // get the index after the . in .txt.
    // will fail if the file has a . in its name.
    for (i = 0; i < len; i++)
    {
        if (argv[1][i] != '.') continue;
        i++;
        break;
    }
    char filetype[] = "txt";

    // after the . we need 3 characters for txt.
    if (len - i != 3)
    {
        printf("Invalid input filetype!\nExpected: .%s\n", filetype);
        return -2;
    }

    // "txt" is "txt\0" so we need 4 characters.
    char extension[4];

    // copy the filename starting from but excluding the .
    // meaning, copy the entered file's type to extension variable
    strcpy(extension, argv[1]+i);

    // if the entered file's type is not the same as the expected .txt 
    if (strcmp(extension, filetype))
    {
        printf("Invalid input filetype!\nExpected: .txt\n");
        return -2;
    }


    // prints out:
    /*
        Operation   Command
        
        Compress    c
        Decompress  d
        >>> 
    */
    printf("Operation\tCommand\n\nCompress\tc\nDecompress\td\n>>> ");

    // need two character space for "c\0" or "d\0" or their capital versions.
    char command[2];
    scanf("%s", command);

    // if the command is something other than compress or decompress 
    if (command[0] != 'd' && command[0] != 'c' && command[0] != 'D' && command[0] != 'C')
    {
        printf("Invalid command!\nExpected: 'c' or 'd'\n");
        return -3;
    }


    // pointers for input and output files.
    FILE* in;
    FILE* out;

    // if we'll perform compression 
    if (command[0] == 'c' || command[0] == 'C')
    {
        // read words from the given file name to the input file.
        in = fopen(argv[1], "r");

        // create/open a compressed book file that we'll write to in binary 
        out = fopen("compressed_book.txt", "w+b");

        // if can't open input file / can't create output file
        if (in == NULL || out == NULL)
        {
            printf("Couldn't open file(s)!\n");
            return -4;
        }

        // LZW compress function call
        compress(in, out);
    }

    // if we'll perform decompression 
    else
    {
        // read binary from the given file name to the input file.
        in = fopen(argv[1], "rb");

        // create/open a decompressed book file to which we'll write words. 
        out = fopen("decompressed_book.txt", "w");

        // if can't open input file / can't create output file
        if (in == NULL || out == NULL)
        {
            printf("Couldn't open file(s)!\n");
            return -4;
        }
        
        // LZW decompress function call
        decompress(in, out);
    }

    // close created files
    fclose(in);
    fclose(out);
    return 0;
}