#include "common_protocol.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h> // borrar despues para debugear junto a gdb

//Pre-Header
enum {LITTLE = 'l', BIG = 'b'} endianess = LITTLE;
enum {M_DEFAULT = 1} m_type = M_DEFAULT;
enum {F_DEFAULT = 0} flags = F_DEFAULT;
enum {P_DEFAULT = 1} pro_version = P_DEFAULT;


//Parameters
enum {PATH = 1, DESTINO = 6, INTERFAZ = 2, METODO = 3, FIRMA = 8} par_type;
enum {OBJECT = 'o', STRING = 's', SIGNATURE = 'g'} data_type;

/*
	Crea un array de padding de amount bytes, modificando 
	*array.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
int create_pad_array(char** array, size_t amount) {
	char *result = realloc(*array, amount);
    if (!result) {
    	free(*array);
		return ERROR;
	}
	memset(result, 0, amount);
	*array = result;
	return SUCCESS;
}

/*
	Concatena en un array resultante, arr1 y arr2, reservando memoria dinamica
	que el usuario debe liberar. Debe ser llamado con arr1 = NULL;

*/
char* array_concat(char *arr1, size_t arr1_size, char *arr2, size_t arr2_size, int32_t *result_size) {
	*result_size = arr1_size + arr2_size;
    char *result = realloc(arr1, *result_size);
    if (!result) {
		free(arr1);
		return NULL;
	}
    memcpy(result + arr1_size, arr2,  arr2_size);
    return result;
}

void set_header_pre(header_pre_t *header_pre, int8_t endianess, int8_t m_type,
					int8_t flags, int8_t pro_version, int32_t body_size,
					int32_t arr_size) {
	static int message_id = 1;
	header_pre->endianess = endianess;
	header_pre->m_type = m_type;
	header_pre->flags = flags;
	header_pre->pro_version = pro_version;
	header_pre->body_size = body_size;
	header_pre->message_id = message_id;
	header_pre->arr_size = arr_size;
	message_id++;
}

/*
	Devuelve el multiplo de 8 mayor mas cercano a number.
*/

int next_multiple_8(size_t number) {
	//((n + denominator -1) / denominator )* denominator 
	return (number + (8 - 1)) & ~(8 - 1); 

}


void set_base_param(base_param_t *base_param, int8_t par_type, int8_t byte_1,
					int8_t data_type, int8_t null_byte) {
	base_param->par_type = par_type;
	base_param->byte_1 = byte_1;
	base_param->data_type = data_type;
	base_param->null_byte = null_byte;
}

void set_norm_param(norm_param_t *norm_param, int8_t par_type, int8_t byte_1, 
					int8_t data_type, int8_t null_byte, int32_t data_size) {
	base_param_t base_param;
	set_base_param(&base_param, par_type, byte_1, data_type, null_byte);
	norm_param->base_param = base_param;
	norm_param->data_size = data_size;
}

void set_sign_param(sign_param_t *sign_param, int8_t par_type, int8_t byte_1, 
					int8_t data_type, int8_t null_byte, int8_t arg_count) {
	base_param_t base_param;
	set_base_param(&base_param, par_type, byte_1, data_type, null_byte);
	sign_param->base_param = base_param;
	sign_param->arg_count = arg_count;
	
}

char* _strdup(const char *str) {
    size_t str_size = strlen(str) + 1;
    char *result = malloc(str_size);
    if (!result) return NULL;
    memcpy(result, str, str_size);
   	*(result + str_size - 1) = '\0';
    return result;
}

size_t get_arg_count(const char *buffer){
	size_t count = 0;
	char *ptr = strrchr(buffer, ' ');
	while ((ptr = strchr(ptr, ','))) {
    	count++;
    	ptr++;
	}
	if (count == 0) return count;
	return count + 1;
}

/*
	Devuelve la cantidad de bytes que ocupa el nombre del metodo
*/
size_t get_bytes_method_name(char* method) {
	size_t count = 0;
	while (*method != '(') {
		method++;
		count++;
	} 
	return count;
}

/*
	Se encarga de pedir la memoria dinamica exacta para almacenar el nombre
	del metodo (sin los parametros). EL usuario debe liberar la memoria, y
	method debe ser un string previamente en el heap.
	Devuelve un string con el nombre, NULL en caso de error.
*/
char* get_method_name(char *method) {
	size_t bytes_name_method = get_bytes_method_name(method);
	char* result = realloc(method, bytes_name_method + 1);
	if (!result) return NULL;
	*(result + bytes_name_method) = '\0';
	return result;
}

/*
	Recibe la llamada de buffer, parsea por espacio las palabras, y las
	alloca dinamicamente en param_names.
	El usuario debe hacerse cargo de liberar la memoria de cada string.
	Devuelva SUCCESS si ok, ERROR en caso contrario.
*/
int dup_param_names(const char *buffer, char* param_names[4]) {
	size_t i = 0;
	char *copy = _strdup(buffer);
	if (!copy) return ERROR;
	char *token = strtok(copy, " ");
    while (token) {
        param_names[i] = _strdup(token);
        if(!param_names[i]) {
        	free(copy);
        	return ERROR;
        }
        token = strtok (NULL, " ");
        i++;
    }
    param_names[3] = get_method_name(param_names[3]);
    free(copy);
    return SUCCESS;
}

/*
	Recibe la llamada de buffer, parsea por ',' los argumentos, y los
	alloca dinamicamente en arg_names.
	El usuario debe hacerse cargo de liberar la memoria de cada string.
	Devuelva SUCCESS si ok, ERROR en caso contrario.
	FALTO VER CASO ESPECIAL DONDE HAY ESPACIO ENTRE LOS ARGUMENTOS,
	LO VA A TOMAR COMO PARTES DEL NOMBRE.
*/
int dup_args_names(const char* buffer, char *arg_names[]) {
	size_t i = 0;

	char *ptr_start = strchr(buffer, '(');
	char *ptr_end = strchr(buffer, ')');
	size_t bytes_amount = ptr_end - ptr_start - 1;

	char *args= malloc(bytes_amount * sizeof(char)+1);
	if(!args) return ERROR;
	memcpy(args, ptr_start + 1, bytes_amount);
	*(args + bytes_amount) = '\0';

	char *token = strtok(args, ",");
    while (token) {
        arg_names[i] = _strdup(token);
        token = strtok (NULL, ",");
        i++;
    }
    free(args);
    return SUCCESS;
}

int protocol_encode(char **data, const char *buffer, size_t *encoded_size) {
	*encoded_size = 0;
	int32_t arr_size; //ESTE SE USA EN EL HEADER_PRE
	size_t n_multiple_8;
	size_t pad_bytes;
	char *pad_arr = NULL;
	int32_t body_size = 0; //ESTE SE USA EN EL HEADER_PRE
	char *body_arr = NULL;

	char *param_names[4];
	dup_param_names(buffer,param_names);

	norm_param_t param_destino;
	norm_param_t param_path;
	norm_param_t param_interfaz;
	norm_param_t param_metodo;

	char *param_arr = NULL;
	int32_t pa_arr_size = 0;

	par_type = DESTINO;
	data_type = STRING;
	set_norm_param(&param_destino, par_type, 1, data_type, '\0', htonl(strlen(param_names[0])));
	param_arr = array_concat(param_arr, pa_arr_size, (char*)&param_destino, sizeof(norm_param_t), &pa_arr_size);
	param_arr = array_concat(param_arr, pa_arr_size, param_names[0], strlen(param_names[0])+1, &pa_arr_size);
	free(param_names[0]);
	n_multiple_8 = next_multiple_8(pa_arr_size); 
	if (pa_arr_size != n_multiple_8) {
		//Padding
		pad_bytes = n_multiple_8 - pa_arr_size;
		create_pad_array(&pad_arr, pad_bytes);
		param_arr = array_concat(param_arr, pa_arr_size, pad_arr, pad_bytes, &pa_arr_size);
	}

	par_type = PATH;
	data_type = OBJECT;
	set_norm_param(&param_path, par_type, 1, data_type, '\0', htonl(strlen(param_names[1])));
	param_arr = array_concat(param_arr, pa_arr_size, (char*)&param_path, sizeof(norm_param_t), &pa_arr_size);
	param_arr = array_concat(param_arr, pa_arr_size, param_names[1], strlen(param_names[1])+1, &pa_arr_size);
	free(param_names[1]);

	n_multiple_8 = next_multiple_8(pa_arr_size); 
	if (pa_arr_size != n_multiple_8) {
		//Padding
		pad_bytes = n_multiple_8 - pa_arr_size;
		create_pad_array(&pad_arr, pad_bytes);
		param_arr = array_concat(param_arr, pa_arr_size, pad_arr, pad_bytes, &pa_arr_size);
	}

	par_type = INTERFAZ;
	data_type = STRING;
	set_norm_param(&param_interfaz, par_type, 1, data_type, '\0', htonl(strlen(param_names[2])));
	param_arr = array_concat(param_arr, pa_arr_size, (char*)&param_interfaz, sizeof(norm_param_t), &pa_arr_size);
	param_arr = array_concat(param_arr, pa_arr_size, param_names[2], strlen(param_names[2])+1, &pa_arr_size);
	free(param_names[2]);

	n_multiple_8 = next_multiple_8(pa_arr_size); 
	if (pa_arr_size != n_multiple_8) {
		//Padding
		pad_bytes = n_multiple_8 - pa_arr_size;
		create_pad_array(&pad_arr, pad_bytes);
		param_arr = array_concat(param_arr, pa_arr_size, pad_arr, pad_bytes, &pa_arr_size);
	}

	par_type = METODO;
	data_type = STRING;
	set_norm_param(&param_metodo, par_type, 1, data_type, '\0', htonl(strlen(param_names[3])));
	param_arr = array_concat(param_arr, pa_arr_size, (char*)&param_metodo, sizeof(norm_param_t), &pa_arr_size);
	param_arr = array_concat(param_arr, pa_arr_size, param_names[3], strlen(param_names[3])+1, &pa_arr_size);
	free(param_names[3]);

	arr_size = pa_arr_size; //Me guardo la longitud antes de agregar padding (si es que despues no me lo reemplaza el if abajo)

	n_multiple_8 = next_multiple_8(pa_arr_size); 
	if (pa_arr_size != n_multiple_8) {
		//Padding		
		pad_bytes = n_multiple_8 - pa_arr_size;
		create_pad_array(&pad_arr, pad_bytes);
		param_arr = array_concat(param_arr, pa_arr_size, pad_arr, pad_bytes, &pa_arr_size);
	}

	size_t n_arg = get_arg_count(buffer);
	if (n_arg > 0) {
		//Tengo parametro firma y body!
		char* arg_names[n_arg];
		dup_args_names(buffer, arg_names);
		sign_param_t param_firma;

		par_type = FIRMA;
		data_type = SIGNATURE;
		set_sign_param(&param_firma, par_type, 1, data_type, '\0', n_arg);
		char * firma_s_arr = malloc(n_arg * sizeof(char) + 1);
		//EN CASO DE ERROR CHEQUEAR QUE PASA
		memset(firma_s_arr,'s',n_arg);
		*(firma_s_arr + n_arg) = '\0';

		param_arr = array_concat(param_arr, pa_arr_size, (char*)&param_firma, sizeof(sign_param_t), &pa_arr_size);
		param_arr = array_concat(param_arr, pa_arr_size, firma_s_arr, strlen(firma_s_arr) + 1, &pa_arr_size);
		free(firma_s_arr);

		arr_size = pa_arr_size;  //Me guardo la longitud antes de agregar padding

		n_multiple_8 = next_multiple_8(pa_arr_size); 
		if (pa_arr_size != n_multiple_8) {
			//Padding			
			pad_bytes = n_multiple_8 - pa_arr_size;
			create_pad_array(&pad_arr, pad_bytes);
			param_arr = array_concat(param_arr, pa_arr_size, pad_arr, pad_bytes, &pa_arr_size);
		}
		int32_t arg_length;
		for (size_t i = 0; i < n_arg; i++) {
			arg_length = strlen(arg_names[i]);
			arg_length = htonl(arg_length);
			body_arr = array_concat(body_arr, body_size, (char*) &arg_length, 4, &body_size);
			body_arr = array_concat(body_arr, body_size, arg_names[i], strlen(arg_names[i]) + 1, &body_size);
			free(arg_names[i]);
		}
	}

	header_pre_t header_pre;
	set_header_pre(&header_pre, endianess, m_type, flags, pro_version, htonl(body_size), htonl(arr_size));

	*data = array_concat(*data, *encoded_size, (char*)&header_pre, sizeof(header_pre_t), (int32_t*)encoded_size);
	*data = array_concat(*data, *encoded_size, param_arr, pa_arr_size, (int32_t*)encoded_size);
	if (n_arg > 0) *data = array_concat(*data, *encoded_size, body_arr, body_size, (int32_t*)encoded_size);
	free(param_arr);
	free(body_arr);
	free(pad_arr);
	return SUCCESS;
}


// AGREGAR QUE DEVUELVA ERROR EN CASO DE FALLA DE MALLOC O SUCCESS
int protocol_decode_parameters(const char *encoded_arr, size_t length, char *param_names[4], size_t* n_arg) {
	int n_multiple_8;
	base_param_t base_param;
	int32_t data_size;
	int8_t arg_count;
	int idx = 0;
	while (idx < length) {
		memcpy(&base_param, encoded_arr + idx, sizeof(base_param_t));
		idx += sizeof(base_param_t);
		//Me fijo en que caso caigo
		par_type = base_param.par_type; //capaz falte casteo
		printf("VALOR PAR TYPE: %d\n", par_type);
		switch (par_type) {
			case DESTINO:
				memcpy(&data_size, encoded_arr + idx, sizeof(int32_t));
				data_size = ntohl(data_size);
				idx += sizeof(int32_t);
				param_names[0] = malloc(data_size * sizeof(char) + 1); //El 1 es para el \0
				memcpy(param_names[0], encoded_arr + idx, data_size + 1);
				idx += data_size + 1;
				break;
			case PATH:
				memcpy(&data_size, encoded_arr + idx, sizeof(int32_t));
				data_size = ntohl(data_size);
				idx += sizeof(int32_t);
				param_names[1] = malloc(data_size * sizeof(char) + 1); //El 1 es para el \0
				memcpy(param_names[1], encoded_arr + idx, data_size + 1);
				idx += data_size + 1;
				break;
			case INTERFAZ:
				memcpy(&data_size, encoded_arr + idx, sizeof(int32_t));
				data_size = ntohl(data_size);
				idx += sizeof(int32_t);
				param_names[2] = malloc(data_size * sizeof(char) + 1); //El 1 es para el \0
				memcpy(param_names[2], encoded_arr + idx, data_size + 1);
				idx += data_size + 1;
				break;
			case METODO:
				memcpy(&data_size, encoded_arr + idx, sizeof(int32_t));
				data_size = ntohl(data_size);
				idx += sizeof(int32_t);
				param_names[3] = malloc(data_size * sizeof(char) + 1); //El 1 es para el \0
				memcpy(param_names[3], encoded_arr + idx, data_size + 1);
				idx += data_size + 1;
				break;
			case FIRMA:
				memcpy(&arg_count, encoded_arr + idx, sizeof(int8_t));
				*n_arg = arg_count;
				idx += sizeof(int8_t);
				idx += arg_count + 1;
				break;
			default:
				printf("ERROR NO DEBERIAS HABER LLEGADO ACA!\n");
				break;
		}
		n_multiple_8 = next_multiple_8(idx);
		if (idx != n_multiple_8) {
			idx += n_multiple_8 - idx;
		}

	}
	return SUCCESS;
}


int protocol_decode_arguments(const char* encoded_body, size_t body_size, char *arg_names[], size_t n_arg) {
	size_t idx = 0;
	int32_t string_size;

	for (size_t i = 0; i < n_arg; i++) {
		memcpy(&string_size, encoded_body + idx, sizeof(int32_t));
		string_size = ntohl(string_size);
		idx += sizeof(int32_t);
		arg_names[i] = malloc(string_size * sizeof(char) + 1); //El 1 es para el \0
		memcpy(arg_names[i], encoded_body + idx, string_size + 1);
		idx += string_size + 1;
	}
	return SUCCESS;
}
