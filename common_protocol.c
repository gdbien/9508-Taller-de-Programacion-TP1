#include "common_protocol.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h> // borrar despues para debugear junto a gdb
#include <stdbool.h>

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
static int _create_pad_array(char** array, size_t amount) {
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
static char* _array_concat(char *arr1, size_t arr1_size, char *arr2, size_t arr2_size, int32_t *result_size) {
	*result_size = arr1_size + arr2_size;
    char *result = realloc(arr1, *result_size);
    if (!result) {
		free(arr1);
		return NULL;
	}
    memcpy(result + arr1_size, arr2,  arr2_size);
    return result;
}

static void _set_header_pre(header_pre_t *header_pre, int8_t endianess, int8_t m_type,
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


static void _set_base_param(base_param_t *base_param, int8_t par_type, int8_t byte_1,
							int8_t data_type, int8_t null_byte) {
	base_param->par_type = par_type;
	base_param->byte_1 = byte_1;
	base_param->data_type = data_type;
	base_param->null_byte = null_byte;
}

static void _set_norm_param(norm_param_t *norm_param, int8_t par_type, int8_t byte_1, 
							int8_t data_type, int8_t null_byte, int32_t data_size) {
	base_param_t base_param;
	_set_base_param(&base_param, par_type, byte_1, data_type, null_byte);
	norm_param->base_param = base_param;
	norm_param->data_size = data_size;
}

static void _set_sign_param(sign_param_t *sign_param, int8_t par_type, int8_t byte_1, 
							int8_t data_type, int8_t null_byte, int8_t arg_count) {
	base_param_t base_param;
	_set_base_param(&base_param, par_type, byte_1, data_type, null_byte);
	sign_param->base_param = base_param;
	sign_param->arg_count = arg_count;
	
}

static char* _strdup(const char *str) {
    size_t str_size = strlen(str) + 1;
    char *result = malloc(str_size);
    if (!result) return NULL;
    memcpy(result, str, str_size);
   	*(result + str_size - 1) = '\0';
    return result;
}

static size_t _get_arg_count(const char *buffer){
	size_t count = 0;
	bool only_one_arg = false;
	
	char *ptr_start = strchr(buffer, '(');
	char *ptr_end = strchr(buffer, ')');
	if ((ptr_end - ptr_start) == 2) {
		count = 1; //Manejo el caso de 1 arg
		only_one_arg = true;
	} 
	char *ptr = strrchr(buffer, ' ');
	while ((ptr = strchr(ptr, ','))) {
    	count++;
    	ptr++;
	}
	if (count == 0 || only_one_arg) return count;
	return count + 1;
}

/*
	Devuelve la cantidad de bytes que ocupa el nombre del metodo
*/
static size_t _get_bytes_method_name(char* method) {
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
static char* _get_method_name(char *method) {
	size_t bytes_name_method = _get_bytes_method_name(method);
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
static int _dup_param_names(const char *buffer, char* param_names[4]) {
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
    param_names[3] = _get_method_name(param_names[3]);
    free(copy);
    return SUCCESS;
}

/*
	Recibe la llamada de buffer, parsea por ',' los argumentos, y los
	alloca dinamicamente en arg_names.
	El usuario debe hacerse cargo de liberar la memoria de cada string.
	Devuelva SUCCESS si ok, ERROR en caso contrario.
*/
static int _dup_args_names(const char* buffer, char *arg_names[]) {
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


/*
	Devuelve el par_type y data_type, asociado al orden de ENCODE
	decidido por el usuario (para decodificar el enunciado decia
	que podian estar en cualquier orden, no necesariamente en este orden).
*/
static void _get_par_type_order() {
	static int array_order = 0;
	switch (array_order) {
		case 0:
			par_type = DESTINO;
			data_type = STRING;
			break;
		case 1:
			par_type = PATH;
			data_type = OBJECT;
			break;
		case 2:
			par_type = INTERFAZ;
			data_type = STRING;
			break;
		case 3:
			par_type = METODO;
			data_type = STRING;
			break;
	}
	array_order++;
}


static int _prot_enc_add_pad(char** param_arr, int32_t *pa_arr_size, char **pad_arr) {
	size_t n_multiple_8;
	size_t pad_bytes;
	n_multiple_8 = next_multiple_8(*pa_arr_size); 
	if (*pa_arr_size != n_multiple_8) {			
		pad_bytes = n_multiple_8 - *pa_arr_size;
		_create_pad_array(pad_arr, pad_bytes);
		*param_arr = _array_concat(*param_arr, *pa_arr_size, *pad_arr, pad_bytes, pa_arr_size);
	}
	return SUCCESS;
}

static int _prot_enc_proc_norm_param(char **param_arr, int32_t *pa_arr_size,
 									 char **param_name, char **pad_arr,
 									 int32_t *arr_size) {
	_get_par_type_order();
	norm_param_t norm_param;
	_set_norm_param(&norm_param, par_type, 1, data_type, '\0', htonl(strlen(*param_name)));
	*param_arr = _array_concat(*param_arr, *pa_arr_size, (char*)&norm_param, sizeof(norm_param_t), pa_arr_size);
	*param_arr = _array_concat(*param_arr, *pa_arr_size, *param_name, strlen(*param_name) + 1, pa_arr_size);
	free(*param_name);
	*arr_size = *pa_arr_size;
	_prot_enc_add_pad(param_arr, pa_arr_size, pad_arr);
	return SUCCESS;
}

static int _prot_enc_proc_sign_param(char **param_arr, int32_t *pa_arr_size,
 									 size_t n_arg, char **pad_arr,
 									 int32_t *arr_size) {
	sign_param_t sign_param;
	par_type = FIRMA;
	data_type = SIGNATURE;
	_set_sign_param(&sign_param, par_type, 1, data_type, '\0', n_arg);
	char * firma_s_arr = malloc(n_arg * sizeof(char) + 1);
	memset(firma_s_arr,'s',n_arg);
	*(firma_s_arr + n_arg) = '\0';
	*param_arr = _array_concat(*param_arr, *pa_arr_size, (char*)&sign_param, sizeof(sign_param_t), pa_arr_size);
	*param_arr = _array_concat(*param_arr, *pa_arr_size, firma_s_arr, strlen(firma_s_arr) + 1, pa_arr_size);
	free(firma_s_arr);
	*arr_size = *pa_arr_size;  //Me guardo la longitud antes de agregar padding
	_prot_enc_add_pad(param_arr, pa_arr_size, pad_arr);
	return SUCCESS;
}



static int _prot_enc_add_arg(char **body_arr, int32_t *body_size, char *arg_names[], size_t n_arg) {
	int32_t arg_length;
		for (size_t i = 0; i < n_arg; i++) {
			arg_length = strlen(arg_names[i]);
			arg_length = htonl(arg_length);
			*body_arr = _array_concat(*body_arr, *body_size, (char*) &arg_length, 4, body_size);
			*body_arr = _array_concat(*body_arr, *body_size, arg_names[i], strlen(arg_names[i]) + 1, body_size);
			free(arg_names[i]);
		}
	return SUCCESS;
}


static int _prot_enc_pack_data(char **data, size_t *encoded_size, header_pre_t *header_pre,
							  char *param_arr, int32_t pa_arr_size, size_t n_arg,
							  char *body_arr, int32_t body_size) {
	*data = _array_concat(*data, *encoded_size, (char*)header_pre, sizeof(header_pre_t), (int32_t*)encoded_size);
	*data = _array_concat(*data, *encoded_size, param_arr, pa_arr_size, (int32_t*)encoded_size);
	if (n_arg > 0) *data = _array_concat(*data, *encoded_size, body_arr, body_size, (int32_t*)encoded_size);
	return SUCCESS;
}



int protocol_encode(char **data, const char *buffer, size_t *encoded_size) {
	int32_t arr_size; //Para header_pre
	int32_t body_size = 0; //Para header_pre
	char *pad_arr = NULL;
	char *body_arr = NULL;
	*encoded_size = 0;

	char *param_names[4];
	_dup_param_names(buffer,param_names);

	char *param_arr = NULL;
	int32_t pa_arr_size = 0;

	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, &param_names[0], &pad_arr, &arr_size);
	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, &param_names[1], &pad_arr, &arr_size);
	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, &param_names[2], &pad_arr, &arr_size);
	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, &param_names[3], &pad_arr, &arr_size);

	size_t n_arg = _get_arg_count(buffer);
	if (n_arg > 0) {
		//Tengo parametro firma y body!
		char* arg_names[n_arg];
		_dup_args_names(buffer, arg_names);
		_prot_enc_proc_sign_param(&param_arr, &pa_arr_size,n_arg, &pad_arr, &arr_size);
		_prot_enc_add_arg(&body_arr, &body_size, arg_names, n_arg);
	}

	header_pre_t header_pre;
	_set_header_pre(&header_pre, endianess, m_type, flags, pro_version, htonl(body_size), htonl(arr_size));
	_prot_enc_pack_data(data, encoded_size, &header_pre, param_arr, pa_arr_size, n_arg, body_arr, body_size);
	free(param_arr);
	free(body_arr);
	free(pad_arr);
	return SUCCESS;
}

/*
	Procesa el string asociado a un parametro normal, almacenando memoria
	dinámica para cada uno en param_names, modificiando la posición de
	arr_idx en el array.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _prot_dec_proc_norm_param(const char *encoded_arr, size_t* arr_idx, char **param_name) {
	int32_t data_size;
	memcpy(&data_size, encoded_arr + *arr_idx, sizeof(int32_t));
	data_size = ntohl(data_size);
	*arr_idx += sizeof(int32_t);
	*param_name = malloc(data_size * sizeof(char) + 1);
	if (!*param_name) return ERROR;
	memcpy(*param_name, encoded_arr + *arr_idx, data_size + 1);
	*arr_idx += data_size + 1;
	return SUCCESS;
}

/*
	Decodifica los parametros.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/

int protocol_decode_parameters(const char *encoded_arr, size_t length, char *param_names[4], size_t *n_arg) {
	int n_multiple_8;
	base_param_t base_param;
	int8_t arg_count;
	size_t arr_idx = 0;
	int ret;
	while (arr_idx < length) {
		memcpy(&base_param, encoded_arr + arr_idx, sizeof(base_param_t));
		arr_idx += sizeof(base_param_t);
		par_type = base_param.par_type;
		switch (par_type) {
			case DESTINO:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx, &param_names[0]);
				if (ret == ERROR) return ERROR;
				break;
			case PATH:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx, &param_names[1]);
				if (ret == ERROR) return ERROR;
				break;
			case INTERFAZ:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx, &param_names[2]);
				if (ret == ERROR) return ERROR;
				break;
			case METODO:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx, &param_names[3]);
				if (ret == ERROR) return ERROR;
				break;
			case FIRMA:
				memcpy(&arg_count, encoded_arr + arr_idx, sizeof(int8_t));
				*n_arg = arg_count;
				arr_idx += sizeof(int8_t);
				arr_idx += arg_count + 1;
				break;
			default:
				printf("ERROR! 404 NOT FOUND\n");
				break;
		}
		n_multiple_8 = next_multiple_8(arr_idx);
		if (arr_idx != n_multiple_8) {
			arr_idx += n_multiple_8 - arr_idx;
		}
	}
	return SUCCESS;
}

int protocol_decode_arguments(const char *encoded_body, char *arg_names[], size_t n_arg) {
	size_t idx = 0;
	int32_t string_size;
	for (size_t i = 0; i < n_arg; i++) {
		memcpy(&string_size, encoded_body + idx, sizeof(int32_t));
		string_size = ntohl(string_size);
		idx += sizeof(int32_t);
		arg_names[i] = malloc(string_size * sizeof(char) + 1);
		memcpy(arg_names[i], encoded_body + idx, string_size + 1);
		idx += string_size + 1;
	}
	return SUCCESS;
}
