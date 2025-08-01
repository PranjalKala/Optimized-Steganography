#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Needed for strcmp, strcspn

#define BYTE_RANGE 256
#define MAX_PATH_LEN 1024 // Maximum length for file paths

// --- Data Structures (HuffmanNode, CodeTable) remain the same ---
typedef struct HuffmanNode {
    unsigned char data;
    int freq;
    struct HuffmanNode *left, *right;
} HuffmanNode;

typedef struct {
    unsigned char byte;
    char *code;
} CodeTable;


// --- Huffman Node/Tree Functions (createNode, freeHuffmanTree) remain the same ---
HuffmanNode* createNode(unsigned char data, int freq) {
    HuffmanNode* node = (HuffmanNode*)malloc(sizeof(HuffmanNode));
    if (!node) {
        perror("Failed to allocate Huffman node");
        return NULL;
    }
    node->data = data;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

void freeHuffmanTree(HuffmanNode* node) {
    if (!node) return;
    freeHuffmanTree(node->left);
    freeHuffmanTree(node->right);
    free(node);
}

// --- Min-Heap Functions (swap, heapify, buildMinHeap, extractMin) remain the same ---
void swap(HuffmanNode** a, HuffmanNode** b) {
    HuffmanNode* temp = *a;
    *a = *b;
    *b = temp;
}

void heapify(HuffmanNode** heap, int n, int i) {
    int smallest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && heap[left]->freq < heap[smallest]->freq)
        smallest = left;
    if (right < n && heap[right]->freq < heap[smallest]->freq)
        smallest = right;

    if (smallest != i) {
        swap(&heap[i], &heap[smallest]);
        heapify(heap, n, smallest);
    }
}

void buildMinHeap(HuffmanNode** heap, int n) {
    int startIdx = (n / 2) - 1;
    for (int i = startIdx; i >= 0; i--) {
        heapify(heap, n, i);
    }
}

HuffmanNode* extractMin(HuffmanNode** heap, int *n) {
    if (*n <= 0) return NULL;
    HuffmanNode* temp = heap[0];
    heap[0] = heap[*n - 1];
    (*n)--;
    if (*n > 0) {
       heapify(heap, *n, 0);
    }
    return temp;
}

// --- Huffman Tree Building (buildHuffmanTree) remains the same ---
HuffmanNode* buildHuffmanTree(int freq[BYTE_RANGE]) {
    HuffmanNode* heap[BYTE_RANGE];
    int size = 0;

    for (int i = 0; i < BYTE_RANGE; i++) {
        if (freq[i] > 0) {
            heap[size] = createNode((unsigned char)i, freq[i]);
            if (!heap[size]) return NULL;
            size++;
        }
    }

    if (size == 0) return NULL;
    if (size == 1) {
        HuffmanNode* root = createNode(0, heap[0]->freq);
        if (!root) return NULL;
        root->left = heap[0];
        return root;
    }

    buildMinHeap(heap, size);

    while (size > 1) {
        HuffmanNode *left = extractMin(heap, &size);
        HuffmanNode *right = extractMin(heap, &size);
        if (!left || !right) {
             fprintf(stderr, "Heap extraction error.\n");
             if(left) freeHuffmanTree(left);
             return NULL;
        }
        HuffmanNode *internalNode = createNode('$', left->freq + right->freq);
        if (!internalNode) {
            freeHuffmanTree(left);
            freeHuffmanTree(right);
            return NULL;
        }
        internalNode->left = left;
        internalNode->right = right;

        int i = size;
        heap[i] = internalNode;
        size++;
        while (i != 0 && heap[(i - 1) / 2]->freq > heap[i]->freq) {
           swap(&heap[i], &heap[(i - 1) / 2]);
           i = (i - 1) / 2;
       }
    }
    return extractMin(heap, &size);
}


// --- Code Generation & Compression (generateCodesRecursive, generateCodes, freeCodeTable, huffmanCompress) remain the same ---
void generateCodesRecursive(HuffmanNode* root, char* currentCode, int depth, CodeTable* codeTable) {
    if (!root) return;
    if (!root->left && !root->right) {
        currentCode[depth] = '\0';
        int i;
        for(i = 0; i < BYTE_RANGE; i++){
            if(codeTable[i].byte == root->data){
                codeTable[i].code = strdup(currentCode);
                if(!codeTable[i].code) perror("strdup failed in generateCodes");
                break;
            }
        }
        return;
    }
    if (root->left) {
        currentCode[depth] = '0';
        if(depth + 1 < BYTE_RANGE) // Basic bounds check for code buffer
             generateCodesRecursive(root->left, currentCode, depth + 1, codeTable);
    }
    if (root->right) {
        currentCode[depth] = '1';
         if(depth + 1 < BYTE_RANGE) // Basic bounds check
             generateCodesRecursive(root->right, currentCode, depth + 1, codeTable);
    }
}

int generateCodes(HuffmanNode* root, int freq[BYTE_RANGE], CodeTable* codeTable) {
    for (int i = 0; i < BYTE_RANGE; i++) {
        codeTable[i].byte = (unsigned char)i;
        codeTable[i].code = NULL;
    }
    if (root && !root->right && root->left && !root->left->left && !root->left->right) {
         codeTable[root->left->data].code = strdup("0");
         if (!codeTable[root->left->data].code) {
             perror("strdup failed for single char");
             return 0;
         }
    } else if (root) {
        char currentCode[BYTE_RANGE];
        generateCodesRecursive(root, currentCode, 0, codeTable);
    }
    return 1;
}

void freeCodeTable(CodeTable* table) {
    if (!table) return;
    for (int i = 0; i < BYTE_RANGE; i++) {
        free(table[i].code);
        table[i].code = NULL;
    }
}

unsigned char* huffmanCompress(unsigned char* input, long fileSize, long* outSize, int freq[BYTE_RANGE]) {
    HuffmanNode* root = buildHuffmanTree(freq);
    if (!root && fileSize > 0) {
        fprintf(stderr, "Failed to build Huffman tree.\n");
        return NULL;
    }
     if (!root && fileSize == 0) {
        *outSize = 0;
        return NULL;
    }

    CodeTable codeTable[BYTE_RANGE];
    if (!generateCodes(root, freq, codeTable)) {
        fprintf(stderr, "Failed to generate Huffman codes.\n");
        freeHuffmanTree(root);
        return NULL;
    }

    long totalBits = 0;
    for (long i = 0; i < fileSize; i++) {
        if (codeTable[input[i]].code) {
            totalBits += strlen(codeTable[input[i]].code);
        } else {
             if (root && root->left && !root->right && root->left->data == input[i]) {
                 totalBits += 1;
             } else {
                fprintf(stderr, "Warning: No code found for byte %d (freq %d) during compression.\n", input[i], freq[input[i]]);
             }
        }
    }
    *outSize = totalBits;

    if (totalBits == 0) {
        freeCodeTable(codeTable);
        freeHuffmanTree(root);
        return NULL;
    }

    unsigned char* bitStream = (unsigned char*)malloc(totalBits * sizeof(unsigned char));
    if (!bitStream) {
        perror("Failed to allocate memory for bit stream");
        freeCodeTable(codeTable);
        freeHuffmanTree(root);
        return NULL;
    }

    long bitIndex = 0;
    for (long i = 0; i < fileSize; i++) {
        char* codeStr = codeTable[input[i]].code;
         if (!codeStr && root && root->left && !root->right && root->left->data == input[i]) {
             codeStr = "0";
         }

        if (codeStr) {
            while (*codeStr) {
                if (bitIndex < totalBits) {
                    bitStream[bitIndex++] = (*codeStr == '1') ? 1 : 0;
                }
                codeStr++;
            }
        }
    }

    freeCodeTable(codeTable);
    freeHuffmanTree(root);

    return bitStream;
}


// --- File I/O and LSB Steganography (readBinaryFile, encodeBinaryIntoImage) remain the same ---
unsigned char* readBinaryFile(const char* filePath, long* fileSize) {
    FILE* file = fopen(filePath, "rb");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", filePath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *fileSize = ftell(file);
    if (*fileSize < 0) {
        perror("ftell error");
        fclose(file);
        return NULL;
    }
    if (*fileSize == 0) {
        printf("Input file '%s' is empty.\n", filePath);
        fclose(file);
        *fileSize = 0; // Ensure fileSize is 0 for caller
        return NULL;
    }
    fseek(file, 0, SEEK_SET);

    unsigned char* buffer = (unsigned char*)malloc(*fileSize);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for reading file!\n");
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, *fileSize, file);
    if (bytesRead != (size_t)*fileSize) {
         fprintf(stderr, "Error reading file content.\n");
         free(buffer);
         fclose(file);
         return NULL;
    }
    fclose(file);
    return buffer;
}

void encodeBinaryIntoImage(const char *imagePath, const char *binaryFilePath, const char *outputPath) {
    FILE *image = NULL, *output = NULL;
    unsigned char* inputData = NULL;
    unsigned char* bitStream = NULL;

    long originalFileSize = 0; // Initialize to 0
    inputData = readBinaryFile(binaryFilePath, &originalFileSize);
    if (!inputData && originalFileSize > 0) {
        fprintf(stderr, "Failed to read file to hide.\n");
        return;
    }
    // If inputData is NULL and originalFileSize is 0, it's an empty file, proceed.

    int freq[BYTE_RANGE] = {0};
    if (inputData) {
        for (long i = 0; i < originalFileSize; i++) {
            freq[inputData[i]]++;
        }
    }

    long compressedBitsCount = 0; // Initialize to 0
    bitStream = huffmanCompress(inputData, originalFileSize, &compressedBitsCount, freq);
    if (!bitStream && originalFileSize > 0) {
         fprintf(stderr, "Huffman compression failed.\n");
         free(inputData);
         return;
    }
    // If bitStream is NULL and originalFileSize is 0, it's okay.

    image = fopen(imagePath, "rb");
    if (!image) { fprintf(stderr, "Error opening input image: %s\n", imagePath); goto encode_cleanup; }
    output = fopen(outputPath, "wb");
    if (!output) { fprintf(stderr, "Error opening output image: %s\n", outputPath); goto encode_cleanup; }

    unsigned char header[54];
    if (fread(header, 1, 54, image) != 54) { fprintf(stderr, "Error reading BMP header.\n"); goto encode_cleanup; }
    if (fwrite(header, 1, 54, output) != 54) { fprintf(stderr, "Error writing BMP header.\n"); goto encode_cleanup; }

    long currentImagePos = ftell(image);
    fseek(image, 0, SEEK_END);
    long imageFileSize = ftell(image);
    fseek(image, currentImagePos, SEEK_SET);
    long availableBytesForData = imageFileSize - 54;
    long requiredBits = 32 + (BYTE_RANGE * 32) + compressedBitsCount;

    if (availableBytesForData < requiredBits) {
        fprintf(stderr, "Error: Image capacity insufficient.\n");
        fprintf(stderr, "  Available LSBs (pixels/bytes): %ld\n", availableBytesForData);
        fprintf(stderr, "  Required LSBs (bits): %ld\n", requiredBits);
        goto encode_cleanup;
    }
    printf("Capacity OK. Required: %ld bits, Available: %ld pixels\n", requiredBits, availableBytesForData);

    printf("Embedding file size (%ld bytes)...\n", originalFileSize);
    unsigned int size_val = (unsigned int)originalFileSize;
    for (int i = 0; i < 32; i++) {
        unsigned char pixel_byte;
        if(fread(&pixel_byte, 1, 1, image) != 1) {fprintf(stderr, "Error reading image pixel for size.\n"); goto encode_cleanup;}
        unsigned char bit_to_embed = (size_val >> (31 - i)) & 1;
        pixel_byte = (pixel_byte & 0xFE) | bit_to_embed;
        if(fwrite(&pixel_byte, 1, 1, output) != 1) {fprintf(stderr, "Error writing image pixel for size.\n"); goto encode_cleanup;}
    }

    printf("Embedding frequency table...\n");
    for (int i = 0; i < BYTE_RANGE; i++) {
        unsigned int freq_val = (unsigned int)freq[i];
        for (int j = 0; j < 32; j++) {
            unsigned char pixel_byte;
             if(fread(&pixel_byte, 1, 1, image) != 1) {fprintf(stderr, "Error reading image pixel for freq table.\n"); goto encode_cleanup;}
             unsigned char bit_to_embed = (freq_val >> (31 - j)) & 1;
            pixel_byte = (pixel_byte & 0xFE) | bit_to_embed;
            if(fwrite(&pixel_byte, 1, 1, output) != 1) {fprintf(stderr, "Error writing image pixel for freq table.\n"); goto encode_cleanup;}
        }
    }

    printf("Embedding compressed data (%ld bits)...\n", compressedBitsCount);
    // Only embed if there are bits to embed (handles empty file case)
    if (bitStream && compressedBitsCount > 0) {
        for (long i = 0; i < compressedBitsCount; i++) {
            unsigned char pixel_byte;
            if(fread(&pixel_byte, 1, 1, image) != 1) {fprintf(stderr, "Error reading image pixel for data.\n"); goto encode_cleanup;}
            unsigned char bit_to_embed = bitStream[i];
            pixel_byte = (pixel_byte & 0xFE) | bit_to_embed;
            if(fwrite(&pixel_byte, 1, 1, output) != 1) {fprintf(stderr, "Error writing image pixel for data.\n"); goto encode_cleanup;}
        }
    }

    printf("Copying remaining image data...\n");
    unsigned char pixel_byte;
    while (fread(&pixel_byte, 1, 1, image) == 1) {
        if (fwrite(&pixel_byte, 1, 1, output) != 1) {
             fprintf(stderr, "Error writing remaining pixel data.\n");
             break;
        }
    }
     printf("Encoding finished successfully for '%s'.\n", outputPath);

encode_cleanup:
    if (image) fclose(image);
    if (output) fclose(output);
    if (inputData) free(inputData);
    if (bitStream) free(bitStream);
}


// --- Decoding Function (read_lsb, decodeHuffmanFromImage) remain the same ---
int read_lsb(FILE *image) {
    unsigned char pixel_byte;
    if (fread(&pixel_byte, 1, 1, image) == 1) {
        return pixel_byte & 1;
    }
    return -1;
}

void decodeHuffmanFromImage(const char *stegoImagePath, const char *outputFilePath) {
    FILE *image = NULL, *output = NULL;
    unsigned char *decodedData = NULL;
    HuffmanNode *root = NULL;

    image = fopen(stegoImagePath, "rb");
    if (!image) { fprintf(stderr, "Error opening stego image: %s\n", stegoImagePath); return; }

    if (fseek(image, 54, SEEK_SET) != 0) { perror("fseek error skipping header"); fclose(image); return; }

    unsigned int originalFileSize = 0;
    printf("Reading original file size...\n");
    for (int i = 0; i < 32; i++) {
        int bit = read_lsb(image);
        if (bit < 0) { fprintf(stderr, "Error reading file size from image.\n"); goto decode_cleanup; }
        originalFileSize = (originalFileSize << 1) | bit;
    }
    printf("Extracted original file size: %u bytes\n", originalFileSize);

     if (originalFileSize == 0) {
        printf("Original file was empty. Creating empty output file.\n");
        output = fopen(outputFilePath, "wb");
        if (output) fclose(output);
        else perror("Error creating empty output file");
        goto decode_cleanup;
    }

    int freq[BYTE_RANGE];
    printf("Reading frequency table...\n");
    for (int i = 0; i < BYTE_RANGE; i++) {
        unsigned int freq_val = 0;
        for (int j = 0; j < 32; j++) {
             int bit = read_lsb(image);
             if (bit < 0) { fprintf(stderr, "Error reading frequency table from image.\n"); goto decode_cleanup; }
             freq_val = (freq_val << 1) | bit;
        }
        freq[i] = (int)freq_val;
    }

    printf("Rebuilding Huffman tree...\n");
    root = buildHuffmanTree(freq);
    if (!root) { fprintf(stderr, "Error rebuilding Huffman tree.\n"); goto decode_cleanup; }
    if (!root->left && !root->right) { fprintf(stderr, "Error: Rebuilt tree is empty but file size > 0.\n"); goto decode_cleanup; }

    decodedData = (unsigned char*)malloc(originalFileSize);
    if (!decodedData) { perror("Memory allocation failed for decoded data"); goto decode_cleanup; }

    printf("Decoding data...\n");
    HuffmanNode *currentNode = root;
    unsigned int bytesDecoded = 0;
    while (bytesDecoded < originalFileSize) {
        int bit = read_lsb(image);
        if (bit < 0) { fprintf(stderr, "Error: Unexpected end of image data during decoding.\n"); goto decode_cleanup; }

        if (bit == 0) { currentNode = currentNode->left; }
        else { currentNode = currentNode->right; }

        if (currentNode == NULL) {
             if (root->left && !root->right && !root->left->left && !root->left->right && bit == 0) {
                currentNode = root->left;
             } else {
                fprintf(stderr, "Error: Invalid path in Huffman tree during decoding.\n");
                goto decode_cleanup;
             }
        }

        if (currentNode && !currentNode->left && !currentNode->right) {
            if (bytesDecoded < originalFileSize) { // Check bounds before writing
                decodedData[bytesDecoded] = currentNode->data;
            } else {
                 fprintf(stderr, "Error: Decoded more bytes than expected.\n");
                 goto decode_cleanup;
            }
            bytesDecoded++;
            currentNode = root;
        }
    }
    printf("Decoded %u bytes.\n", bytesDecoded);

    output = fopen(outputFilePath, "wb");
    if (!output) { perror("Error creating output file"); goto decode_cleanup; }

    if (fwrite(decodedData, 1, originalFileSize, output) != originalFileSize) {
        fprintf(stderr, "Error writing decoded data to output file.\n");
        fclose(output); output = NULL;
        remove(outputFilePath);
        goto decode_cleanup;
    }
    printf("File extracted successfully to '%s'\n", outputFilePath);

decode_cleanup:
    if (image) fclose(image);
    if (output) fclose(output);
    if (decodedData) free(decodedData);
    if (root) freeHuffmanTree(root);
}


// -----------------------------------------------------------------------------
// NEW Main Function (Interactive CLI)
// -----------------------------------------------------------------------------

// Helper function to read a line and remove trailing newline
void readLine(char* buffer, int size) {
    if (fgets(buffer, size, stdin)) {
        buffer[strcspn(buffer, "\n")] = 0; // Remove trailing newline
    } else {
        buffer[0] = '\0'; // Handle EOF or read error
    }
}

// Helper function to check if filename ends with .bmp (case-insensitive)
int endsWithBMP(const char* filename) {
    const char* dot = strrchr(filename, '.'); // Find last dot
    if (dot && (strcasecmp(dot, ".bmp") == 0)) { // Compare ignoring case
         return 1;
    }
    return 0;
}


int main() {
    char choice[20];
    char inputImagePath[MAX_PATH_LEN];
    char secretFilePath[MAX_PATH_LEN];
    char outputImagePath[MAX_PATH_LEN];
    char stegoImagePath[MAX_PATH_LEN];
    char outputFilePath[MAX_PATH_LEN];

    printf("-------------------------------------------\n");
    printf(" Simple Huffman Steganography Tool (BMP) \n");
    printf("-------------------------------------------\n");

    printf("Choose operation:\n");
    printf("  1. Encode (hide file in BMP)\n");
    printf("  2. Decode (extract file from BMP)\n");
    printf("Enter choice (1 or 2): ");
    readLine(choice, sizeof(choice));

    if (strcmp(choice, "1") == 0) {
        printf("\n--- Encode Mode ---\n");

        // Get Input BMP Image Path
        printf("Enter path to the input BMP image file: ");
        readLine(inputImagePath, sizeof(inputImagePath));
        if (!endsWithBMP(inputImagePath)) {
            printf("Warning: Input image file does not end with .bmp. Proceeding anyway.\n");
            // You could add stricter checking or loop here if needed
        }

        // Get Secret File Path (can be any file type)
        printf("Enter path to the secret file to hide (text, image, etc.): ");
        readLine(secretFilePath, sizeof(secretFilePath));

        // Get Output BMP Image Path
        printf("Enter path for the output BMP image file (e.g., output.bmp): ");
        readLine(outputImagePath, sizeof(outputImagePath));
         if (!endsWithBMP(outputImagePath)) {
            printf("Warning: Output image file does not end with .bmp. Output will be created with this name.\n");
        }


        printf("\nStarting encoding...\n");
        encodeBinaryIntoImage(inputImagePath, secretFilePath, outputImagePath);
        // Result message is printed inside encodeBinaryIntoImage

    } else if (strcmp(choice, "2") == 0) {
        printf("\n--- Decode Mode ---\n");

        // Get Stego BMP Image Path
        printf("Enter path to the stego BMP image file (containing hidden data): ");
        readLine(stegoImagePath, sizeof(stegoImagePath));
         if (!endsWithBMP(stegoImagePath)) {
            printf("Warning: Stego image file does not end with .bmp. Proceeding anyway.\n");
        }

        // Get Output File Path for Extracted Data
        printf("Enter path for the extracted output file (e.g., extracted_secret.txt): ");
        readLine(outputFilePath, sizeof(outputFilePath));

        printf("\nStarting decoding...\n");
        decodeHuffmanFromImage(stegoImagePath, outputFilePath);
         // Result message is printed inside decodeHuffmanFromImage

    } else {
        fprintf(stderr, "Invalid choice. Please enter 1 or 2.\n");
        return 1;
    }

    printf("\nOperation finished.\n");
    return 0;
}