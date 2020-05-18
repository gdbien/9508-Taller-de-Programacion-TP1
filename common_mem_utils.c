#include "common_mem_utils.h"
#include <string.h>

char* array_concat(char *arr1, size_t arr1_size, char *arr2, size_t arr2_size,
                   int32_t *result_size) {
	*result_size = arr1_size + arr2_size;
    char *result = realloc(arr1, *result_size);
    if (!result) {
		return NULL;
	}
    memcpy(result + arr1_size, arr2, arr2_size);
    return result;
}

char* strdup(const char *str) {
    size_t str_size = strlen(str) + 1;
    char *result = malloc(str_size);
    if (!result) return NULL;
    memcpy(result, str, str_size);
   	*(result + str_size - 1) = '\0';
    return result;
}

char* dynamic_concat(char *str_1, char *str_2) {
    int length = 0;
    char *result;

    if (str_1) length = strlen(str_1);      
    length += strlen(str_2) + 1;
    result = realloc(str_1, length);
    if (!result) {
        free(str_1);
        return NULL;
    }
    if (!str_1) *result = '\0'; 
    strcat(result, str_2);
    return result;
}
